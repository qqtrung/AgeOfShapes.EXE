#include "SceneMultiplayer.hpp"

#include "Debug.hpp"
#include "Game.hpp"
#include "SceneCombat.hpp"
#include "TextUtil.hpp"

SceneMultiplayer::SceneMultiplayer(Game* game) : Scene(game)
{

    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
        mGame->mNetwork.outgoingPacket.clear();
        mGame->startNetwork();
    }

    int currentId = (mGame && mGame->mId >= 0) ? mGame->mId : 0;
    mPlayerName   = "Player " + std::to_string(currentId + 1);

    if (mGame && mGame->isConnected())
    {
        Packet connPkt;
        connPkt.type     = PacketType::Connect;
        connPkt.data     = PacketConnect{mPlayerName, 1};
        connPkt.clientId = currentId;
        mGame->sendPacket(std::move(connPkt));
        mHasSentConnectPacket = true;
        mLobbyNotice          = "Connecting to server...";
    }
    else
    {
        mLobbyNotice = "Connecting to server in background...";
    }
}

SceneMultiplayer::~SceneMultiplayer()
{
    if (!mStartingCombat && !mServerDisconnected && mGame && mGame->isConnected())
    {
        if (mCurrentRoomId != -1)
        {
            Packet leavePkt;
            leavePkt.type = PacketType::RoomLeave;
            leavePkt.data = PacketRoomLeave{mCurrentRoomId, mGame->mId, "Player left multiplayer lobby room"};
            mGame->sendPacket(std::move(leavePkt));
        }

        Packet discPkt;
        discPkt.type = PacketType::Disconnect;
        discPkt.data = PacketDisconnect{mGame->mId, "Player left multiplayer lobby"};
        mGame->sendPacket(std::move(discPkt));
    }

    if (!mStartingCombat && mGame)
    {
        mGame->stopNetwork();
    }
}

