#include "ClientGame.hpp"

#include "Components.hpp"
#include "DeterministicRandom.hpp"
#include "FloatFixed.hpp"
#include "Game.hpp"
#include "GameConfig.hpp"
#include "GameState.hpp"
#include "GameStateDetail.hpp"
#include "GuiCombat.hpp"
#include "GuiLoss.hpp"
#include "GuiVictory.hpp"
#include "Profiler.hpp"
#include "SystemGameState.hpp"
#include "SystemViewState.hpp"
#include "ViewState.hpp"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <numbers>
#include <string>

#include <cassert>

ClientGame::ClientGame(Game* game, const std::string&, int level, int difficulty, bool replay, int id, int numClients, int map, int gameMode) :
    Scene(game)
{
    mGameState = std::make_shared<GameState>();
    mViewState = std::make_shared<ViewState>(this);

    this->isReplay        = replay;
    this->mId             = id;
    this->mDesyncDetected = false;
    this->mNumClients     = numClients;

    mGameState->isReplay        = replay;
    mGameState->mId             = id;
    mGameState->mDesyncDetected = false;
    mGameState->mPlayerCount    = numClients;

    int resolvedMap                           = (map != 1) ? map : level;
    int resolvedLevel                         = std::clamp(difficulty + 1, 1, 3);
    this->mGameState->campaign().currentMap   = std::clamp(resolvedMap, 1, 3);
    this->mGameState->campaign().currentLevel = resolvedLevel;
    this->mGameState->campaign().difficulty   = difficulty;

    if (gameMode == 1 && numClients >= 2)
    {
        this->mGameState->setGameMode(GameState::GameMode::PvP);
    }
    else
    {
        this->mGameState->setGameMode(GameState::GameMode::Coop);
    }

    mGameState->mSystems.init();
    mViewState->mSystems.init();

    std::vector<int> playerEntities;
    for (int uid : mGameState->admin().mPool.viewIndex<CPlayer>())
    {
        playerEntities.push_back(uid);
    }
    std::ranges::sort(playerEntities);

    if (this->mId >= 0 && this->mId < static_cast<int>(playerEntities.size()))
    {
        mViewState->mPlayerEntityId = playerEntities[this->mId];
    }
    else if (!playerEntities.empty())
    {
        mViewState->mPlayerEntityId = playerEntities[0];
    }

    auto viewSize = sf::Vector2f(mGame->uiView().getSize() / 1.5f);
    mViewState->worldView().setSize(viewSize * mViewState->zoomLevel());
    mViewState->worldView().setCenter(viewSize / 2.f);
}

sf::RenderWindow& ClientGame::window()
{
    return mWindow ? *mWindow : Scene::window();
}

[[nodiscard]] const GameState& ClientGame::gameState() const
{
    return *this->mGameState;
}

bool ClientGame::isGameOver() const
{
    return (mGameState && mGameState->isGameOver()) || hasChild<GUIVictory>() || hasChild<GUILoss>();
}

void ClientGame::pushCommand(CommandOnTick packet)
{
    if (mId == 0 && !isReplay)
    {
        mGame->mCommands.push(packet);
    }
    mCommandQueue.push(std::move(packet));
}

void ClientGame::queueOutgoingCommand(CommandOnTick packet)
{
    if (static_cast<int64_t>(packet.tick) <= mServerAckedTick)
    {
        return;
    }
    mUnackedOutgoingCommands[packet.tick] = std::move(packet);
    mNeedSendCommandSync                  = true;
}

void ClientGame::processIncomingServerCommand(const CommandOnTick& cot)
{
    if (mIncomingReorderBuffer.empty() && static_cast<int64_t>(cot.tick) == mLastContiguousServerTick + 1)
    {
        pushCommand(cot);
        mLastContiguousServerTick = static_cast<int64_t>(cot.tick);
        return;
    }

    if (static_cast<int64_t>(cot.tick) <= mLastContiguousServerTick || mIncomingReorderBuffer.contains(cot.tick))
    {
        return;
    }

    mIncomingReorderBuffer[cot.tick] = cot;

    while (true)
    {
        uint64_t nextTick = static_cast<uint64_t>(mLastContiguousServerTick + 1);
        auto     it       = mIncomingReorderBuffer.find(nextTick);
        if (it == mIncomingReorderBuffer.end())
        {
            break;
        }
        pushCommand(std::move(it->second));
        mIncomingReorderBuffer.erase(it);
        mLastContiguousServerTick = static_cast<int64_t>(nextTick);
    }
}

