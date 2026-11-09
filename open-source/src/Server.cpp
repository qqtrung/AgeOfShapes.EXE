#include "Server.hpp"

#include "Debug.hpp"
#include "GameConfig.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

Server::Server(int clientCount, bool enableGui) :
    mClientCount(std::max(1, clientCount)),
    mGuiEnabled(enableGui),
    mLastFrameTime(std::chrono::steady_clock::now())
{
    if (mGuiEnabled)
    {
        const unsigned int winW = 600;
        const unsigned int winH = 380;

        mWindow = std::make_unique<sf::RenderWindow>(sf::VideoMode{sf::Vector2u{winW, winH}},
                                                     "DEDICATED SERVER (HOST) - Traffic & Room Monitor",
                                                     sf::Style::Titlebar | sf::Style::Close);
        mWindow->setPosition(sf::Vector2i{15, 30});
        mWindow->setFramerateLimit(mTargetFps);

        if (!mFont.openFromFile("fonts/consolas.ttf"))
        {
            (void)mFont.openFromFile("C:/Windows/Fonts/consola.ttf");
        }
    }

    logTraffic("Server started successfully (GUI: " + std::string(mGuiEnabled ? "Enabled" : "Disabled") + ")");
}

Server::~Server()
{
    stop();
}

void Server::stop()
{
    mRunning.store(false);
    stopNetwork();
    if (mWindow && mWindow->isOpen())
    {
        mWindow->close();
    }
}

void Server::setGuiEnabled(bool enable)
{
    if (mGuiEnabled == enable)
    {
        return;
    }

    mGuiEnabled = enable;
    if (mGuiEnabled)
    {
        const unsigned int winW = 600;
        const unsigned int winH = 380;
        mWindow                 = std::make_unique<sf::RenderWindow>(sf::VideoMode{sf::Vector2u{winW, winH}},
                                                                     "DEDICATED SERVER (HOST) - Traffic & Room Monitor",
                                                                     sf::Style::Titlebar | sf::Style::Close);
        mWindow->setPosition(sf::Vector2i{15, 30});
        mWindow->setFramerateLimit(mTargetFps);
        if (!mFont.openFromFile("fonts/consolas.ttf"))
        {
            (void)mFont.openFromFile("C:/Windows/Fonts/consola.ttf");
        }
        logout("[Server] GUI enabled.");
    }
    else
    {
        if (mWindow && mWindow->isOpen())
        {
            mWindow->close();
        }
        mWindow.reset();
        logout("[Server] GUI disabled. Server running headless at 256 FPS.");
    }
}

bool Server::isGuiEnabled() const
{
    return mGuiEnabled && mWindow && mWindow->isOpen();
}

bool Server::startNetwork(uint16_t port, size_t maxClients, uint32_t targetHz)
{
    return mNetwork.start(port, maxClients, targetHz);
}

void Server::stopNetwork()
{
    mNetwork.stop();
}

bool Server::isNetworkRunning() const
{
    return mNetwork.isRunning();
}

void Server::logTraffic(const std::string& log)
{
    logout("[Server] " + log);
    mPacketLogs.push_back(log);
    if (mPacketLogs.size() > 14)
    {
        mPacketLogs.erase(mPacketLogs.begin());
    }
}

void Server::sendPacketToClient(int clientId, Packet pkt, uint64_t now)
{
    if (mConnectedClients.contains(clientId) && mConnectedClients[clientId].networkAddress)
    {
        mNetwork.sendPacketToClient(mConnectedClients[clientId].networkAddress, clientId, std::move(pkt), now);
    }
}

void Server::broadcastPacket(Packet pkt, uint64_t now)
{
    for (const auto& [id, client] : mConnectedClients)
    {
        if (client.networkAddress)
        {
            mNetwork.sendPacketToClient(client.networkAddress, id, pkt, now);
        }
    }
}

void Server::broadcastLobbySync(uint64_t now)
{
    PacketLobbySync sync;
    for (const auto& [id, client] : mConnectedClients)
    {
        sync.clients.push_back(PacketLobbyClientInfo{client.id, client.name, client.roomId, client.pingMs, client.status});
    }

    for (const auto& [id, room] : mRooms)
    {
        std::string hostName = "Unknown";
        if (mConnectedClients.contains(room.hostClientId))
        {
            hostName = mConnectedClients[room.hostClientId].name;
        }

        sync.rooms.push_back(
            PacketLobbyRoomInfo{room.id,
                                hostName,
                                static_cast<int>(room.memberClientIds.size()),
                                3,
                                room.level,
                                room.difficulty,
                                room.inGame});
    }

    Packet pkt;
    pkt.type = PacketType::LobbySync;
    pkt.data = std::move(sync);
    broadcastPacket(std::move(pkt), now);
}