void SceneMultiplayer::network()
{
    if (mGame && mGame->isConnected())
    {
        if (!mHasSentConnectPacket)
        {
            mHasSentConnectPacket = true;
            int currentId         = (mGame->mId >= 0) ? mGame->mId : 0;
            mPlayerName           = "Player " + std::to_string(currentId + 1);

            Packet connPkt;
            connPkt.type     = PacketType::Connect;
            connPkt.data     = PacketConnect{mPlayerName, 1};
            connPkt.clientId = currentId;
            mGame->sendPacket(std::move(connPkt));
            mLobbyNotice = "Connected to server. Logging in...";
        }
    }
    else
    {
        if (mHasSentConnectPacket)
        {
            mHasSentConnectPacket = false;
        }
        if (mLobbyNotice.empty() || mLobbyNotice == "In lobby..." || mLobbyNotice.starts_with("Connected"))
        {
            if (mLobbyNotice != "Offline / Connecting to server in background...")
            {
                mLobbyNotice = "Offline / Connecting to server in background...";
            }
        }
    }

    while (!mGame->mNetwork.ingoingPacket.empty())
    {
        Packet pkt = std::move(mGame->mNetwork.ingoingPacket.front());
        mGame->mNetwork.ingoingPacket.pop_front();

        switch (pkt.type)
        {
            case PacketType::ConnectAck:
            {
                if (const auto* ack = pkt.getIf<PacketConnectAck>())
                {
                    if (ack->accepted && ack->assignedPlayerId >= 0)
                    {
                        mGame->mId = ack->assignedPlayerId;
                        mGame->mNetwork.setId(ack->assignedPlayerId);
                        mPlayerName  = "Player " + std::to_string(mGame->mId + 1);
                        mLobbyNotice = "Connected to server (Client ID: " + std::to_string(mGame->mId) + ")";
                    }
                    else
                    {
                        mLobbyNotice = "Connection rejected by server.";
                    }
                }
                break;
            }
            case PacketType::LobbySync:
            {
                if (const auto* sync = pkt.getIf<PacketLobbySync>())
                {
                    mOnlineClients = sync->clients;
                    mOnlineRooms   = sync->rooms;
                }
                break;
            }
            case PacketType::RoomInviteNotify:
            {
                if (const auto* inv = pkt.getIf<PacketRoomInviteNotify>())
                {
                    mHasPendingInvite      = true;
                    mPendingInviteRoomId   = inv->roomId;
                    mPendingInviteHostId   = inv->hostClientId;
                    mPendingInviteHostName = inv->hostName;
                    mLobbyNotice           = inv->hostName + " invited you to Room #" + std::to_string(inv->roomId);
                }
                break;
            }
            case PacketType::RoomSync:
            {
                if (const auto* sync = pkt.getIf<PacketRoomSync>())
                {
                    mCurrentRoomId    = sync->roomId;
                    mRoomLevel        = sync->level;
                    mRoomDifficulty   = sync->difficulty;
                    mRoomGameMode     = sync->gameMode;
                    mCurrentRoomSlots = sync->slots;

                    int myId = (mGame && mGame->mId >= 0) ? mGame->mId : 0;

                    bool foundMySlot = false;
                    for (const auto& s : sync->slots)
                    {
                        if (s.clientId == myId)
                        {
                            foundMySlot = true;
                            mIsHost     = s.isHost;
                            mIsReady    = s.isReady;
                            if (!s.name.empty())
                            {
                                mPlayerName = s.name;
                            }
                            break;
                        }
                    }

                    if (!foundMySlot)
                    {
                        mCurrentRoomId = -1;
                        mIsHost        = false;
                        mIsReady       = false;
                        mCurrentRoomSlots.clear();
                    }

                    if (!sync->notice.empty())
                    {
                        mLobbyNotice = sync->notice;
                    }
                }
                break;
            }
            case PacketType::RoomLeave:
            {
                if (const auto* l = pkt.getIf<PacketRoomLeave>())
                {
                    if (mCurrentRoomId != -1 && l->roomId != -1 && l->roomId != mCurrentRoomId)
                    {
                        break;
                    }
                    mCurrentRoomId = -1;
                    mIsHost        = false;
                    mIsReady       = false;
                    mCurrentRoomSlots.clear();
                    if (!l->reason.empty())
                    {
                        mLobbyNotice = l->reason;
                    }
                    else
                    {
                        mLobbyNotice = "Room was disbanded. Returned to lobby.";
                    }
                }
                else
                {
                    mCurrentRoomId = -1;
                    mIsHost        = false;
                    mIsReady       = false;
                    mCurrentRoomSlots.clear();
                    mLobbyNotice = "Room was disbanded. Returned to lobby.";
                }
                break;
            }
            case PacketType::Disconnect:
            {
                if (const auto* disc = pkt.getIf<PacketDisconnect>())
                {
                    if (disc->playerId == -1)
                    {
                        mServerDisconnected = true;
                        logout() << "[SceneMultiplayer] Server closed connection. Returning to Menu!";
                        mGame->changeScene("Menu");
                        return;
                    }

                    if (!disc->reason.empty())
                    {
                        mLobbyNotice = disc->reason;
                    }
                    else if (disc->playerId >= 0)
                    {
                        mLobbyNotice = "Player " + std::to_string(disc->playerId + 1) + " disconnected.";
                    }
                    else
                    {
                        mLobbyNotice = "A player disconnected.";
                    }
                }
                break;
            }
            case PacketType::GameStart:
            {
                if (const auto* gs = pkt.getIf<PacketGameStart>())
                {
                    mStartingCombat = true;
                    int myId        = (mGame && mGame->mId >= 0) ? mGame->mId : 0;
                    int mySlot      = gs->slotIndex;
                    if (mySlot < 0 || mySlot >= gs->playerCount)
                    {
                        mySlot = 0;
                        for (size_t i = 0; i < mCurrentRoomSlots.size(); ++i)
                        {
                            if (mCurrentRoomSlots[i].clientId == myId)
                            {
                                mySlot = static_cast<int>(i);
                                break;
                            }
                        }
                    }

                    mGame->changeScene("Combat AI", gs->level, gs->difficulty, gs->playerCount, mySlot, gs->level, gs->gameMode);
                    return;
                }
                break;
            }
            default:
                break;
        }
    }
}

void SceneMultiplayer::update()
{
}

