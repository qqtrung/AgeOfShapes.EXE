#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-enum-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#define ENET_IMPLEMENTATION
#include "enet.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "Debug.hpp"
#include "ENetTransport.hpp"
#include "PacketSerializer.hpp"

#include <iostream>
#include <vector>

#include <cstring>


static bool sENetInitialized = false;

bool ENetSystem::initialize()
{
    if (sENetInitialized)
    {
        return true;
    }
    if (enet_initialize() != 0)
    {
        logout("[ENet] Failed to initialize ENet!", true);
        return false;
    }
    sENetInitialized = true;
    return true;
}

void ENetSystem::deinitialize()
{
    if (sENetInitialized)
    {
        enet_deinitialize();
        sENetInitialized = false;
    }
}

bool ENetSystem::isInitialized()
{
    return sENetInitialized;
}


ENetServer::ENetServer() = default;

ENetServer::~ENetServer()
{
    stop();
}

bool ENetServer::start(uint16_t port, size_t maxClients, uint32_t targetHz)
{
    stop();

    if (!ENetSystem::initialize())
    {
        logout("[ENetServer] Failed to initialize ENet system!", true);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);
        mIncomingStaging.clear();
        mOutgoingStaging.clear();
        mClientRtt.clear();
    }

    mRunning.store(true);
    mThread = std::thread(&ENetServer::threadLoop, this, port, maxClients, targetHz);
    return true;
}

void ENetServer::stop()
{
    if (mRunning.exchange(false))
    {
        mWakeupCv.notify_all();
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mIncomingStaging.clear();
    mOutgoingStaging.clear();
    mClientRtt.clear();
}

bool ENetServer::isRunning() const
{
    return mRunning.load(std::memory_order_relaxed);
}

bool ENetServer::isAwake() const
{
    if (mIsAwake.load(std::memory_order_acquire))
    {
        return true;
    }
    std::lock_guard<std::mutex> lock(mMutex);
    return !mIncomingStaging.empty();
}

void ENetServer::waitForWakeup(uint32_t timeoutMs)
{
    if (isAwake())
    {
        return;
    }
    std::unique_lock<std::mutex> lock(mWakeupMutex);
    mWakeupCv.wait_for(lock,
                       std::chrono::milliseconds(timeoutMs),
                       [this]
                       { return mIsAwake.load(std::memory_order_acquire) || !mRunning.load(std::memory_order_relaxed); });
}

void ENetServer::sync(Network& serverNetwork)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mIncomingStaging.empty())
    {
        serverNetwork.ingoingPacket.splice(serverNetwork.ingoingPacket.end(), mIncomingStaging);
    }

    if (!serverNetwork.outgoingPacket.empty())
    {
        mOutgoingStaging.splice(mOutgoingStaging.end(), serverNetwork.outgoingPacket);
    }
}

uint32_t ENetServer::getClientRtt(int clientId) const
{
    std::lock_guard<std::mutex> lock(mMutex);
    auto                        it = mClientRtt.find(clientId);
    return (it != mClientRtt.end()) ? it->second : 0;
}