void Server::broadcastRoomSync(int roomId, uint64_t now, const std::string& notice)
{
    if (!mRooms.contains(roomId))
    {
        return;
    }

    const auto&    room = mRooms[roomId];
    PacketRoomSync sync;
    sync.roomId       = room.id;
    sync.hostClientId = room.hostClientId;
    sync.level        = room.level;
    sync.difficulty   = room.difficulty;
    sync.gameMode     = room.gameMode;
    sync.inGame       = room.inGame;
    sync.notice       = notice;

    auto& mutableRoom = mRooms[roomId];
    mutableRoom.memberClientIds.erase(std::remove_if(mutableRoom.memberClientIds.begin(),
                                                     mutableRoom.memberClientIds.end(),
                                                     [&](int mid) { return mid < 0 || !mConnectedClients.contains(mid); }),
                                      mutableRoom.memberClientIds.end());

    for (int mid : mutableRoom.memberClientIds)
    {
        std::string mName = (mid >= 0) ? ("Player " + std::to_string(mid + 1)) : "Player";
        if (mConnectedClients.contains(mid))
        {
            mName = mConnectedClients[mid].name;
        }

        bool isHost = (mid == mutableRoom.hostClientId);
        bool isReady = isHost ? true : (mutableRoom.readyStates.contains(mid) ? mutableRoom.readyStates.at(mid) : false);

        sync.slots.push_back(RoomSlotSync{mid, mName, isHost, isReady});
    }

    for (int mid : mutableRoom.memberClientIds)
    {
        Packet pkt;
        pkt.type = PacketType::RoomSync;
        pkt.data = sync;
        sendPacketToClient(mid, std::move(pkt), now);
    }
}

void Server::dissolveRoom(int roomId, const std::string& reason, uint64_t now)
{
    if (!mRooms.contains(roomId))
    {
        return;
    }

    Room room = mRooms[roomId];
    mRooms.erase(roomId);

    logTraffic("[ROOM #" + std::to_string(roomId) + "] Dissolved: " + reason);

    for (int mid : room.memberClientIds)
    {
        if (mConnectedClients.contains(mid))
        {
            mConnectedClients[mid].roomId = -1;
            mConnectedClients[mid].status = "Lobby (Idle)";
        }

        Packet leavePkt;
        leavePkt.type = PacketType::RoomLeave;
        leavePkt.data = PacketRoomLeave{roomId, mid, reason};
        sendPacketToClient(mid, std::move(leavePkt), now);

        if (room.inGame)
        {
            Packet discPkt;
            discPkt.type = PacketType::Disconnect;
            discPkt.data = PacketDisconnect{mid, reason};
            sendPacketToClient(mid, std::move(discPkt), now);
        }
    }

    broadcastLobbySync(now);
}

void Server::leaveCurrentRoom(int clientId, const std::string& reason, uint64_t now)
{
    if (!mConnectedClients.contains(clientId))
    {
        return;
    }

    int rid = mConnectedClients[clientId].roomId;
    if (rid == -1 || !mRooms.contains(rid))
    {
        for (const auto& [id, room] : mRooms)
        {
            if (std::ranges::find(room.memberClientIds, clientId) != room.memberClientIds.end())
            {
                rid = id;
                break;
            }
        }
    }

    if (rid == -1 || !mRooms.contains(rid))
    {
        mConnectedClients[clientId].roomId = -1;
        mConnectedClients[clientId].status = "Lobby (Idle)";
        Packet leaveAck;
        leaveAck.type = PacketType::RoomLeave;
        leaveAck.data = PacketRoomLeave{rid, clientId, "Returned to lobby."};
        sendPacketToClient(clientId, std::move(leaveAck), now);
        broadcastLobbySync(now);
        return;
    }

    std::string cName = mConnectedClients[clientId].name;
    auto&       room  = mRooms[rid];

    if (clientId == room.hostClientId || room.inGame)
    {
        dissolveRoom(rid, reason.empty() ? ("Room dissolved: " + cName + " left the room.") : reason, now);
    }
    else
    {
        room.memberClientIds.erase(std::remove(room.memberClientIds.begin(), room.memberClientIds.end(), clientId),
                                   room.memberClientIds.end());
        room.readyStates.erase(clientId);

        mConnectedClients[clientId].roomId = -1;
        mConnectedClients[clientId].status = "Lobby (Idle)";

        Packet leaveAck;
        leaveAck.type = PacketType::RoomLeave;
        leaveAck.data = PacketRoomLeave{rid, clientId, "Returned to lobby."};
        sendPacketToClient(clientId, std::move(leaveAck), now);

        if (room.memberClientIds.empty())
        {
            dissolveRoom(rid, "Room closed: all members left.", now);
        }
        else
        {
            broadcastRoomSync(rid, now, cName + " left the room.");
            broadcastLobbySync(now);
        }
    }
}

void Server::removeClient(int clientId, const std::string& reason, uint64_t now)
{
    if (!mConnectedClients.contains(clientId))
    {
        return;
    }

    std::string cName = mConnectedClients[clientId].name;
    logTraffic("[C" + std::to_string(clientId) + "] (" + cName + ") disconnected: " + reason);

    mConnectedClients[clientId].networkAddress = nullptr;

    std::vector<int> roomsToDissolve;
    for (auto& [rid, room] : mRooms)
    {
        bool isInThisRoom = (std::ranges::find(room.memberClientIds, clientId) != room.memberClientIds.end());
        if (room.hostClientId == clientId || (room.inGame && isInThisRoom))
        {
            roomsToDissolve.push_back(rid);
        }
        else if (isInThisRoom)
        {
            room.memberClientIds.erase(std::remove(room.memberClientIds.begin(), room.memberClientIds.end(), clientId),
                                       room.memberClientIds.end());
            room.readyStates.erase(clientId);
            if (room.memberClientIds.empty())
            {
                roomsToDissolve.push_back(rid);
            }
            else
            {
                broadcastRoomSync(rid, now, cName + " disconnected.");
            }
        }
    }

    for (int rid : roomsToDissolve)
    {
        dissolveRoom(rid, "Room closed: " + cName + " disconnected.", now);
    }

    mConnectedClients.erase(clientId);

    broadcastLobbySync(now);
}