bool SceneMultiplayer::input(const sf::Event& event)
{
    sf::Vector2i pixelPos;
    bool         isClick = false;

    if (const auto* mouseButton = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButton->button == sf::Mouse::Button::Left)
        {
            pixelPos = mouseButton->position;
            isClick  = true;
        }
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
    {
        pixelPos = touch->position;
        isClick  = true;
    }

    if (!isClick)
    {
        return false;
    }

    sf::Vector2f mpos = mGame->mWindow.mapPixelToCoords(pixelPos, mGame->uiView());

    if (mHasPendingInvite)
    {
        sf::FloatRect acceptBtn({780.f, 650.f}, {420.f, 110.f});
        sf::FloatRect declineBtn({1332.f, 650.f}, {420.f, 110.f});

        if (acceptBtn.contains(mpos))
        {
            mIsHost  = false;
            mIsReady = false;
            Packet pkt;
            pkt.type = PacketType::RoomInviteResponse;
            pkt.data = PacketRoomInviteResponse{mPendingInviteRoomId, mGame->mId, true};
            mGame->sendPacket(std::move(pkt));
            mHasPendingInvite = false;
            return true;
        }
        if (declineBtn.contains(mpos))
        {
            Packet pkt;
            pkt.type = PacketType::RoomInviteResponse;
            pkt.data = PacketRoomInviteResponse{mPendingInviteRoomId, mGame->mId, false};
            mGame->sendPacket(std::move(pkt));
            mHasPendingInvite = false;
            return true;
        }
        return true;
    }

    if (mCurrentRoomId == -1)
    {
        sf::FloatRect backBtn({2180.f, 25.f}, {280.f, 80.f});
        if (backBtn.contains(mpos))
        {
            mGame->changeScene("Menu");
            return true;
        }

        sf::FloatRect createRoomBtn({70.f, 205.f}, {500.f, 90.f});
        if (createRoomBtn.contains(mpos))
        {
            if (!mGame || !mGame->isConnected())
            {
                mLobbyNotice = "Cannot create room: Not connected to server yet.";
                return true;
            }

            mIsHost  = true;
            mIsReady = true;
            Packet pkt;
            pkt.type = PacketType::RoomCreate;
            pkt.data = PacketRoomCreate{mGame->mId, ""};
            mGame->sendPacket(std::move(pkt));
            return true;
        }

        float ry = 310.f;
        for (const auto& r : mOnlineRooms)
        {
            if (!r.inGame && r.playerCount < r.maxPlayers)
            {
                sf::FloatRect joinBtn({2190.f, ry + 15.f}, {210.f, 65.f});
                if (joinBtn.contains(mpos))
                {
                    if (!mGame || !mGame->isConnected())
                    {
                        mLobbyNotice = "Cannot join room: Not connected to server yet.";
                        return true;
                    }

                    mIsHost  = false;
                    mIsReady = false;
                    Packet pkt;
                    pkt.type = PacketType::RoomJoin;
                    pkt.data = PacketRoomJoin{r.roomId, mGame->mId};
                    mGame->sendPacket(std::move(pkt));
                    return true;
                }
            }
            ry += 120.f;
        }

        return false;
    }

    sf::FloatRect leaveBtn({2180.f, 25.f}, {280.f, 80.f});
    if (leaveBtn.contains(mpos))
    {
        Packet pkt;
        pkt.type = PacketType::RoomLeave;
        pkt.data = PacketRoomLeave{mCurrentRoomId, mGame->mId};
        mGame->sendPacket(std::move(pkt));
        mCurrentRoomId = -1;
        mIsHost        = false;
        mIsReady       = false;
        mCurrentRoomSlots.clear();
        mLobbyNotice = "Left room. Returned to lobby.";
        return true;
    }

    if (mIsHost)
    {
        for (int gm = 0; gm < 2; ++gm)
        {
            float gx = 95.f + gm * 270.f;
            if (sf::FloatRect({gx, 168.f}, {250.f, 65.f}).contains(mpos))
            {
                if (mRoomGameMode != gm)
                {
                    mRoomGameMode = gm;
                    Packet pkt;
                    pkt.type = PacketType::RoomChangeSettings;
                    pkt.data = PacketRoomChangeSettings{mCurrentRoomId, mGame->mId, mRoomLevel, mRoomDifficulty, mRoomGameMode};
                    mGame->sendPacket(std::move(pkt));
                }
                return true;
            }
        }

        for (int lv = 1; lv <= 3; ++lv)
        {
            float bx = 675.f + (lv - 1) * 270.f;
            if (sf::FloatRect({bx, 168.f}, {250.f, 65.f}).contains(mpos))
            {
                if (mRoomLevel != lv)
                {
                    mRoomLevel = lv;
                    Packet pkt;
                    pkt.type = PacketType::RoomChangeSettings;
                    pkt.data = PacketRoomChangeSettings{mCurrentRoomId, mGame->mId, mRoomLevel, mRoomDifficulty, mRoomGameMode};
                    mGame->sendPacket(std::move(pkt));
                }
                return true;
            }
        }

        for (int df = 0; df < 3; ++df)
        {
            float dx = 1525.f + df * 310.f;
            if (sf::FloatRect({dx, 168.f}, {280.f, 65.f}).contains(mpos))
            {
                if (mRoomDifficulty != df)
                {
                    mRoomDifficulty = df;
                    Packet pkt;
                    pkt.type = PacketType::RoomChangeSettings;
                    pkt.data = PacketRoomChangeSettings{mCurrentRoomId, mGame->mId, mRoomLevel, mRoomDifficulty, mRoomGameMode};
                    mGame->sendPacket(std::move(pkt));
                }
                return true;
            }
        }

        sf::FloatRect startBtn({866.f, 1025.f}, {800.f, 115.f});
        if (startBtn.contains(mpos))
        {
            if (!mGame || !mGame->isConnected())
            {
                mLobbyNotice = "Cannot start match: Not connected to server.";
                return true;
            }

            bool hasMembers = (mCurrentRoomSlots.size() > 1);
            if (!hasMembers)
            {
                mLobbyNotice = "Waiting for other players to join (need at least 2)...";
                return true;
            }

            bool allReady = true;
            for (const auto& s : mCurrentRoomSlots)
            {
                if (!s.isHost && !s.isReady)
                {
                    allReady = false;
                    break;
                }
            }

            if (allReady)
            {
                Packet pkt;
                pkt.type = PacketType::GameStartRequest;
                pkt.data = PacketGameStartRequest{mCurrentRoomId};
                mGame->sendPacket(std::move(pkt));
            }
            else
            {
                mLobbyNotice = "Waiting for all members to be ready...";
            }
            return true;
        }

        float iy = 300.f;
        for (const auto& c : mOnlineClients)
        {
            if (c.id != mGame->mId && c.roomId == -1)
            {
                if (mCurrentRoomSlots.size() < 3)
                {
                    sf::FloatRect invBtn({2220.f, iy + 15.f}, {210.f, 70.f});
                    if (invBtn.contains(mpos))
                    {
                        Packet pkt;
                        pkt.type = PacketType::RoomInvite;
                        pkt.data = PacketRoomInvite{mCurrentRoomId, mGame->mId, c.id};
                        mGame->sendPacket(std::move(pkt));
                        return true;
                    }
                }
                iy += 115.f;
            }
        }

        for (size_t i = 0; i < mCurrentRoomSlots.size(); ++i)
        {
            const auto& s = mCurrentRoomSlots[i];
            if (!s.isHost)
            {
                float         py = 300.f + i * 215.f;
                sf::FloatRect kickBtn({1250.f, py + 55.f}, {220.f, 75.f});
                if (kickBtn.contains(mpos))
                {
                    Packet pkt;
                    pkt.type = PacketType::RoomKick;
                    pkt.data = PacketRoomKick{mCurrentRoomId, s.clientId};
                    mGame->sendPacket(std::move(pkt));
                    return true;
                }
            }
        }
    }
    else
    {
        sf::FloatRect readyBtn({866.f, 1025.f}, {800.f, 115.f});
        if (readyBtn.contains(mpos))
        {
            mIsReady = !mIsReady;
            Packet pkt;
            pkt.type = PacketType::Ready;
            pkt.data = PacketReady{mGame->mId, mIsReady};
            mGame->sendPacket(std::move(pkt));
            return true;
        }
    }

    return false;
}