void ENetServer::threadLoop(uint16_t port, size_t maxClients, uint32_t targetHz)
{
    ENetAddress address;
    address.host          = ENET_HOST_ANY;
    address.port          = port;
    address.sin6_scope_id = 0;

    ENetHost* host = enet_host_create(&address, maxClients, 3, 0, 0);
    if (!host)
    {
        logout("[ENetServer] Failed to create ENet server host on port " + std::to_string(port) + "!", true);
        mRunning.store(false, std::memory_order_relaxed);
        return;
    }

    logout("[ENetServer] Started background server network thread on port " + std::to_string(port) + " at " +
           std::to_string(targetHz) + " Hz...");

    std::unordered_set<ENetPeer*>      connectedPeers;
    std::unordered_map<int, ENetPeer*> clientIdToPeer;
    std::unordered_map<ENetPeer*, int> peerToClientId;

    const auto        frameDuration = std::chrono::microseconds(1000000 / std::max(1u, targetHz));
    std::list<Packet> outgoingQueue;
    std::list<Packet> incomingBatch;

    auto handleEvent = [&](ENetEvent& event)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                if (peerToClientId.contains(event.peer))
                {
                    int oldId = peerToClientId[event.peer];
                    clientIdToPeer.erase(oldId);
                    peerToClientId.erase(event.peer);
                }
                connectedPeers.insert(event.peer);
                enet_peer_set_data(event.peer, nullptr);

                enet_peer_ping_interval(event.peer, 500);
                enet_peer_timeout(event.peer, 32, 2000, 5000);

                char ipStr[64] = {0};
                enet_address_get_host_ip(&event.peer->address, ipStr, sizeof(ipStr));
                logout("[ENetServer] Peer connected from " + std::string(ipStr) + ":" +
                       std::to_string(event.peer->address.port));
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                Packet pkt;
                if (NetSer::deserializePacket(event.packet->data, event.packet->dataLength, pkt))
                {
                    pkt.srcAddress  = reinterpret_cast<Network*>(event.peer);
                    pkt.destAddress = nullptr;
                    if (peerToClientId.contains(event.peer))
                    {
                        pkt.clientId = peerToClientId[event.peer];
                    }
                    else
                    {
                        pkt.clientId = -1;
                    }
                    incomingBatch.push_back(std::move(pkt));
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                connectedPeers.erase(event.peer);

                int cId = -1;
                if (peerToClientId.contains(event.peer))
                {
                    cId = peerToClientId[event.peer];
                }
                else
                {
                    void* data = enet_peer_get_data(event.peer);
                    if (data)
                    {
                        cId = static_cast<int>(reinterpret_cast<intptr_t>(data)) - 1;
                    }
                }
                if (cId != -1)
                {
                    clientIdToPeer.erase(cId);
                }
                peerToClientId.erase(event.peer);
                enet_peer_set_data(event.peer, nullptr);

                char ipStr[64] = {0};
                enet_address_get_host_ip(&event.peer->address, ipStr, sizeof(ipStr));
                logout("[ENetServer] Peer (C" + std::to_string(cId) + ") disconnected cleanly from " +
                       std::string(ipStr) + ":" + std::to_string(event.peer->address.port));

                Packet disc;
                disc.type        = PacketType::Disconnect;
                disc.data        = PacketDisconnect{cId, "Client disconnected cleanly"};
                disc.clientId    = cId;
                disc.srcAddress  = reinterpret_cast<Network*>(event.peer);
                disc.destAddress = nullptr;
                incomingBatch.push_back(std::move(disc));
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            {
                connectedPeers.erase(event.peer);

                int cId = -1;
                if (peerToClientId.contains(event.peer))
                {
                    cId = peerToClientId[event.peer];
                }
                else
                {
                    void* data = enet_peer_get_data(event.peer);
                    if (data)
                    {
                        cId = static_cast<int>(reinterpret_cast<intptr_t>(data)) - 1;
                    }
                }
                if (cId != -1)
                {
                    clientIdToPeer.erase(cId);
                }
                peerToClientId.erase(event.peer);
                enet_peer_set_data(event.peer, nullptr);

                char ipStr[64] = {0};
                enet_address_get_host_ip(&event.peer->address, ipStr, sizeof(ipStr));
                logout("[ENetServer] Peer (C" + std::to_string(cId) + ") TIMED OUT from " + std::string(ipStr) + ":" +
                       std::to_string(event.peer->address.port) + " (connection lost)");

                Packet disc;
                disc.type        = PacketType::Disconnect;
                disc.data        = PacketDisconnect{cId, "Client timed out (connection lost)"};
                disc.clientId    = cId;
                disc.srcAddress  = reinterpret_cast<Network*>(event.peer);
                disc.destAddress = nullptr;
                incomingBatch.push_back(std::move(disc));
                break;
            }

            default:
                break;
        }
    };

    bool hadIncoming = false;

    while (mRunning.load(std::memory_order_relaxed))
    {
        if (connectedPeers.empty())
        {
            {
                std::lock_guard<std::mutex> lock(mWakeupMutex);
                mIsAwake.store(false, std::memory_order_release);
            }

            logout("[ENetServer] No clients connected, sleeping network thread until a client connects...");

            while (mRunning.load(std::memory_order_relaxed) && connectedPeers.empty())
            {
                ENetEvent event;
                if (enet_host_service(host, &event, 3'600'000) > 0)
                {
                    handleEvent(event);
                    {
                        std::lock_guard<std::mutex> lock(mWakeupMutex);
                        mIsAwake.store(true, std::memory_order_release);
                    }
                    mWakeupCv.notify_all();
                    break;
                }
            }

            logout("[ENetServer] Network thread woke up from sleep, resuming normal operation...");

            if (!mRunning.load(std::memory_order_relaxed))
            {
                break;
            }
        }

        auto frameStart = std::chrono::steady_clock::now();

        bool hadOutgoing = false;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (!mOutgoingStaging.empty())
            {
                outgoingQueue.splice(outgoingQueue.end(), mOutgoingStaging);
                hadOutgoing = true;
            }
        }

        while (!outgoingQueue.empty())
        {
            Packet pkt = std::move(outgoingQueue.front());
            outgoingQueue.pop_front();

            ENetPeer* targetPeer = nullptr;
            if (pkt.destAddress && connectedPeers.contains(reinterpret_cast<ENetPeer*>(pkt.destAddress)))
            {
                targetPeer = reinterpret_cast<ENetPeer*>(pkt.destAddress);
            }
            else if (clientIdToPeer.contains(pkt.clientId))
            {
                targetPeer = clientIdToPeer[pkt.clientId];
            }

            if (!targetPeer || targetPeer->state != ENET_PEER_STATE_CONNECTED)
            {
                continue;
            }

            if (pkt.type != PacketType::ConnectAck && pkt.clientId >= 0)
            {
                if (!peerToClientId.contains(targetPeer) || peerToClientId[targetPeer] != pkt.clientId)
                {
                    continue;
                }
            }

            const bool  isUnreliableCmd = (pkt.type == PacketType::CommandSync);
            enet_uint32 pktFlags        = isUnreliableCmd ? 0 : ENET_PACKET_FLAG_RELIABLE;
            enet_uint8  channelId       = isUnreliableCmd ? 2 : 1;

            std::vector<uint8_t> bytes   = NetSer::serializePacket(pkt);
            ENetPacket*          enetPkt = enet_packet_create(bytes.data(), bytes.size(), pktFlags);
            if (enetPkt)
            {
                if (enet_peer_send(targetPeer, channelId, enetPkt) < 0)
                {
                    enet_packet_destroy(enetPkt);
                }
            }

            if (pkt.clientId >= 0 && targetPeer && pkt.type == PacketType::ConnectAck)
            {
                if (peerToClientId.contains(targetPeer) && peerToClientId[targetPeer] != pkt.clientId)
                {
                    clientIdToPeer.erase(peerToClientId[targetPeer]);
                }
                if (clientIdToPeer.contains(pkt.clientId) && clientIdToPeer[pkt.clientId] != targetPeer)
                {
                    peerToClientId.erase(clientIdToPeer[pkt.clientId]);
                }

                enet_peer_set_data(targetPeer, reinterpret_cast<void*>(static_cast<intptr_t>(pkt.clientId + 1)));
                clientIdToPeer[pkt.clientId] = targetPeer;
                peerToClientId[targetPeer]   = pkt.clientId;
            }
        }

        enet_host_flush(host);

        auto        frameNow = std::chrono::steady_clock::now();
        auto        elapsed  = std::chrono::duration_cast<std::chrono::microseconds>(frameNow - frameStart);
        enet_uint32 waitMs   = 0;
        if (hadOutgoing || hadIncoming)
        {
            waitMs = 0;
        }
        else if (elapsed < frameDuration)
        {
            waitMs = static_cast<enet_uint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(frameDuration - elapsed).count());
        }

        hadIncoming = false;
        ENetEvent event;
        if (enet_host_service(host, &event, waitMs) > 0)
        {
            do
            {
                handleEvent(event);
            } while (enet_host_service(host, &event, 0) > 0);
        }

        std::vector<ENetPeer*> deadPeers;
        for (auto* peer : connectedPeers)
        {
            if (peer->state == ENET_PEER_STATE_DISCONNECTED || peer->state == ENET_PEER_STATE_ZOMBIE)
            {
                deadPeers.push_back(peer);
            }
        }
        for (auto* peer : deadPeers)
        {
            connectedPeers.erase(peer);

            int cId = -1;
            if (peerToClientId.contains(peer))
            {
                cId = peerToClientId[peer];
            }
            else
            {
                void* data = enet_peer_get_data(peer);
                if (data)
                {
                    cId = static_cast<int>(reinterpret_cast<intptr_t>(data)) - 1;
                }
            }
            if (cId != -1)
            {
                clientIdToPeer.erase(cId);
            }
            peerToClientId.erase(peer);
            enet_peer_set_data(peer, nullptr);

            char ipStr[64] = {0};
            enet_address_get_host_ip(&peer->address, ipStr, sizeof(ipStr));
            logout("[ENetServer] Peer (C" + std::to_string(cId) + ") detected as disconnected/zombie at " +
                   std::string(ipStr) + ":" + std::to_string(peer->address.port));

            Packet disc;
            disc.type        = PacketType::Disconnect;
            disc.data        = PacketDisconnect{cId, "Client connection lost"};
            disc.clientId    = cId;
            disc.srcAddress  = reinterpret_cast<Network*>(peer);
            disc.destAddress = nullptr;
            incomingBatch.push_back(std::move(disc));
        }

        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (!incomingBatch.empty())
            {
                hadIncoming = true;
                mIncomingStaging.splice(mIncomingStaging.end(), incomingBatch);
            }
            mClientRtt.clear();
            for (const auto& [cId, peer] : clientIdToPeer)
            {
                if (peer && peer->state == ENET_PEER_STATE_CONNECTED)
                {
                    mClientRtt[cId] = peer->roundTripTime;
                }
            }
        }
        if (hadIncoming)
        {
            mWakeupCv.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mOutgoingStaging.empty())
        {
            outgoingQueue.splice(outgoingQueue.end(), mOutgoingStaging);
        }
    }
    while (!outgoingQueue.empty())
    {
        Packet pkt = std::move(outgoingQueue.front());
        outgoingQueue.pop_front();

        ENetPeer* targetPeer = nullptr;
        if (pkt.destAddress && connectedPeers.contains(reinterpret_cast<ENetPeer*>(pkt.destAddress)))
        {
            targetPeer = reinterpret_cast<ENetPeer*>(pkt.destAddress);
        }
        else if (clientIdToPeer.contains(pkt.clientId))
        {
            targetPeer = clientIdToPeer[pkt.clientId];
        }

        if (targetPeer && targetPeer->state == ENET_PEER_STATE_CONNECTED)
        {
            std::vector<uint8_t> bytes   = NetSer::serializePacket(pkt);
            ENetPacket*          enetPkt = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
            if (enetPkt)
            {
                if (enet_peer_send(targetPeer, 1, enetPkt) < 0)
                {
                    enet_packet_destroy(enetPkt);
                }
            }
        }
    }
    enet_host_flush(host);

    for (auto* peer : connectedPeers)
    {
        enet_peer_disconnect_now(peer, 0);
    }
    connectedPeers.clear();
    clientIdToPeer.clear();
    peerToClientId.clear();

    enet_host_destroy(host);
    logout("[ENetServer] Background server network thread stopped cleanly.");
}