void Server::input()
{
    if (mGuiEnabled && mWindow && mWindow->isOpen())
    {
        bool shouldClose = false;
        while (const std::optional event = mWindow->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                shouldClose = true;
                break;
            }
        }

        if (shouldClose)
        {
            mWindow->close();
            mWindow.reset();
            mGuiEnabled = false;
            logout("[Server] GUI closed. Server continuing in headless mode (256 FPS)...");
        }
    }
}

void Server::network(uint64_t)
{
    mNetwork.sync();
}

void Server::update(uint64_t now)
{
    auto resolveClientId = [&](int cId, Network* src) -> int
    {
        if (src)
        {
            for (const auto& [id, client] : mConnectedClients)
            {
                if (client.networkAddress && client.networkAddress == src)
                {
                    return id;
                }
            }
            return -1;
        }
        if (cId >= 0 && mConnectedClients.contains(cId))
        {
            return cId;
        }
        return -1;
    };

    while (!mNetwork.ingoingPacket.empty())
    {
        Packet pkt = std::move(mNetwork.ingoingPacket.front());
        mNetwork.ingoingPacket.pop_front();

        switch (pkt.type)
        {
            case PacketType::Ping:
            {
                int    cId = resolveClientId(pkt.clientId, pkt.srcAddress);
                Packet pongPkt;
                pongPkt.type = PacketType::Pong;
                if (const auto* ping = pkt.getIf<PacketPing>())
                {
                    pongPkt.data = PacketPong{ping->timestamp};
                }
                sendPacketToClient(cId, std::move(pongPkt), now);
                break;
            }
            case PacketType::Connect:
            {
                if (const auto* conn = pkt.getIf<PacketConnect>())
                {
                    int staleId = -1;
                    if (pkt.srcAddress)
                    {
                        for (const auto& [id, client] : mConnectedClients)
                        {
                            if (client.networkAddress && client.networkAddress == pkt.srcAddress)
                            {
                                staleId = id;
                                break;
                            }
                        }
                    }
                    if (staleId != -1)
                    {
                        removeClient(staleId, "Peer reconnected with new session", now);
                    }

                    int cId = mNextClientId++;

                    std::string pName = conn->playerName;
                    if (pName.empty() || pName.rfind("Player", 0) == 0)
                    {
                        pName = "Player " + std::to_string(cId + 1);
                    }

                    mConnectedClients[cId] = ConnectedClient{cId, pName, -1, 20, "Lobby (Idle)", pkt.srcAddress};
                    logTraffic("[C" + std::to_string(cId) + "] " + pName +
                               " connected (assigned ID: " + std::to_string(cId) + ")");

                    Packet ack;
                    ack.type = PacketType::ConnectAck;
                    ack.data = PacketConnectAck{cId, true};
                    mNetwork.sendPacketToClient(pkt.srcAddress, cId, std::move(ack), now);

                    broadcastLobbySync(now);
                }
                break;
            }
            case PacketType::Disconnect:
            {
                int cId = resolveClientId(pkt.clientId, pkt.srcAddress);
                if (cId < 0)
                {
                    if (const auto* disc = pkt.getIf<PacketDisconnect>())
                    {
                        if (disc->playerId >= 0 && mConnectedClients.contains(disc->playerId))
                        {
                            cId = disc->playerId;
                        }
                    }
                    if (cId < 0 && pkt.clientId >= 0 && mConnectedClients.contains(pkt.clientId))
                    {
                        cId = pkt.clientId;
                    }
                }
                if (cId < 0)
                {
                    break;
                }
                std::string reason = "Client disconnected";
                if (const auto* disc = pkt.getIf<PacketDisconnect>())
                {
                    if (!disc->reason.empty())
                    {
                        reason = disc->reason;
                    }
                }
                removeClient(cId, reason, now);
                break;
            }
            case PacketType::RoomCreate:
            {
                int cId = resolveClientId(pkt.clientId, pkt.srcAddress);
                if (!mConnectedClients.contains(cId))
                {
                    cId                    = mNextClientId++;
                    std::string pName      = "Player " + std::to_string(cId + 1);
                    mConnectedClients[cId] = ConnectedClient{cId, pName, -1, 20, "Lobby (Idle)", pkt.srcAddress};
                    logTraffic("[C" + std::to_string(cId) + "] " + pName + " auto-registered on RoomCreate");
                    Packet ackPkt;
                    ackPkt.type = PacketType::ConnectAck;
                    ackPkt.data = PacketConnectAck{cId, true};
                    sendPacketToClient(cId, std::move(ackPkt), now);
                }

                int oldRid = mConnectedClients[cId].roomId;
                if (oldRid != -1 && mRooms.contains(oldRid))
                {
                    leaveCurrentRoom(cId, "Room dissolved: Host created a new room.", now);
                }

                int  rid = mNextRoomId++;
                Room r;
                r.id           = rid;
                r.hostClientId = cId;
                r.name         = "Room #" + std::to_string(rid);
                r.memberClientIds.push_back(cId);
                r.readyStates[cId] = true;
                mRooms[rid]        = r;

                mConnectedClients[cId].roomId = rid;
                mConnectedClients[cId].status = "Room #" + std::to_string(rid) + " (Host)";

                logTraffic("[C" + std::to_string(cId) + "] Created Room #" + std::to_string(rid));
                broadcastRoomSync(rid, now, "Room created. Waiting for players...");
                broadcastLobbySync(now);
                break;
            }
            case PacketType::RoomJoin:
            {
                if (const auto* j = pkt.getIf<PacketRoomJoin>())
                {
                    int rid = j->roomId;
                    int cId = resolveClientId(j->clientId, pkt.srcAddress);

                    if (!mConnectedClients.contains(cId))
                    {
                        cId                    = mNextClientId++;
                        std::string pName      = "Player " + std::to_string(cId + 1);
                        mConnectedClients[cId] = ConnectedClient{cId, pName, -1, 20, "Lobby (Idle)", pkt.srcAddress};
                        logTraffic("[C" + std::to_string(cId) + "] " + pName + " auto-registered on RoomJoin");
                        Packet ackPkt;
                        ackPkt.type = PacketType::ConnectAck;
                        ackPkt.data = PacketConnectAck{cId, true};
                        sendPacketToClient(cId, std::move(ackPkt), now);
                    }

                    if (cId < 0 || !mConnectedClients.contains(cId))
                    {
                        break;
                    }

                    if (mRooms.contains(rid) && mRooms[rid].memberClientIds.size() < 3 && !mRooms[rid].inGame)
                    {
                        int oldRid = mConnectedClients[cId].roomId;
                        if (oldRid != -1 && oldRid != rid && mRooms.contains(oldRid))
                        {
                            leaveCurrentRoom(cId, "Room dissolved: Host joined another room.", now);
                        }

                        if (!mRooms.contains(rid) || mRooms[rid].memberClientIds.size() >= 3 || mRooms[rid].inGame)
                        {
                            break;
                        }

                        if (std::ranges::find(mRooms[rid].memberClientIds, cId) == mRooms[rid].memberClientIds.end())
                        {
                            mRooms[rid].memberClientIds.push_back(cId);
                            mRooms[rid].readyStates[cId] = false;
                        }

                        mConnectedClients[cId].roomId = rid;
                        mConnectedClients[cId].status = "Room #" + std::to_string(rid) + " (Member)";

                        logTraffic("[C" + std::to_string(cId) + "] Joined Room #" + std::to_string(rid));
                        broadcastRoomSync(rid, now, "New member joined room.");
                        broadcastLobbySync(now);
                    }
                }
                break;
            }
            case PacketType::RoomLeave:
            {
                if (const auto* l = pkt.getIf<PacketRoomLeave>())
                {
                    int cId = resolveClientId(l->clientId, pkt.srcAddress);
                    if (cId < 0)
                    {
                        break;
                    }
                    std::string cName = mConnectedClients.contains(cId) ? mConnectedClients[cId].name
                                                                        : ("Player " + std::to_string(cId + 1));
                    logTraffic("[C" + std::to_string(cId) + "] (" + cName + ") requested RoomLeave");
                    std::string rReason = l->reason.empty() ? ("Room dissolved: " + cName + " left the room.") : l->reason;
                    leaveCurrentRoom(cId, rReason, now);
                }
                break;
            }
            case PacketType::RoomInvite:
            {
                if (const auto* inv = pkt.getIf<PacketRoomInvite>())
                {
                    int hostId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (hostId < 0)
                    {
                        break;
                    }
                    int rid    = inv->roomId;
                    int target = inv->targetClientId;
                    if (target < 0 || !mConnectedClients.contains(target) || !mRooms.contains(rid) ||
                        mRooms[rid].hostClientId != hostId || mRooms[rid].inGame)
                    {
                        break;
                    }
                    logTraffic("[C" + std::to_string(hostId) + "] Invite C" + std::to_string(target) + " to Room #" +
                               std::to_string(rid));

                    std::string hName = mConnectedClients[hostId].name;

                    Packet invNotify;
                    invNotify.type = PacketType::RoomInviteNotify;
                    invNotify.data = PacketRoomInviteNotify{rid, hostId, hName};
                    sendPacketToClient(target, std::move(invNotify), now);
                }
                break;
            }
            case PacketType::RoomInviteResponse:
            {
                if (const auto* res = pkt.getIf<PacketRoomInviteResponse>())
                {
                    int rid    = res->roomId;
                    int target = res->targetClientId;
                    if (target < 0 || !mConnectedClients.contains(target))
                    {
                        break;
                    }
                    int senderId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (senderId != target)
                    {
                        break;
                    }
                    if (res->accepted)
                    {
                        if (mRooms.contains(rid) && mRooms[rid].memberClientIds.size() < 3 && !mRooms[rid].inGame)
                        {
                            if (mConnectedClients.contains(target))
                            {
                                int oldRid = mConnectedClients[target].roomId;
                                if (oldRid != -1 && oldRid != rid && mRooms.contains(oldRid))
                                {
                                    leaveCurrentRoom(target, "Room dissolved: Host accepted another room invite.", now);
                                }
                            }

                            if (!mRooms.contains(rid) || mRooms[rid].memberClientIds.size() >= 3 || mRooms[rid].inGame)
                            {
                                break;
                            }

                            if (std::ranges::find(mRooms[rid].memberClientIds, target) == mRooms[rid].memberClientIds.end())
                            {
                                mRooms[rid].memberClientIds.push_back(target);
                                mRooms[rid].readyStates[target] = false;
                            }

                            if (mConnectedClients.contains(target))
                            {
                                mConnectedClients[target].roomId = rid;
                                mConnectedClients[target].status = "Room #" + std::to_string(rid) + " (Member)";
                            }

                            logTraffic("[C" + std::to_string(target) + "] Accepted invite to Room #" + std::to_string(rid));
                            broadcastRoomSync(rid, now, "New member joined room.");
                            broadcastLobbySync(now);
                        }
                    }
                    else
                    {
                        logTraffic("[C" + std::to_string(target) + "] Declined invite to Room #" + std::to_string(rid));
                    }
                }
                break;
            }
            case PacketType::RoomKick:
            {
                if (const auto* kick = pkt.getIf<PacketRoomKick>())
                {
                    int hostId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (hostId < 0)
                    {
                        break;
                    }
                    int rid = kick->roomId;
                    if (mRooms.contains(rid) && mRooms[rid].hostClientId == hostId && !mRooms[rid].inGame)
                    {
                        int target = kick->targetClientId;
                        if (target >= 0 && target != hostId)
                        {
                            std::string tName = "Player " + std::to_string(target + 1);
                            if (mConnectedClients.contains(target))
                            {
                                tName                            = mConnectedClients[target].name;
                                mConnectedClients[target].roomId = -1;
                                mConnectedClients[target].status = "Lobby (Idle)";
                            }

                            logTraffic("[C" + std::to_string(hostId) + "] Kicked C" + std::to_string(target) +
                                       " from Room #" + std::to_string(rid));

                            auto& members = mRooms[rid].memberClientIds;
                            members.erase(std::remove(members.begin(), members.end(), target), members.end());
                            mRooms[rid].readyStates.erase(target);

                            Packet leavePkt;
                            leavePkt.type = PacketType::RoomLeave;
                            leavePkt.data = PacketRoomLeave{rid, target, "You were kicked by the room host."};
                            sendPacketToClient(target, std::move(leavePkt), now);

                            broadcastRoomSync(rid, now, tName + " was kicked by the room host.");

                            broadcastLobbySync(now);
                        }
                    }
                }
                break;
            }
            case PacketType::RoomChangeSettings:
            {
                if (const auto* cs = pkt.getIf<PacketRoomChangeSettings>())
                {
                    int hostId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (hostId < 0)
                    {
                        break;
                    }
                    int rid = cs->roomId;
                    if (mRooms.contains(rid) && mRooms[rid].hostClientId == hostId && !mRooms[rid].inGame)
                    {
                        mRooms[rid].level      = cs->level;
                        mRooms[rid].difficulty = cs->difficulty;
                        mRooms[rid].gameMode   = cs->gameMode;
                        std::string diffName = (cs->difficulty == 0) ? "Easy" : ((cs->difficulty == 1) ? "Normal" : "Hard");
                        std::string modeName = (cs->gameMode == 1) ? "PvP" : "Co-op";
                        logTraffic("[C" + std::to_string(hostId) + "] Room #" + std::to_string(rid) + " setting: Lv." +
                                   std::to_string(cs->level) + " (" + diffName + " | " + modeName + ")");
                        broadcastRoomSync(rid,
                                          now,
                                          "Settings changed to Lv." + std::to_string(cs->level) + " (" + diffName +
                                              " | " + modeName + ")");
                        broadcastLobbySync(now);
                    }
                }
                break;
            }
            case PacketType::Ready:
            {
                if (const auto* r = pkt.getIf<PacketReady>())
                {
                    int cId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (cId < 0)
                    {
                        break;
                    }
                    for (auto& [rid, room] : mRooms)
                    {
                        if (!room.inGame && std::ranges::find(room.memberClientIds, cId) != room.memberClientIds.end())
                        {
                            room.readyStates[cId] = r->isReady;
                            logTraffic("[C" + std::to_string(cId) + "] " + (r->isReady ? "Ready" : "Not ready"));
                            broadcastRoomSync(rid, now);
                            break;
                        }
                    }
                }
                break;
            }
            case PacketType::GameStartRequest:
            {
                if (const auto* startReq = pkt.getIf<PacketGameStartRequest>())
                {
                    int hostId = resolveClientId(pkt.clientId, pkt.srcAddress);
                    if (hostId < 0)
                    {
                        break;
                    }
                    int rid = startReq->roomId;
                    if (mRooms.contains(rid) && mRooms[rid].hostClientId == hostId && !mRooms[rid].inGame)
                    {
                        auto& room = mRooms[rid];
                        if (room.memberClientIds.size() < 2)
                        {
                            broadcastRoomSync(rid, now, "Cannot start: Room needs at least 2 players!");
                            break;
                        }

                        bool allReady = true;
                        for (int mid : room.memberClientIds)
                        {
                            if (mid != room.hostClientId && (!room.readyStates.contains(mid) || !room.readyStates[mid]))
                            {
                                allReady = false;
                                break;
                            }
                        }

                        if (allReady)
                        {
                            room.inGame           = true;
                            room.nextTickToCommit = GameConfig::Game::kCommandDelayTicks;
                            room.pendingTicks.clear();
                            room.clientContiguousTick.clear();
                            room.clientReceivedTicks.clear();
                            room.lastAckSentToClient.clear();
                            room.committedTicks.clear();
                            room.clientAckedServerTick.clear();
                            room.lastSyncSentTimeUs.clear();
                            room.lastSyncSentTick.clear();

                            const int64_t initialTick = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
                            for (int mid : room.memberClientIds)
                            {
                                room.clientContiguousTick[mid]  = initialTick;
                                room.lastAckSentToClient[mid]   = initialTick;
                                room.clientAckedServerTick[mid] = initialTick;
                                room.lastSyncSentTimeUs[mid]    = 0;
                                room.lastSyncSentTick[mid]      = room.nextTickToCommit;
                            }

                            logTraffic(">> START MATCH ROOM #" + std::to_string(rid) + " (" +
                                       std::to_string(room.memberClientIds.size()) + " players) <<");

                            for (size_t i = 0; i < room.memberClientIds.size(); ++i)
                            {
                                int mid = room.memberClientIds[i];
                                if (mConnectedClients.contains(mid))
                                {
                                    mConnectedClients[mid].status = "Combat";
                                }

                                Packet startPkt;
                                startPkt.type = PacketType::GameStart;
                                startPkt.data = PacketGameStart{1337,
                                                                room.level,
                                                                room.difficulty,
                                                                static_cast<int>(room.memberClientIds.size()),
                                                                static_cast<int>(i),
                                                                room.gameMode};
                                sendPacketToClient(mid, std::move(startPkt), now);
                            }

                            broadcastLobbySync(now);
                        }
                        else
                        {
                            broadcastRoomSync(rid, now, "Cannot start: members are not ready!");
                        }
                    }
                }
                break;
            }
            case PacketType::CommandSync:
            {
                int cId = resolveClientId(pkt.clientId, pkt.srcAddress);
                if (cId >= 0 && mConnectedClients.contains(cId))
                {
                    int rId = mConnectedClients[cId].roomId;
                    if (rId == -1 || !mRooms.contains(rId))
                    {
                        for (const auto& [id, room] : mRooms)
                        {
                            if (std::ranges::find(room.memberClientIds, cId) != room.memberClientIds.end())
                            {
                                rId = id;
                                break;
                            }
                        }
                    }
                    if (rId != -1 && mRooms.contains(rId) && mRooms[rId].inGame)
                    {
                        auto&         room        = mRooms[rId];
                        const int64_t initialTick = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
                        if (!room.clientContiguousTick.contains(cId))
                        {
                            room.clientContiguousTick[cId] = initialTick;
                        }
                        if (!room.lastAckSentToClient.contains(cId))
                        {
                            room.lastAckSentToClient[cId] = initialTick;
                        }
                        if (!room.clientAckedServerTick.contains(cId))
                        {
                            room.clientAckedServerTick[cId] = initialTick;
                        }

                        int64_t prevContAck        = room.clientContiguousTick[cId];
                        bool    hadIncomingHistory = false;

                        auto processClientCot = [&](const CommandOnTick& cot)
                        {
                            uint64_t t = static_cast<uint64_t>(cot.tick);

                            if (static_cast<int64_t>(t) > room.clientContiguousTick[cId])
                            {
                                room.clientReceivedTicks[cId].insert(t);
                            }

                            if (t < room.nextTickToCommit)
                            {
                                return;
                            }

                            auto& tickData = room.pendingTicks[t];
                            if (tickData.clientsReported.contains(cId))
                            {
                                return;
                            }

                            tickData.clientsReported.insert(cId);
                            tickData.clientCommands[cId] = cot.commands;
                        };

                        if (const auto* cot = pkt.getIf<CommandOnTick>())
                        {
                            hadIncomingHistory = true;
                            processClientCot(*cot);
                        }
                        else if (const auto* sync = pkt.getIf<PacketCommandSync>())
                        {
                            if (sync->ackTick > room.clientAckedServerTick[cId])
                            {
                                room.clientAckedServerTick[cId] = sync->ackTick;
                            }

                            hadIncomingHistory = !sync->history.empty();
                            for (const auto& item : sync->history)
                            {
                                processClientCot(item);
                            }
                        }

                        auto& recSet  = room.clientReceivedTicks[cId];
                        auto& contAck = room.clientContiguousTick[cId];
                        while (true)
                        {
                            uint64_t nextExpected = static_cast<uint64_t>(contAck + 1);
                            auto     it           = recSet.find(nextExpected);
                            if (it == recSet.end())
                            {
                                break;
                            }
                            recSet.erase(it);
                            contAck = static_cast<int64_t>(nextExpected);
                        }

                        if (hadIncomingHistory && contAck == prevContAck && room.lastAckSentToClient[cId] >= contAck)
                        {
                            room.lastAckSentToClient[cId] = contAck - 1;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    for (auto& [rid, room] : mRooms)
    {
        if (!room.inGame || room.memberClientIds.empty())
        {
            continue;
        }

        int requiredClients = static_cast<int>(room.memberClientIds.size());

        while (!room.pendingTicks.empty() && room.pendingTicks.begin()->first < room.nextTickToCommit)
        {
            room.pendingTicks.erase(room.pendingTicks.begin());
        }

        bool hasNewlyCommitted = false;

        while (true)
        {
            auto it = room.pendingTicks.find(room.nextTickToCommit);
            if (it == room.pendingTicks.end())
            {
                break;
            }

            auto& tickData = it->second;
            if (static_cast<int>(tickData.clientsReported.size()) < requiredClients)
            {
                break;
            }

            CommandOnTick combined;
            combined.tick = room.nextTickToCommit;

            for (auto& [cId, cmds] : tickData.clientCommands)
            {
                for (auto& cmd : cmds)
                {
                    combined.commands.push_back(std::move(cmd));
                }
            }

            room.committedTicks[room.nextTickToCommit] = std::move(combined);
            room.pendingTicks.erase(it);
            room.nextTickToCommit++;
            hasNewlyCommitted = true;
        }

        const int64_t defaultAck   = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
        int64_t       minClientAck = std::numeric_limits<int64_t>::max();
        for (int cId : room.memberClientIds)
        {
            int64_t cAck = room.clientAckedServerTick.contains(cId) ? room.clientAckedServerTick[cId] : defaultAck;
            minClientAck = std::min(minClientAck, cAck);
        }
        for (auto it = room.committedTicks.begin(); it != room.committedTicks.end();)
        {
            if (static_cast<int64_t>(it->first) <= minClientAck)
            {
                it = room.committedTicks.erase(it);
            }
            else
            {
                break;
            }
        }

        for (int cId : room.memberClientIds)
        {
            int64_t currentAck = room.clientContiguousTick.contains(cId) ? room.clientContiguousTick[cId] : defaultAck;
            int64_t lastAck    = room.lastAckSentToClient.contains(cId) ? room.lastAckSentToClient[cId] : defaultAck;
            int64_t clientAcked = room.clientAckedServerTick.contains(cId) ? room.clientAckedServerTick[cId] : defaultAck;
            uint64_t lastSentUs   = room.lastSyncSentTimeUs.contains(cId) ? room.lastSyncSentTimeUs[cId] : 0;
            uint64_t lastSentTick = room.lastSyncSentTick.contains(cId) ? room.lastSyncSentTick[cId] : 0;

            bool hasUnackedCommitted = !room.committedTicks.empty() &&
                                       static_cast<int64_t>(room.committedTicks.rbegin()->first) > clientAcked;
            bool hasDataToSend       = hasNewlyCommitted || hasUnackedCommitted || (currentAck > lastAck);

            bool isTickDue   = (room.nextTickToCommit >= lastSentTick + 2) || (room.nextTickToCommit < lastSentTick);
            bool isTimeDue   = (lastSentUs == 0) || (now >= lastSentUs + GameConfig::Game::kNetSyncIntervalUs);
            bool isDueToSend = isTickDue || isTimeDue;

            if (hasDataToSend && isDueToSend)
            {
                PacketCommandSync syncOut;
                syncOut.ackTick = currentAck;

                uint64_t latestCommittedTick = 0;
                for (const auto& [t, cot] : room.committedTicks)
                {
                    if (static_cast<int64_t>(t) > clientAcked)
                    {
                        syncOut.history.push_back(cot);
                        latestCommittedTick = t;
                    }
                }

                Packet broadcastPkt;
                broadcastPkt.type = PacketType::CommandSync;
                broadcastPkt.tick = syncOut.history.empty() ? static_cast<uint64_t>(std::max<int64_t>(0, currentAck))
                                                            : latestCommittedTick;
                broadcastPkt.data = std::move(syncOut);
                sendPacketToClient(cId, std::move(broadcastPkt), now);

                room.lastAckSentToClient[cId] = currentAck;
                room.lastSyncSentTimeUs[cId]  = now;
                room.lastSyncSentTick[cId]    = room.nextTickToCommit;
            }
        }
    }
}

void Server::render()
{
    if (!mGuiEnabled || !mWindow || !mWindow->isOpen())
    {
        return;
    }

    mWindow->clear(sf::Color(10, 14, 22));

    sf::View view(sf::FloatRect({0.f, 0.f}, {600.f, 380.f}));
    mWindow->setView(view);

    sf::Text title(mFont, "SERVER DASHBOARD - NETWORK COORDINATOR", 15);
    title.setFillColor(sf::Color(0, 220, 255));
    title.setPosition({15.f, 10.f});
    mWindow->draw(title);

    sf::RectangleShape clientBox({280.f, 140.f});
    clientBox.setPosition({15.f, 50.f});
    clientBox.setFillColor(sf::Color(18, 24, 36));
    clientBox.setOutlineColor(sf::Color(45, 65, 95));
    clientBox.setOutlineThickness(1.f);
    mWindow->draw(clientBox);

    sf::Text cTitle(mFont, "CONNECTED CLIENTS (" + std::to_string(mConnectedClients.size()) + ")", 12);
    cTitle.setFillColor(sf::Color(120, 200, 255));
    cTitle.setPosition({25.f, 56.f});
    mWindow->draw(cTitle);

    float cy = 80.f;
    for (const auto& [id, client] : mConnectedClients)
    {
        sf::Color       dotColor = (id == 0) ? sf::Color(0, 220, 255)
                                             : ((id == 1) ? sf::Color(255, 75, 75) : sf::Color(190, 90, 255));
        sf::CircleShape dot(4.f);
        dot.setFillColor(dotColor);
        dot.setPosition({25.f, cy + 4.f});
        mWindow->draw(dot);

        std::string addrStr = client.networkAddress
                                  ? "@" + std::to_string(reinterpret_cast<uintptr_t>(client.networkAddress)).substr(0, 6)
                                  : "@null";
        sf::Text line(mFont, "C" + std::to_string(id) + " " + addrStr + ": " + client.name + " [" + client.status + "]", 10);
        line.setFillColor(sf::Color(210, 225, 240));
        line.setPosition({40.f, cy});
        mWindow->draw(line);
        cy += 24.f;
    }

    sf::RectangleShape roomBox({280.f, 160.f});
    roomBox.setPosition({15.f, 205.f});
    roomBox.setFillColor(sf::Color(18, 24, 36));
    roomBox.setOutlineColor(sf::Color(45, 65, 95));
    roomBox.setOutlineThickness(1.f);
    mWindow->draw(roomBox);

    sf::Text rTitle(mFont, "ACTIVE ROOMS (" + std::to_string(mRooms.size()) + ")", 12);
    rTitle.setFillColor(sf::Color(255, 180, 80));
    rTitle.setPosition({25.f, 212.f});
    mWindow->draw(rTitle);

    float ry = 238.f;
    if (mRooms.empty())
    {
        sf::Text emptyText(mFont, "(No active rooms)", 11);
        emptyText.setFillColor(sf::Color(140, 150, 170));
        emptyText.setPosition({30.f, ry});
        mWindow->draw(emptyText);
    }
    else
    {
        for (const auto& [id, room] : mRooms)
        {
            std::string rStatus = room.inGame ? "[PLAYING]" : "[OPEN]";
            sf::Color   sColor  = room.inGame ? sf::Color(255, 90, 90) : sf::Color(80, 255, 140);

            sf::Text rInfo(mFont,
                           "Room #" + std::to_string(room.id) + " (" + std::to_string(room.memberClientIds.size()) +
                               "/3) " + rStatus,
                           11);
            rInfo.setFillColor(sColor);
            rInfo.setPosition({30.f, ry});
            mWindow->draw(rInfo);
            ry += 22.f;
        }
    }

    sf::RectangleShape logBox({275.f, 315.f});
    logBox.setPosition({310.f, 50.f});
    logBox.setFillColor(sf::Color(14, 18, 28));
    logBox.setOutlineColor(sf::Color(40, 60, 85));
    logBox.setOutlineThickness(1.f);
    mWindow->draw(logBox);

    sf::Text lTitle(mFont, "PACKET TRAFFIC MONITOR", 12);
    lTitle.setFillColor(sf::Color(100, 255, 180));
    lTitle.setPosition({320.f, 56.f});
    mWindow->draw(lTitle);

    float ly = 82.f;
    for (const auto& logMsg : mPacketLogs)
    {
        sf::Text line(mFont, "> " + logMsg, 10);
        line.setFillColor(sf::Color(180, 215, 235));
        line.setPosition({320.f, ly});
        mWindow->draw(line);
        ly += 20.f;
    }

    mWindow->display();
}

void Server::run()
{
    bool isIdle = mNetwork.isRunning() && !mNetwork.isAwake() && mConnectedClients.empty() &&
                  mNetwork.ingoingPacket.empty();

    if (isIdle)
    {
        if (mGuiEnabled && mWindow && mWindow->isOpen())
        {
            input();
            if (mGuiEnabled && mWindow && mWindow->isOpen())
            {
                render();
            }
            mNetwork.waitForWakeup(30);
        }
        else
        {
            logout("Server sleeping", false);

            mNetwork.waitForWakeup(3'600'000);
        }
        return;
    }

    auto frameStart = std::chrono::steady_clock::now();

    input();
    uint64_t now = SimulationClock::nowUs();
    network(now);
    update(now);
    mNetwork.sync();

    if (mGuiEnabled && mWindow && mWindow->isOpen())
    {
        render();
    }

    const auto targetDuration = std::chrono::microseconds(1000000 / std::max(1u, mTargetFps));
    auto       frameEnd       = std::chrono::steady_clock::now();
    auto       elapsed        = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
    if (elapsed < targetDuration)
    {
        std::this_thread::sleep_for(targetDuration - elapsed);
    }
}