void ClientGame::sendRedundantCommands()
{
    if (mUnackedOutgoingCommands.empty() && mLastContiguousServerTick <= mLastAckSentToServer)
    {
        mNeedSendCommandSync = false;
        return;
    }

    PacketCommandSync sync;
    sync.ackTick = mLastContiguousServerTick;
    sync.history.reserve(mUnackedOutgoingCommands.size());

    uint64_t latestTick = 0;
    for (const auto& [tick, cot] : mUnackedOutgoingCommands)
    {
        sync.history.push_back(cot);
        latestTick = tick;
    }

    Packet pkt;
    pkt.type = PacketType::CommandSync;
    pkt.tick = mUnackedOutgoingCommands.empty() ? static_cast<uint64_t>(std::max<int64_t>(0, mLastContiguousServerTick))
                                                : latestTick;
    pkt.data = std::move(sync);
    pkt.clientId = mId;

    uint64_t now        = SimulationClock::nowUs();
    uint64_t halfPingUs = (GameConfig::Network::kSimulatedPingMs / 2) * 1000;
    pkt.receiveTime     = now + halfPingUs;

    outgoingPacket.push_back(std::move(pkt));
    mLastAckSentToServer = mLastContiguousServerTick;
    mNeedSendCommandSync = false;
    mLastSyncSentTimeUs  = now;
    mLastSyncSentTick    = mGameState ? mGameState->mTick : 0;
}

