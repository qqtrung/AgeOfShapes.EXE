#include "SceneCombat.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "GuiBalo.hpp"
#include "GuiCombat.hpp"
#include "GuiControl.hpp"
#include "GuiMinimap.hpp"

void SceneCombat::logout(const std::string& message, bool isError)
{
    ::logout(message, isError);
}

LogStream SceneCombat::logout(bool isError)
{
    return LogStream(isError);
}

void SceneCombat::logOut(const std::string& message, bool isError)
{
    logout(message, isError);
}

LogStream SceneCombat::logOut(bool isError)
{
    return LogStream(isError);
}

SceneCombat::SceneCombat(Game*              game,
                         const std::string& path,
                         int                level,
                         int                difficulty,
                         int                numClients,
                         int                slotIndex,
                         int                map,
                         int                gameMode) :
    Scene(game)
{
    logout() << "[SceneCombat] Client slotIndex = " << slotIndex << ", numClients = " << numClients;

    if (mGame)
    {
        if (numClients <= 1)
        {
            mGame->mNetwork.ingoingPacket.clear();
        }
        else
        {
            std::list<Packet> combatPackets;
            for (auto& p : mGame->mNetwork.ingoingPacket)
            {
                if (p.type == PacketType::CommandSync || p.type == PacketType::StateHash)
                {
                    combatPackets.push_back(std::move(p));
                }
            }
            mGame->mNetwork.ingoingPacket = std::move(combatPackets);
        }

        mGame->mLatestReplay.commands.clear();
        mGame->mLatestReplay.level       = level;
        mGame->mLatestReplay.difficulty  = difficulty;
        mGame->mLatestReplay.playerCount = numClients;
        mGame->mLatestReplay.map         = map;
        mGame->mLatestReplay.gameMode    = gameMode;
        mGame->mLatestReplay.totalTicks  = 0;
        mGame->mLatestReplay.hasReplay   = false;

        mGame->mStateHistory.clear();
        mGame->mCommandHistory.clear();
    }

    client = std::make_unique<ClientGame>(game, path, level, difficulty, false, slotIndex, numClients, map, gameMode);
    client->addChild<GUIMinimap>(game->mAssets.fontMap["Consolas"]);
    client->addChild<GuiBalo>(game->mAssets.fontMap["Consolas"]);
    client->addChild<GUIControl>(game->mAssets.fontMap["Consolas"]);
    client->addChild<GUICombat>(game->mAssets.fontMap["Consolas"]);
}

SceneCombat::~SceneCombat()
{
    if (!mReturningToMultiplayer && mGame && client && client->mNumClients > 1 && mGame->isConnected())
    {
        Packet leavePkt;
        leavePkt.type = PacketType::RoomLeave;
        leavePkt.data = PacketRoomLeave{-1, mGame->mId, "Player exited combat match"};
        mGame->sendPacket(std::move(leavePkt));

        Packet discPkt;
        discPkt.type = PacketType::Disconnect;
        discPkt.data = PacketDisconnect{mGame->mId, "Player exited combat match"};
        mGame->sendPacket(std::move(discPkt));
    }

    if (mGame && client && client->mNumClients > 1)
    {
        mGame->stopNetwork();
    }
}

void SceneCombat::network()
{
    if (!client)
        return;

    if (client->mNumClients <= 1)
    {
        if (mGame)
        {
            mGame->mNetwork.ingoingPacket.clear();
        }
        return;
    }

    while (!mGame->mNetwork.ingoingPacket.empty())
    {
        Packet pkt = std::move(mGame->mNetwork.ingoingPacket.front());
        mGame->mNetwork.ingoingPacket.pop_front();

        if (pkt.type == PacketType::Disconnect || pkt.type == PacketType::RoomLeave)
        {
            mReturningToMultiplayer = true;
            logout() << "[SceneCombat] A player left the match or the room was disbanded. Returning to Menu!";
            mGame->mNetwork.ingoingPacket.clear();
            mGame->mWindow.setView(mGame->uiView());
            mGame->changeScene("Menu");
            return;
        }

        client->ingoingPacket.push_back(std::move(pkt));
    }

    client->network();

    while (!client->outgoingPacket.empty())
    {
        mGame->sendPacket(std::move(client->outgoingPacket.front()));
        client->outgoingPacket.pop_front();
    }
}

void SceneCombat::update()
{
    if (client)
    {
        client->updateTraverse();
    }
}

void SceneCombat::render()
{
    if (client)
    {
        client->renderTraverse();
    }
}

bool SceneCombat::input(const sf::Event& event)
{
    if (client)
    {
        return client->inputTraverse(event);
    }
    return false;
}

void SceneCombat::doAction(const Action& action)
{
    if (client)
    {
        client->doAction(action);
    }
    else
    {
        Scene::doAction(action);
    }
}
