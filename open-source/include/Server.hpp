#ifndef SERVER_HPP
#define SERVER_HPP

#include "GameConfig.hpp"
#include "Network.hpp"
#include "Packet.hpp"

#include <SFML/Graphics.hpp>

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <cstdint>

class Server
{
public:
    std::unique_ptr<sf::RenderWindow> mWindow;
    ServerNetwork                     mNetwork;
    sf::Font                          mFont;

    int               mClientCount = 3;
    bool              mGuiEnabled  = true;
    uint32_t          mTargetFps   = 256;
    std::atomic<bool> mRunning{true};

    void setExternalTransport(bool ext)
    {
        mNetwork.setExternalTransport(ext);
    }
    void setFramerateLimit(unsigned int limit)
    {
        mTargetFps = limit;
        if (mWindow)
        {
            mWindow->setFramerateLimit(limit);
        }
    }
    void               setGuiEnabled(bool enable);
    [[nodiscard]] bool isGuiEnabled() const;
    [[nodiscard]] bool isRunning() const
    {
        return mRunning.load();
    }
    void stop();

    bool               startNetwork(uint16_t port = 7777, size_t maxClients = 32, uint32_t targetHz = 256);
    void               stopNetwork();
    [[nodiscard]] bool isNetworkRunning() const;

    struct ConnectedClient
    {
        int         id = -1;
        std::string name;
        int         roomId         = -1;
        int         pingMs         = 20;
        std::string status         = "Lobby (Idle)";
        Network*    networkAddress = nullptr;
    };

    struct TickCommands
    {
        std::set<int>                       clientsReported;
        std::map<int, std::vector<Command>> clientCommands;
    };

    struct Room
    {
        int                               id           = -1;
        int                               hostClientId = -1;
        std::string                       name;
        std::vector<int>                  memberClientIds;
        std::map<int, bool>               readyStates;
        int                               level            = 1;
        int                               difficulty       = 1;
        int                               gameMode         = 0;
        bool                              inGame           = false;
        uint64_t                          nextTickToCommit = GameConfig::Game::kCommandDelayTicks;
        std::map<uint64_t, TickCommands>  pendingTicks;
        std::map<int, int64_t>            clientContiguousTick;
        std::map<int, std::set<uint64_t>> clientReceivedTicks;
        std::map<int, int64_t>            lastAckSentToClient;
        std::map<uint64_t, CommandOnTick> committedTicks;
        std::map<int, int64_t>            clientAckedServerTick;
        std::map<int, uint64_t>           lastSyncSentTimeUs;
        std::map<int, uint64_t>           lastSyncSentTick;
    };

    std::map<int, ConnectedClient> mConnectedClients;
    std::map<int, Room>            mRooms;
    int                            mNextRoomId   = 1;
    int                            mNextClientId = 0;
    std::vector<std::string>       mPacketLogs;

    sf::Clock                             mClock;
    std::chrono::steady_clock::time_point mLastFrameTime;

    explicit Server(int clientCount = 3, bool enableGui = true);
    ~Server();

    void run();
    void input();
    void network(uint64_t now);
    void update(uint64_t now);
    void render();

    void sendPacketToClient(int clientId, Packet pkt, uint64_t now);
    void broadcastPacket(Packet pkt, uint64_t now);

    void logTraffic(const std::string& log);
    void broadcastLobbySync(uint64_t now);
    void broadcastRoomSync(int roomId, uint64_t now, const std::string& notice = "");

    void dissolveRoom(int roomId, const std::string& reason, uint64_t now);
    void leaveCurrentRoom(int clientId, const std::string& reason, uint64_t now);
    void removeClient(int clientId, const std::string& reason, uint64_t now);
};

#endif
