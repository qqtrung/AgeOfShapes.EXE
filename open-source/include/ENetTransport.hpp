#ifndef ENET_TRANSPORT_HPP
#define ENET_TRANSPORT_HPP

#include "Network.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <cstdint>

struct _ENetHost;
struct _ENetPeer;

class ENetSystem
{
public:
    static bool initialize();
    static void deinitialize();
    static bool isInitialized();
};

class ENetServer
{
public:
    ENetServer();
    ~ENetServer();

    bool               start(uint16_t port = 7777, size_t maxClients = 32, uint32_t targetHz = 256);
    void               stop();
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isAwake() const;
    void               waitForWakeup(uint32_t timeoutMs = 100);

    void sync(Network& serverNetwork);

    [[nodiscard]] uint32_t getClientRtt(int clientId) const;

private:
    void threadLoop(uint16_t port, size_t maxClients, uint32_t targetHz);

    std::atomic<bool>                 mRunning{false};
    std::atomic<bool>                 mIsAwake{false};
    std::condition_variable           mWakeupCv;
    std::mutex                        mWakeupMutex;
    std::thread                       mThread;
    mutable std::mutex                mMutex;
    std::list<Packet>                 mIncomingStaging;
    std::list<Packet>                 mOutgoingStaging;
    std::unordered_map<int, uint32_t> mClientRtt;
};

class ENetClient
{
public:
    ENetClient();
    ~ENetClient();

    bool start(const std::string& host = "geometrylegion.ddns.net", uint16_t port = 7777, uint32_t targetHz = 256);
    void stop();
    [[nodiscard]] bool     isRunning() const;
    [[nodiscard]] bool     isConnected() const;
    [[nodiscard]] uint32_t getRtt() const;

    void sync(Network& gameNetwork, std::atomic<bool>& isConnectedFlag);
    void sync(Network& gameNetwork, bool& isConnectedFlag);

private:
    void threadLoop(std::string host, uint16_t port, uint32_t targetHz);

    std::atomic<bool>     mRunning{false};
    std::atomic<bool>     mIsConnected{false};
    std::atomic<uint32_t> mRtt{0};
    std::thread           mThread;
    std::mutex            mMutex;
    std::list<Packet>     mIncomingStaging;
    std::list<Packet>     mOutgoingStaging;
};

using ThreadedENetServer = ENetServer;
using ThreadedENetClient = ENetClient;

#endif