ENetClient::ENetClient() = default;

ENetClient::~ENetClient()
{
    stop();
}

bool ENetClient::start(const std::string& host, uint16_t port, uint32_t targetHz)
{
    stop();

    if (!ENetSystem::initialize())
    {
        logout("[ENetClient] Failed to initialize ENet system!", true);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);
        mIncomingStaging.clear();
        mOutgoingStaging.clear();
    }

    mRtt.store(0, std::memory_order_relaxed);
    mRunning.store(true);
    mThread = std::thread(&ENetClient::threadLoop, this, host, port, targetHz);
    return true;
}

void ENetClient::stop()
{
    mRtt.store(0, std::memory_order_relaxed);
    if (mRunning.exchange(false))
    {
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mIncomingStaging.clear();
    mOutgoingStaging.clear();
    mRtt.store(0, std::memory_order_relaxed);
}

bool ENetClient::isRunning() const
{
    return mRunning.load(std::memory_order_relaxed);
}

bool ENetClient::isConnected() const
{
    return mIsConnected.load(std::memory_order_relaxed);
}

uint32_t ENetClient::getRtt() const
{
    return mRtt.load(std::memory_order_relaxed);
}

void ENetClient::sync(Network& gameNetwork, std::atomic<bool>& isConnectedFlag)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mIncomingStaging.empty())
    {
        gameNetwork.ingoingPacket.splice(gameNetwork.ingoingPacket.end(), mIncomingStaging);
    }

    if (!gameNetwork.outgoingPacket.empty())
    {
        mOutgoingStaging.splice(mOutgoingStaging.end(), gameNetwork.outgoingPacket);
    }

    isConnectedFlag.store(mIsConnected.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void ENetClient::sync(Network& gameNetwork, bool& isConnectedFlag)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mIncomingStaging.empty())
    {
        gameNetwork.ingoingPacket.splice(gameNetwork.ingoingPacket.end(), mIncomingStaging);
    }

    if (!gameNetwork.outgoingPacket.empty())
    {
        mOutgoingStaging.splice(mOutgoingStaging.end(), gameNetwork.outgoingPacket);
    }

    isConnectedFlag = mIsConnected.load(std::memory_order_relaxed);
}