void ClientGame::network()
{
    while (!ingoingPacket.empty())
    {
        Packet packet = std::move(ingoingPacket.front());
        ingoingPacket.pop_front();

        if (packet.type == PacketType::CommandSync)
        {
            if (const auto* cot = packet.getIf<CommandOnTick>())
            {
                processIncomingServerCommand(*cot);
            }
            else if (const auto* sync = packet.getIf<PacketCommandSync>())
            {
                int64_t prevContiguous = mLastContiguousServerTick;

                if (sync->ackTick > mServerAckedTick)
                {
                    mServerAckedTick = sync->ackTick;
                    for (auto it = mUnackedOutgoingCommands.begin(); it != mUnackedOutgoingCommands.end();)
                    {
                        if (static_cast<int64_t>(it->first) <= mServerAckedTick)
                        {
                            it = mUnackedOutgoingCommands.erase(it);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                for (const auto& item : sync->history)
                {
                    processIncomingServerCommand(item);
                }

                if (!sync->history.empty() && mLastContiguousServerTick == prevContiguous &&
                    mLastAckSentToServer >= mLastContiguousServerTick)
                {
                    mLastAckSentToServer = mLastContiguousServerTick - 1;
                }
            }
        }
    }

    if (!isReplay && !isGameOver() && mGameState && mGameState->mPlayerCount > 1 && (!mGame || mGame->isConnected()))
    {
        uint64_t now           = SimulationClock::nowUs();
        bool     hasDataToSend = mNeedSendCommandSync || !mUnackedOutgoingCommands.empty() ||
                                 (mLastContiguousServerTick > mLastAckSentToServer);
        bool     isTickDue = (mGameState->mTick >= mLastSyncSentTick + 2) || (mGameState->mTick < mLastSyncSentTick);
        bool isTimeDue = (mLastSyncSentTimeUs == 0) || (now >= mLastSyncSentTimeUs + GameConfig::Game::kNetSyncIntervalUs);
        bool isDueToSend = isTickDue || isTimeDue;

        if (hasDataToSend && isDueToSend)
        {
            sendRedundantCommands();
        }
    }
}

bool ClientGame::input(const sf::Event& event)
{
    if (isGameOver())
    {
        return false;
    }

    return mViewState->input(event);
}

void ClientGame::update()
{
    PROFILE_FUNCTION();
    const auto updateStartTime = std::chrono::high_resolution_clock::now();
    this->mDesyncDetected      = mGameState->mDesyncDetected;

    if (mDesyncDetected)
    {
        return;
    }

    mCurrentFrame++;

    if (isGameOver())
    {
        if (mGame)
        {
            mGame->setLockstepBufferSize(-1);
        }
        while (!mCommandQueue.empty())
        {
            mCommandQueue.pop();
        }
        return;
    }

    constexpr uint64_t delay       = GameConfig::Game::kCommandDelayTicks;
    int                updateCount = static_cast<int>(mGameState->mTimeScale);

    if (mGame)
    {
        const bool isLockstepSync = (!isReplay && mGameState->mPlayerCount > 1 && mGame->isConnected());
        mGame->setLockstepBufferSize(
            (isLockstepSync && mGameState->mTick >= delay) ? static_cast<int>(mCommandQueue.size()) : -1);
    }

    uint64_t targetTick = mGameState->mTick + updateCount;

    int           simulatedSteps           = 0;
    constexpr int kMaxCatchUpStepsPerFrame = 16;

    while (mGameState->mTick < targetTick && simulatedSteps < kMaxCatchUpStepsPerFrame)
    {
        simulatedSteps++;
        uint64_t      currentTick = mGameState->mTick;
        CommandOnTick tickCmd;

        if (isReplay)
        {
            while (!mCommandQueue.empty() && mCommandQueue.front().tick < currentTick)
            {
                mCommandQueue.pop();
            }

            if (!mCommandQueue.empty() && mCommandQueue.front().tick == currentTick)
            {
                tickCmd = std::move(mCommandQueue.front());
                mCommandQueue.pop();
            }
            else
            {
                tickCmd.tick = currentTick;
            }
        }
        else if (mGameState->mPlayerCount == 1 || (mGame && !mGame->isConnected()))
        {
            while (!mCommandQueue.empty() && mCommandQueue.front().tick < currentTick)
            {
                mCommandQueue.pop();
            }

            if (!mCommandQueue.empty() && mCommandQueue.front().tick == currentTick)
            {
                tickCmd = std::move(mCommandQueue.front());
                mCommandQueue.pop();
            }
            else
            {
                tickCmd.tick = currentTick;
            }
        }
        else if (currentTick < delay)
        {
            if (!mCommandQueue.empty() && mCommandQueue.front().tick == currentTick)
            {
                tickCmd = std::move(mCommandQueue.front());
                mCommandQueue.pop();
            }
            else
            {
                tickCmd.tick = currentTick;
            }
        }
        else
        {
            while (!mCommandQueue.empty() && mCommandQueue.front().tick < currentTick)
            {
                mCommandQueue.pop();
            }

            if (mCommandQueue.empty() || mCommandQueue.front().tick > currentTick)
            {
                logout() << "[LOCKSTEP STALL] Tick " << currentTick << ": Waiting for CommandOnTick packet from network...";

                break;
            }

            tickCmd = std::move(mCommandQueue.front());
            mCommandQueue.pop();
        }

        if (!mGameState->update(tickCmd))
        {
            break;
        }

        if (!isReplay)
        {
            mViewState->flushCommand();
        }

        if (mId == 0 && !isReplay)
        {
            mGame->mReplayTotalTicks = mGameState->mTick;
        }

        mViewState->handleEvents(mGameState->events());
        mGameState->clearEvents();

        if (isGameOver())
        {
            while (!mCommandQueue.empty())
            {
                mCommandQueue.pop();
            }
            break;
        }
    }

    if (mGameState && mGameState->isGameOver())
    {
        if (mViewState->isLocalVictory())
        {
            if (!hasChild<GUIVictory>())
            {
                addChild<GUIVictory>(mGame->mAssets.fontMap["Consolas"]);
            }
        }
        else
        {
            if (!hasChild<GUILoss>())
            {
                addChild<GUILoss>(mGame->mAssets.fontMap["Consolas"]);
            }
        }
    }

    if (!isGameOver())
    {
        mViewState->updateVisuals();
    }

    const auto updateEndTime = std::chrono::high_resolution_clock::now();
    if (mViewState)
    {
        mViewState->mProfiler.updateTime = std::chrono::duration<float, std::milli>(updateEndTime - updateStartTime).count();
        if (mGameState && mViewState->debug().showPanel)
        {
            for (const auto& [name, us] : mGameState->profilerTimes())
            {
                mViewState->mProfiler.systemTimes[name] = static_cast<float>(us);
            }
        }
    }
}

void ClientGame::render()
{
    PROFILE_FUNCTION();
    const auto renderStartTime = std::chrono::high_resolution_clock::now();
    mViewState->render();
    const auto renderEndTime = std::chrono::high_resolution_clock::now();
    if (mViewState)
    {
        mViewState->mProfiler.renderTime = std::chrono::duration<float, std::milli>(renderEndTime - renderStartTime).count();
    }
}

void ClientGame::doAction(const Action& action)
{
    mViewState->doAction(action);
}
