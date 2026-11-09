#ifndef CLIENTGAME_HPP
#define CLIENTGAME_HPP

#include "Action.hpp"
#include "Game.hpp"
#include "Packet.hpp"
#include "Scene.hpp"

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <queue>

class GameState;
class ViewState;

class ClientGame : public Scene
{
    std::shared_ptr<GameState> mGameState;

public:
    [[nodiscard]] const GameState& gameState() const;

    bool isReplay        = false;
    bool mDesyncDetected = false;
    int  mId             = -1;
    int  mNumClients     = 1;

    std::unique_ptr<sf::RenderWindow> mWindow;

    sf::RenderWindow& window() override;

    std::shared_ptr<ViewState> mViewState;
    std::queue<CommandOnTick>  mCommandQueue;

    std::list<Packet> ingoingPacket;
    std::list<Packet> outgoingPacket;

    std::map<uint64_t, CommandOnTick> mUnackedOutgoingCommands;
    std::map<uint64_t, CommandOnTick> mIncomingReorderBuffer;
    int64_t                           mServerAckedTick = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
    int64_t  mLastContiguousServerTick                 = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
    int64_t  mLastAckSentToServer                      = static_cast<int64_t>(GameConfig::Game::kCommandDelayTicks) - 1;
    uint64_t mLastSyncSentTimeUs                       = 0;
    uint64_t mLastSyncSentTick                         = 0;
    bool     mNeedSendCommandSync                      = false;

    ClientGame(Game*              game,
               const std::string& path,
               int                level      = 1,
               int                difficulty = 1,
               bool               replay     = false,
               int                id         = 0,
               int                numClients = 1,
               int                map        = 1,
               int                gameMode   = 0);

    void pushCommand(CommandOnTick packet);
    void queueOutgoingCommand(CommandOnTick packet);
    void sendRedundantCommands();
    void processIncomingServerCommand(const CommandOnTick& cot);

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;

    [[nodiscard]] bool isGameOver() const;

    void doAction(const Action& action) override;
};

#endif