void ENetClient::threadLoop(std::string host, uint16_t port, uint32_t targetHz)
{
    ENetHost* enetHost = enet_host_create(nullptr, 1, 3, 0, 0);
    if (!enetHost)
    {
        logout("[ENetClient] Failed to create ENet client host!", true);
        mRunning.store(false, std::memory_order_relaxed);
        return;
    }

    logout("[ENetClient] Started background client network thread at " + std::to_string(targetHz) +
           " Hz (target: " + host + ":" + std::to_string(port) + ")...");

    ENetAddress address;
    memset(&address, 0, sizeof(address));
    address.port          = port;
    address.sin6_scope_id = 0;
    if (enet_address_set_host(&address, host.c_str()) != 0)
    {
        logout("[ENetClient] Failed to resolve host address: " + host, true);
        enet_host_destroy(enetHost);
        mRunning.store(false, std::memory_order_relaxed);
        return;
    }

    ENetPeer* serverPeer = enet_host_connect(enetHost, &address, 3, 0);
    if (!serverPeer)
    {
        logout("[ENetClient] Failed to initiate connection to " + host + ":" + std::to_string(port), true);
        enet_host_destroy(enetHost);
        mRunning.store(false, std::memory_order_relaxed);
        return;
    }
    enet_host_flush(enetHost);

    const auto frameDuration     = std::chrono::microseconds(1000000 / std::max(1u, targetHz));
    auto       lastReconnectTime = std::chrono::steady_clock::now();
    bool       connected         = false;
    bool       hadIncoming       = false;

    std::list<Packet> outgoingQueue;
    std::list<Packet> incomingBatch;

    while (mRunning.load(std::memory_order_relaxed))
    {
        auto frameStart = std::chrono::steady_clock::now();

        bool isPeerConnected  = (serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED);
        bool isPeerConnecting = (serverPeer && (serverPeer->state == ENET_PEER_STATE_CONNECTING ||
                                                serverPeer->state == ENET_PEER_STATE_CONNECTION_PENDING ||
                                                serverPeer->state == ENET_PEER_STATE_CONNECTION_SUCCEEDED));

        if (!isPeerConnected && !isPeerConnecting)
        {
            auto nowTime = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastReconnectTime).count() >= 30)
            {
                lastReconnectTime = nowTime;
                logout("[ENetClient] Reconnecting to server " + host + ":" + std::to_string(port) + " (30s retry)...");
                serverPeer = enet_host_connect(enetHost, &address, 3, 0);
                if (serverPeer)
                {
                    enet_host_flush(enetHost);
                }
            }
        }

        bool hadOutgoing = false;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (!mOutgoingStaging.empty())
            {
                outgoingQueue.splice(outgoingQueue.end(), mOutgoingStaging);
                hadOutgoing = true;
            }
        }

        if (connected && serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED)
        {
            while (!outgoingQueue.empty())
            {
                Packet pkt = std::move(outgoingQueue.front());
                outgoingQueue.pop_front();

                const bool  isUnreliableCmd = (pkt.type == PacketType::CommandSync);
                enet_uint32 pktFlags        = isUnreliableCmd ? 0 : ENET_PACKET_FLAG_RELIABLE;
                enet_uint8  channelId       = isUnreliableCmd ? 2 : 1;

                std::vector<uint8_t> bytes   = NetSer::serializePacket(pkt);
                ENetPacket*          enetPkt = enet_packet_create(bytes.data(), bytes.size(), pktFlags);
                if (enetPkt)
                {
                    if (enet_peer_send(serverPeer, channelId, enetPkt) < 0)
                    {
                        enet_packet_destroy(enetPkt);
                    }
                }
            }
        }
        else if (!serverPeer)
        {
            outgoingQueue.clear();
        }

        enet_host_flush(enetHost);

        auto        frameNow = std::chrono::steady_clock::now();
        auto        elapsed  = std::chrono::duration_cast<std::chrono::microseconds>(frameNow - frameStart);
        enet_uint32 waitMs   = 0;
        if (hadOutgoing || hadIncoming)
        {
            waitMs = 0;
        }
        else if (elapsed < frameDuration)
        {
            waitMs = static_cast<enet_uint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(frameDuration - elapsed).count());
        }

        hadIncoming = false;
        ENetEvent event;
        if (enet_host_service(enetHost, &event, waitMs) > 0)
        {
            do
            {
                switch (event.type)
                {
                    case ENET_EVENT_TYPE_CONNECT:
                    {
                        connected = true;
                        enet_peer_ping_interval(serverPeer, 500);
                        enet_peer_timeout(serverPeer, 32, 2000, 5000);
                        logout("[ENetClient] Connected to server successfully.");
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE:
                    {
                        Packet pkt;
                        if (NetSer::deserializePacket(event.packet->data, event.packet->dataLength, pkt))
                        {
                            pkt.srcAddress  = reinterpret_cast<Network*>(serverPeer);
                            pkt.destAddress = nullptr;
                            incomingBatch.push_back(std::move(pkt));
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT:
                    {
                        connected  = false;
                        serverPeer = nullptr;
                        logout("[ENetClient] Server closed connection (graceful disconnect).");

                        Packet disc;
                        disc.type        = PacketType::Disconnect;
                        disc.data        = PacketDisconnect{-1, "Server closed connection"};
                        disc.clientId    = -1;
                        disc.srcAddress  = nullptr;
                        disc.destAddress = nullptr;
                        incomingBatch.push_back(std::move(disc));
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    {
                        connected  = false;
                        serverPeer = nullptr;
                        logout("[ENetClient] Connection to server TIMED OUT (heartbeat lost)!");

                        Packet disc;
                        disc.type        = PacketType::Disconnect;
                        disc.data        = PacketDisconnect{-1, "Connection to server timed out"};
                        disc.clientId    = -1;
                        disc.srcAddress  = nullptr;
                        disc.destAddress = nullptr;
                        incomingBatch.push_back(std::move(disc));
                        break;
                    }

                    default:
                        break;
                }
            } while (enet_host_service(enetHost, &event, 0) > 0);
        }

        if (connected && serverPeer &&
            (serverPeer->state == ENET_PEER_STATE_DISCONNECTED || serverPeer->state == ENET_PEER_STATE_ZOMBIE))
        {
            connected  = false;
            serverPeer = nullptr;
            logout("[ENetClient] Server peer became disconnected/zombie.");

            Packet disc;
            disc.type        = PacketType::Disconnect;
            disc.data        = PacketDisconnect{-1, "Lost connection to server"};
            disc.clientId    = -1;
            disc.srcAddress  = nullptr;
            disc.destAddress = nullptr;
            incomingBatch.push_back(std::move(disc));
        }
        else if (serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED)
        {
            connected = true;
        }

        mIsConnected.store(connected, std::memory_order_relaxed);
        if (connected && serverPeer)
        {
            mRtt.store(serverPeer->roundTripTime, std::memory_order_relaxed);
        }
        else
        {
            mRtt.store(0, std::memory_order_relaxed);
        }

        if (!incomingBatch.empty())
        {
            hadIncoming = true;
            std::lock_guard<std::mutex> lock(mMutex);
            mIncomingStaging.splice(mIncomingStaging.end(), incomingBatch);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mOutgoingStaging.empty())
        {
            outgoingQueue.splice(outgoingQueue.end(), mOutgoingStaging);
        }
    }
    if (connected && serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED)
    {
        while (!outgoingQueue.empty())
        {
            Packet pkt = std::move(outgoingQueue.front());
            outgoingQueue.pop_front();

            const bool  isUnreliableCmd = (pkt.type == PacketType::CommandSync);
            enet_uint32 pktFlags        = isUnreliableCmd ? 0 : ENET_PACKET_FLAG_RELIABLE;
            enet_uint8  channelId       = isUnreliableCmd ? 2 : 1;

            std::vector<uint8_t> bytes   = NetSer::serializePacket(pkt);
            ENetPacket*          enetPkt = enet_packet_create(bytes.data(), bytes.size(), pktFlags);
            if (enetPkt)
            {
                if (enet_peer_send(serverPeer, channelId, enetPkt) < 0)
                {
                    enet_packet_destroy(enetPkt);
                }
            }
        }
        enet_host_flush(enetHost);
    }

    if (serverPeer)
    {
        if (connected)
        {
            enet_peer_disconnect(serverPeer, 0);
            enet_host_flush(enetHost);

            auto      disconnectStart = std::chrono::steady_clock::now();
            ENetEvent event;
            bool      disconnected = false;

            while (enetHost && enet_host_service(enetHost, &event, 50) > 0)
            {
                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                {
                    enet_packet_destroy(event.packet);
                }
                else if (event.type == ENET_EVENT_TYPE_DISCONNECT || event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
                {
                    disconnected = true;
                    break;
                }

                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now() - disconnectStart)
                                     .count();
                if (elapsedMs >= 200)
                {
                    break;
                }
            }

            if (!disconnected)
            {
                enet_peer_reset(serverPeer);
            }
        }
        else
        {
            enet_peer_reset(serverPeer);
        }
        serverPeer = nullptr;
    }

    enet_host_destroy(enetHost);
    mIsConnected.store(false, std::memory_order_relaxed);
    mRtt.store(0, std::memory_order_relaxed);
    logout("[ENetClient] Background client network thread stopped cleanly.");
}