void SceneMultiplayer::render()
{
    auto& window = mGame->mWindow;
    window.clear(sf::Color(14, 18, 28));
    window.setView(mGame->uiView());
    drawUI();
}

void SceneMultiplayer::drawUI()
{
    auto& font = mGame->mAssets.fontMap.at("Consolas");

    auto& target = mGame->mWindow;

    sf::RectangleShape headerBar({2532.f, 130.f});
    headerBar.setFillColor(sf::Color(20, 26, 42));
    target.draw(headerBar);

    sf::RectangleShape lineSep({2532.f, 4.f});
    lineSep.setPosition({0.f, 130.f});
    lineSep.setFillColor(sf::Color(0, 200, 255));
    target.draw(lineSep);

    std::string titleStr = (mCurrentRoomId != -1) ? (mPlayerName + "  -  ROOM #" + std::to_string(mCurrentRoomId))
                                                  : "MULTIPLAYER";
    sf::Text    title    = TextUtil::make(font, titleStr, 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({70.f, 38.f});
    target.draw(title);

    if (!mLobbyNotice.empty())
    {
        sf::Text noticeText = TextUtil::make(font, mLobbyNotice, 26);
        noticeText.setFillColor(sf::Color(255, 215, 90));
        noticeText.setPosition({820.f, 48.f});
        target.draw(noticeText);
    }

    if (mCurrentRoomId == -1)
    {
        sf::RectangleShape backBtn({280.f, 80.f});
        backBtn.setPosition({2180.f, 25.f});
        backBtn.setFillColor(sf::Color(38, 48, 68));
        backBtn.setOutlineColor(sf::Color(80, 110, 150));
        backBtn.setOutlineThickness(2.f);
        target.draw(backBtn);

        sf::Text bText = TextUtil::make(font, "< BACK", 30);
        bText.setFillColor(sf::Color::White);
        auto bb = TextUtil::bounds(bText);
        bText.setPosition(
            {2180.f + (280.f - bb.size.x) / 2.f - bb.position.x, 25.f + (80.f - bb.size.y) / 2.f - bb.position.y});
        target.draw(bText);

        sf::RectangleShape createBtn({500.f, 90.f});
        createBtn.setPosition({70.f, 205.f});
        createBtn.setFillColor(sf::Color(0, 150, 190));
        createBtn.setOutlineColor(sf::Color(0, 230, 255));
        createBtn.setOutlineThickness(2.f);
        target.draw(createBtn);

        sf::Text createLabel = TextUtil::make(font, "+ CREATE ROOM", 32);
        createLabel.setFillColor(sf::Color::White);
        auto cb = TextUtil::bounds(createLabel);
        createLabel.setPosition(
            {70.f + (500.f - cb.size.x) / 2.f - cb.position.x, 205.f + (90.f - cb.size.y) / 2.f - cb.position.y});
        target.draw(createLabel);

        sf::RectangleShape onlineBox({500.f, 790.f});
        onlineBox.setPosition({70.f, 320.f});
        onlineBox.setFillColor(sf::Color(20, 26, 42));
        onlineBox.setOutlineColor(sf::Color(45, 60, 85));
        onlineBox.setOutlineThickness(2.f);
        target.draw(onlineBox);

        float py = 350.f;
        for (const auto& c : mOnlineClients)
        {
            sf::CircleShape dot(8.f);
            dot.setPosition({100.f, py + 10.f});
            dot.setFillColor((c.id == mGame->mId) ? sf::Color(255, 205, 50) : sf::Color(0, 220, 255));
            target.draw(dot);

            sf::Text pLine = TextUtil::make(font, c.name + " (" + c.status + ")", 26);
            pLine.setFillColor((c.id == mGame->mId) ? sf::Color(255, 205, 50) : sf::Color(210, 225, 240));
            pLine.setPosition({130.f, py});
            target.draw(pLine);
            py += 65.f;
        }

        sf::RectangleShape roomsBox({1850.f, 905.f});
        roomsBox.setPosition({610.f, 205.f});
        roomsBox.setFillColor(sf::Color(20, 26, 42));
        roomsBox.setOutlineColor(sf::Color(45, 60, 85));
        roomsBox.setOutlineThickness(2.f);
        target.draw(roomsBox);

        if (!mOnlineRooms.empty())
        {
            float ry = 310.f;
            for (const auto& r : mOnlineRooms)
            {
                sf::RectangleShape rCard({1790.f, 95.f});
                rCard.setPosition({640.f, ry});
                rCard.setFillColor(sf::Color(28, 36, 56));
                rCard.setOutlineColor(sf::Color(55, 75, 105));
                rCard.setOutlineThickness(1.5f);
                target.draw(rCard);

                std::string diffName  = (r.difficulty == 0) ? "EASY" : ((r.difficulty == 1) ? "NORMAL" : "HARD");
                std::string statusStr = r.inGame ? "[PLAYING]" : "[OPEN]";
                sf::Color   sColor    = r.inGame ? sf::Color(255, 90, 90) : sf::Color(80, 255, 140);

                sf::Text rInfo = TextUtil::make(font,
                                                "Room #" + std::to_string(r.roomId) + "   Host: " + r.hostName +
                                                    "   Players: " + std::to_string(r.playerCount) + "/" +
                                                    std::to_string(r.maxPlayers) + "   Level: " + std::to_string(r.level) +
                                                    " (" + diffName + ")   " + statusStr,
                                                28);
                rInfo.setFillColor(sColor);
                rInfo.setPosition({665.f, ry + 28.f});
                target.draw(rInfo);

                if (!r.inGame && r.playerCount < r.maxPlayers)
                {
                    sf::RectangleShape joinBtn({210.f, 65.f});
                    joinBtn.setPosition({2190.f, ry + 15.f});
                    joinBtn.setFillColor(sf::Color(0, 150, 80));
                    joinBtn.setOutlineColor(sf::Color(50, 255, 140));
                    joinBtn.setOutlineThickness(2.f);
                    target.draw(joinBtn);

                    sf::Text jText = TextUtil::make(font, "JOIN", 28);
                    jText.setFillColor(sf::Color::White);
                    auto jb = TextUtil::bounds(jText);
                    jText.setPosition({2190.f + (210.f - jb.size.x) / 2.f - jb.position.x,
                                       ry + 15.f + (65.f - jb.size.y) / 2.f - jb.position.y});
                    target.draw(jText);
                }
                ry += 120.f;
            }
        }
    }
    else
    {
        sf::RectangleShape settingsBar({2392.f, 90.f});
        settingsBar.setPosition({70.f, 155.f});
        settingsBar.setFillColor(sf::Color(18, 24, 38));
        settingsBar.setOutlineColor(sf::Color(45, 65, 95));
        settingsBar.setOutlineThickness(2.f);
        target.draw(settingsBar);

        const char* modeLabels[]    = {"CO-OP", "PVP"};
        sf::Color   modeSelColors[] = {sf::Color(20, 110, 160), sf::Color(170, 35, 45)};
        sf::Color   modeOutlines[]  = {sf::Color(70, 210, 255), sf::Color(255, 80, 90)};
        for (int gm = 0; gm < 2; ++gm)
        {
            float              gx    = 95.f + gm * 270.f;
            bool               isSel = (mRoomGameMode == gm);
            sf::RectangleShape b({250.f, 65.f});
            b.setPosition({gx, 168.f});
            b.setFillColor(isSel ? modeSelColors[gm] : sf::Color(26, 34, 50));
            b.setOutlineColor(isSel ? modeOutlines[gm] : sf::Color(50, 68, 95));
            b.setOutlineThickness(isSel ? 2.5f : 1.5f);
            target.draw(b);

            sf::Text t = TextUtil::make(font, modeLabels[gm], 28);
            t.setFillColor(isSel ? sf::Color::White : sf::Color(160, 175, 195));
            auto tb = TextUtil::bounds(t);
            t.setPosition(
                {gx + (250.f - tb.size.x) / 2.f - tb.position.x, 168.f + (65.f - tb.size.y) / 2.f - tb.position.y});
            target.draw(t);
        }

        sf::RectangleShape sep1({2.f, 65.f});
        sep1.setPosition({645.f, 168.f});
        sep1.setFillColor(sf::Color(45, 65, 95));
        target.draw(sep1);

        for (int lv = 1; lv <= 3; ++lv)
        {
            float              bx    = 675.f + (lv - 1) * 270.f;
            bool               isSel = (mRoomLevel == lv);
            sf::RectangleShape b({250.f, 65.f});
            b.setPosition({bx, 168.f});
            b.setFillColor(isSel ? sf::Color(0, 150, 100) : sf::Color(26, 34, 50));
            b.setOutlineColor(isSel ? sf::Color(80, 255, 170) : sf::Color(50, 68, 95));
            b.setOutlineThickness(isSel ? 2.5f : 1.5f);
            target.draw(b);

            sf::Text t = TextUtil::make(font, "MAP " + std::to_string(lv), 28);
            t.setFillColor(isSel ? sf::Color::White : sf::Color(160, 175, 195));
            auto tb = TextUtil::bounds(t);
            t.setPosition(
                {bx + (250.f - tb.size.x) / 2.f - tb.position.x, 168.f + (65.f - tb.size.y) / 2.f - tb.position.y});
            target.draw(t);
        }

        sf::RectangleShape sep({2.f, 65.f});
        sep.setPosition({1495.f, 168.f});
        sep.setFillColor(sf::Color(45, 65, 95));
        target.draw(sep);

        const char* diffLabels[]      = {"EASY", "NORMAL", "HARD"};
        sf::Color   diffSelColors[]   = {sf::Color(20, 120, 140), sf::Color(170, 100, 20), sf::Color(150, 35, 45)};
        sf::Color   diffSelOutlines[] = {sf::Color(50, 220, 255), sf::Color(255, 190, 60), sf::Color(255, 80, 90)};

        for (int df = 0; df < 3; ++df)
        {
            float              dx    = 1525.f + df * 310.f;
            bool               isSel = (mRoomDifficulty == df);
            sf::RectangleShape b({280.f, 65.f});
            b.setPosition({dx, 168.f});
            b.setFillColor(isSel ? diffSelColors[df] : sf::Color(26, 34, 50));
            b.setOutlineColor(isSel ? diffSelOutlines[df] : sf::Color(50, 68, 95));
            b.setOutlineThickness(isSel ? 2.5f : 1.5f);
            target.draw(b);

            sf::Text t = TextUtil::make(font, diffLabels[df], 28);
            t.setFillColor(isSel ? sf::Color::White : sf::Color(160, 175, 195));
            auto tb = TextUtil::bounds(t);
            t.setPosition(
                {dx + (280.f - tb.size.x) / 2.f - tb.position.x, 168.f + (65.f - tb.size.y) / 2.f - tb.position.y});
            target.draw(t);
        }

        sf::RectangleShape leaveBtn({280.f, 80.f});
        leaveBtn.setPosition({2180.f, 25.f});
        leaveBtn.setFillColor(sf::Color(120, 25, 35));
        leaveBtn.setOutlineColor(sf::Color(240, 70, 80));
        leaveBtn.setOutlineThickness(2.f);
        target.draw(leaveBtn);

        sf::Text leaveText = TextUtil::make(font, "LEAVE", 30);
        leaveText.setFillColor(sf::Color::White);
        auto lb = TextUtil::bounds(leaveText);
        leaveText.setPosition(
            {2180.f + (280.f - lb.size.x) / 2.f - lb.position.x, 25.f + (80.f - lb.size.y) / 2.f - lb.position.y});
        target.draw(leaveText);

        sf::RectangleShape slotBox({1460.f, 730.f});
        slotBox.setPosition({70.f, 270.f});
        slotBox.setFillColor(sf::Color(20, 26, 42));
        slotBox.setOutlineColor(sf::Color(45, 60, 85));
        slotBox.setOutlineThickness(2.f);
        target.draw(slotBox);

        for (size_t i = 0; i < 3; ++i)
        {
            float py = 300.f + i * 215.f;

            sf::RectangleShape slotCard({1400.f, 185.f});
            slotCard.setPosition({100.f, py});

            if (i < mCurrentRoomSlots.size())
            {
                const auto& s = mCurrentRoomSlots[i];
                slotCard.setFillColor(sf::Color(28, 36, 56));
                slotCard.setOutlineColor(s.isHost ? sf::Color(255, 205, 50) : sf::Color(0, 200, 255));
                slotCard.setOutlineThickness(2.f);
                target.draw(slotCard);

                std::string roleTag = s.isHost ? "HOST" : (s.isReady ? "READY" : "WAIT");
                sf::Color   rColor  = s.isHost ? sf::Color(255, 215, 80)
                                               : (s.isReady ? sf::Color(80, 255, 140) : sf::Color(255, 130, 130));

                sf::Text nameText = TextUtil::make(font, s.name, 38);
                nameText.setFillColor(sf::Color::White);
                nameText.setPosition({135.f, py + 35.f});
                target.draw(nameText);

                sf::Text tagText = TextUtil::make(font, roleTag, 28);
                tagText.setFillColor(rColor);
                tagText.setPosition({135.f, py + 105.f});
                target.draw(tagText);

                if (mIsHost && !s.isHost)
                {
                    sf::RectangleShape kickBtn({220.f, 75.f});
                    kickBtn.setPosition({1250.f, py + 55.f});
                    kickBtn.setFillColor(sf::Color(120, 20, 30));
                    kickBtn.setOutlineColor(sf::Color(255, 75, 75));
                    kickBtn.setOutlineThickness(2.f);
                    target.draw(kickBtn);

                    sf::Text kText = TextUtil::make(font, "KICK", 26);
                    kText.setFillColor(sf::Color::White);
                    auto kb = TextUtil::bounds(kText);
                    kText.setPosition({1250.f + (220.f - kb.size.x) / 2.f - kb.position.x,
                                       py + 55.f + (75.f - kb.size.y) / 2.f - kb.position.y});
                    target.draw(kText);
                }
            }
            else
            {
                slotCard.setFillColor(sf::Color(16, 20, 30));
                slotCard.setOutlineColor(sf::Color(35, 45, 62));
                slotCard.setOutlineThickness(1.5f);
                target.draw(slotCard);

                sf::Text emptyText = TextUtil::make(font, "[EMPTY SLOT]", 32);
                emptyText.setFillColor(sf::Color(110, 125, 145));
                emptyText.setPosition({135.f, py + 75.f});
                target.draw(emptyText);
            }
        }

        sf::RectangleShape invBox({902.f, 730.f});
        invBox.setPosition({1560.f, 270.f});
        invBox.setFillColor(sf::Color(20, 26, 42));
        invBox.setOutlineColor(sf::Color(45, 60, 85));
        invBox.setOutlineThickness(2.f);
        target.draw(invBox);

        float iy = 300.f;
        for (const auto& c : mOnlineClients)
        {
            if (c.id != mGame->mId && c.roomId == -1)
            {
                sf::Text cName = TextUtil::make(font, c.name, 30);
                cName.setFillColor(sf::Color(210, 225, 240));
                cName.setPosition({1600.f, iy + 25.f});
                target.draw(cName);

                if (mIsHost && mCurrentRoomSlots.size() < 3)
                {
                    sf::RectangleShape invBtn({210.f, 70.f});
                    invBtn.setPosition({2220.f, iy + 15.f});
                    invBtn.setFillColor(sf::Color(0, 120, 150));
                    invBtn.setOutlineColor(sf::Color(0, 220, 255));
                    invBtn.setOutlineThickness(2.f);
                    target.draw(invBtn);

                    sf::Text invText = TextUtil::make(font, "INVITE", 26);
                    invText.setFillColor(sf::Color::White);
                    auto ib = TextUtil::bounds(invText);
                    invText.setPosition({2220.f + (210.f - ib.size.x) / 2.f - ib.position.x,
                                         iy + 15.f + (70.f - ib.size.y) / 2.f - ib.position.y});
                    target.draw(invText);
                }
                iy += 115.f;
            }
        }

        sf::RectangleShape actionBtn({800.f, 115.f});
        actionBtn.setPosition({866.f, 1025.f});

        if (mIsHost)
        {
            bool hasMembers = (mCurrentRoomSlots.size() > 1);
            bool allReady   = true;
            if (hasMembers)
            {
                for (const auto& s : mCurrentRoomSlots)
                {
                    if (!s.isHost && !s.isReady)
                    {
                        allReady = false;
                        break;
                    }
                }
            }

            bool canStart = (hasMembers && allReady);
            actionBtn.setFillColor(canStart ? sf::Color(0, 160, 90) : sf::Color(38, 46, 62));
            actionBtn.setOutlineColor(canStart ? sf::Color(0, 255, 160) : sf::Color(70, 85, 105));
            actionBtn.setOutlineThickness(2.5f);
            target.draw(actionBtn);

            std::string startLabel = canStart ? "START" : (hasMembers ? "WAITING READY" : "WAITING PLAYERS");
            sf::Text    aText      = TextUtil::make(font, startLabel, 40);
            aText.setFillColor(sf::Color::White);
            auto ab = TextUtil::bounds(aText);
            aText.setPosition(
                {866.f + (800.f - ab.size.x) / 2.f - ab.position.x, 1025.f + (115.f - ab.size.y) / 2.f - ab.position.y});
            target.draw(aText);
        }
        else
        {
            actionBtn.setFillColor(mIsReady ? sf::Color(160, 95, 20) : sf::Color(0, 150, 85));
            actionBtn.setOutlineColor(mIsReady ? sf::Color(255, 200, 50) : sf::Color(50, 255, 150));
            actionBtn.setOutlineThickness(2.5f);
            target.draw(actionBtn);

            sf::Text aText = TextUtil::make(font, mIsReady ? "CANCEL" : "READY", 42);
            aText.setFillColor(sf::Color::White);
            auto ab = TextUtil::bounds(aText);
            aText.setPosition(
                {866.f + (800.f - ab.size.x) / 2.f - ab.position.x, 1025.f + (115.f - ab.size.y) / 2.f - ab.position.y});
            target.draw(aText);
        }
    }

    if (mHasPendingInvite)
    {
        sf::RectangleShape overlay({2532.f, 1170.f});
        overlay.setFillColor(sf::Color(0, 0, 0, 190));
        target.draw(overlay);

        sf::RectangleShape modal({1200.f, 500.f});
        modal.setPosition({666.f, 335.f});
        modal.setFillColor(sf::Color(22, 28, 42));
        modal.setOutlineColor(sf::Color(255, 180, 60));
        modal.setOutlineThickness(3.f);
        target.draw(modal);

        sf::Text mTitle = TextUtil::make(font, "ROOM INVITATION", 40);
        mTitle.setFillColor(sf::Color(255, 205, 70));
        auto mtb = TextUtil::bounds(mTitle);
        mTitle.setPosition({666.f + (1200.f - mtb.size.x) / 2.f - mtb.position.x, 380.f});
        target.draw(mTitle);

        sf::Text mDesc = TextUtil::make(font,
                                        mPendingInviteHostName + " invited you to join Room #" +
                                            std::to_string(mPendingInviteRoomId),
                                        32);
        mDesc.setFillColor(sf::Color::White);
        auto mdb = TextUtil::bounds(mDesc);
        mDesc.setPosition({666.f + (1200.f - mdb.size.x) / 2.f - mdb.position.x, 480.f});
        target.draw(mDesc);

        sf::RectangleShape accBtn({420.f, 110.f});
        accBtn.setPosition({780.f, 650.f});
        accBtn.setFillColor(sf::Color(0, 140, 80));
        accBtn.setOutlineColor(sf::Color(50, 255, 140));
        accBtn.setOutlineThickness(2.5f);
        target.draw(accBtn);

        sf::Text accText = TextUtil::make(font, "ACCEPT", 34);
        accText.setFillColor(sf::Color::White);
        auto acb = TextUtil::bounds(accText);
        accText.setPosition(
            {780.f + (420.f - acb.size.x) / 2.f - acb.position.x, 650.f + (110.f - acb.size.y) / 2.f - acb.position.y});
        target.draw(accText);

        sf::RectangleShape decBtn({420.f, 110.f});
        decBtn.setPosition({1332.f, 650.f});
        decBtn.setFillColor(sf::Color(120, 25, 35));
        decBtn.setOutlineColor(sf::Color(255, 75, 75));
        decBtn.setOutlineThickness(2.5f);
        target.draw(decBtn);

        sf::Text decText = TextUtil::make(font, "DECLINE", 34);
        decText.setFillColor(sf::Color::White);
        auto dcb = TextUtil::bounds(decText);
        decText.setPosition(
            {1332.f + (420.f - dcb.size.x) / 2.f - dcb.position.x, 650.f + (110.f - dcb.size.y) / 2.f - dcb.position.y});
        target.draw(decText);
    }
}

void SceneMultiplayer::doAction(const Action& action)
{
    if (action.type == Action::Type::Back)
    {
        if (mHasPendingInvite)
        {
            Packet pkt;
            pkt.type = PacketType::RoomInviteResponse;
            pkt.data = PacketRoomInviteResponse{mPendingInviteRoomId, mGame->mId, false};
            mGame->sendPacket(std::move(pkt));
            mHasPendingInvite = false;
            return;
        }

        if (mCurrentRoomId != -1)
        {
            Packet pkt;
            pkt.type = PacketType::RoomLeave;
            pkt.data = PacketRoomLeave{mCurrentRoomId, mGame->mId};
            mGame->sendPacket(std::move(pkt));
            mCurrentRoomId = -1;
            mIsHost        = false;
            mIsReady       = false;
            mCurrentRoomSlots.clear();
            mLobbyNotice = "Left room. Returned to lobby.";
            return;
        }
        else
        {
            mGame->changeScene("Menu");
            return;
        }
    }
    Scene::doAction(action);
}
