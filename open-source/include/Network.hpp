#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "Packet.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include <atomic>
#include <list>
#include <memory>
#include <string>

#include <cstdint>

struct SimulationClock
{
    static sf::Clock& getInstance()
    {
        static sf::Clock sClock;
        return sClock;
    }

    static uint64_t nowUs()
    {
        return static_cast<uint64_t>(getInstance().getElapsedTime().asMicroseconds());
    }
};

struct Network
{
    std::list<Packet> ingoingPacket;
    std::list<Packet> outgoingPacket;

    virtual ~Network() = default;

    void sendPacket(Packet pkt)
    {
        outgoingPacket.push_back(std::move(pkt));
    }

    void clear()
    {
        ingoingPacket.clear();
        outgoingPacket.clear();
    }

    [[nodiscard]] virtual uint32_t getRtt() const
    {
        return 0;
    }
};

struct NetworkRouter
{
    static void route(Network& node, uint64_t now)
    {
        while (!node.outgoingPacket.empty())
        {
            if (now < node.outgoingPacket.front().receiveTime)
            {
                break;
            }
            Packet pkt = std::move(node.outgoingPacket.front());
            node.outgoingPacket.pop_front();

            if (pkt.destAddress)
            {
                pkt.destAddress->ingoingPacket.push_back(std::move(pkt));
            }
        }
    }
};

class ENetClient;
class ENetServer;

class ClientNetwork : public Network
{
public:
    explicit ClientNetwork(int id = 0, Network* serverNetwork = nullptr);
    ~ClientNetwork() override;

    ClientNetwork(const ClientNetwork&)            = delete;
    ClientNetwork& operator=(const ClientNetwork&) = delete;
    ClientNetwork(ClientNetwork&&) noexcept;
    ClientNetwork& operator=(ClientNetwork&&) noexcept;

    bool start(const std::string& host = "geometrylegion.ddns.net", uint16_t port = 7777, uint32_t targetHz = 256);
    void stop();
    void sync();

    void sendPacket(Packet pkt);
    void sendDisconnect(const std::string& reason = "Client window closed");

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isDisconnected() const;
    void               setDisconnected(bool disconnected);
    void               setConnected(bool connected);

    [[nodiscard]] uint32_t getRtt() const override;

    void              setId(int id);
    [[nodiscard]] int getId() const;

    void                   setServerNetwork(Network* serverNetwork);
    [[nodiscard]] Network* getServerNetwork() const;

    void               setExternalTransport(bool ext);
    [[nodiscard]] bool isExternalTransport() const;

    [[nodiscard]] bool isRunning() const;

private:
    int                         mId            = 0;
    Network*                    mServerNetwork = nullptr;
    std::atomic<bool>           mIsConnected{false};
    std::atomic<uint32_t>       mRtt{0};
    bool                        mIsDisconnected    = false;
    bool                        mExternalTransport = false;
    std::unique_ptr<ENetClient> mTransport;
};

class ServerNetwork : public Network
{
public:
    ServerNetwork();
    ~ServerNetwork() override;

    ServerNetwork(const ServerNetwork&)            = delete;
    ServerNetwork& operator=(const ServerNetwork&) = delete;
    ServerNetwork(ServerNetwork&&) noexcept;
    ServerNetwork& operator=(ServerNetwork&&) noexcept;

    bool start(uint16_t port = 7777, size_t maxClients = 32, uint32_t targetHz = 256);
    void stop();
    void sync();

    void sendPacketToClient(Network* clientNetwork, int clientId, Packet pkt, uint64_t now);

    [[nodiscard]] uint32_t getClientRtt(int clientId) const;

    void               setExternalTransport(bool ext);
    [[nodiscard]] bool isExternalTransport() const;

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isAwake() const;
    void               waitForWakeup(uint32_t timeoutMs = 100);

private:
    bool                        mExternalTransport = false;
    std::unique_ptr<ENetServer> mTransport;
};

#endif
