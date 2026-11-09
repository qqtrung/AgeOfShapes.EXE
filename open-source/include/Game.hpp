#ifndef GAME_HPP
#define GAME_HPP

#include "Action.hpp"
#include "Assets.hpp"
#include "GameConfig.hpp"
#include "Network.hpp"
#include "Packet.hpp"
#include "Queue.hpp"
#include "Scene.hpp"

#include <atomic>
#include <list>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

struct FPSMeter;

struct Game
{
    int           mId = 0;
    ClientNetwork mNetwork;

    std::string mServerHost = GameConfig::Network::kDefaultHost;
    uint16_t    mServerPort = GameConfig::Network::kDefaultPort;

    void setServerAddress(const std::string& host, uint16_t port = GameConfig::Network::kDefaultPort)
    {
        mServerHost = host;
        mServerPort = port;
    }

    void setExternalTransport(bool ext)
    {
        mNetwork.setExternalTransport(ext);
    }
    void startNetwork(const std::string& host = "", uint16_t port = 0);
    void stopNetwork();

    std::stack<std::unique_ptr<Scene>> m_scenes;
    std::unique_ptr<Scene>             m_pendingScene;
    size_t                             m_countPopScenes = 0;

    sf::RenderWindow          mWindow;
    sf::View                  mUIView{sf::FloatRect({0.f, 0.f}, {2532.f, 1170.f})};
    Assets                    mAssets;
    std::unique_ptr<FPSMeter> mFpsMeter;

    int       mNightLightMode{2};
    sf::Color mNightLightColor{255, 180, 107, 255};

    [[nodiscard]] sf::View& uiView()
    {
        return mUIView;
    }
    [[nodiscard]] const sf::View& uiView() const
    {
        return mUIView;
    }

    struct ReplayRecord
    {
        int                        level       = 1;
        int                        difficulty  = 1;
        int                        playerCount = 1;
        int                        map         = 1;
        int                        gameMode    = 0;
        uint64_t                   totalTicks  = 0;
        std::vector<CommandOnTick> commands;
        bool                       hasReplay = false;
    };
    ReplayRecord mLatestReplay;

    std::queue<CommandOnTick> mCommands;

    std::vector<std::string> mStateHistory;
    std::vector<std::string> mCommandHistory;
    uint64_t                 mReplayTotalTicks  = 0;
    int                      mReplayPlayerCount = 1;

    sf::Clock mClockGui;
    sf::Clock mClockTick;
    uint64_t  m_lag  = 0;
    uint64_t  m_prev = 0;

    int  mLockstepBufferSize = -1;
    void setLockstepBufferSize(int size)
    {
        mLockstepBufferSize = size;
    }
    [[nodiscard]] int lockstepBufferSize() const
    {
        return mLockstepBufferSize;
    }

    int mGraphicsLevel = 2;

    Game(const std::string& path, int id = 0, Network* serverNetwork = nullptr);
    ~Game();

    void init(const std::string& path);
    void run();
    void update();
    void render();
    void input();
    void network();
    void quit();

    void               sendPacket(Packet pkt);
    [[nodiscard]] bool isConnected() const
    {
        return mNetwork.isConnected();
    }

    [[nodiscard]] sf::RenderWindow& window()
    {
        return mWindow;
    }

    void setFramerateLimit(unsigned int limit)
    {
        mWindow.setFramerateLimit(limit);
    }

    void pushScene(std::unique_ptr<Scene> scene);
    void changeScene(std::unique_ptr<Scene> scene);
    void popScene();

    void pushScene(const std::string& name,
                   int                level      = 1,
                   int                difficulty = 1,
                   int                numClients = 1,
                   int                slotIndex  = 0,
                   int                map        = 1,
                   int                gameMode   = 0);
    void changeScene(const std::string& name,
                     int                level      = 1,
                     int                difficulty = 1,
                     int                numClients = 1,
                     int                slotIndex  = 0,
                     int                map        = 1,
                     int                gameMode   = 0);

    std::unique_ptr<Scene> createScene(
        const std::string& name,
        int                level      = 1,
        int                difficulty = 1,
        int                numClients = 1,
        int                slotIndex  = 0,
        int                map        = 1,
        int                gameMode   = 0);
};

#endif
