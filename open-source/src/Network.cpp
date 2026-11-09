#include "Network.hpp"

#include "Debug.hpp"
#include "ENetTransport.hpp"
#include "GameConfig.hpp"


ClientNetwork::ClientNetwork(int id, Network* serverNetwork) : mId(id), mServerNetwork(serverNetwork)
{
}

ClientNetwork::~ClientNetwork()
{
    stop();
}

ClientNetwork::ClientNetwork(ClientNetwork&& other) noexcept :
    Network(std::move(other)),
    mId(other.mId),
    mServerNetwork(other.mServerNetwork),
    mIsConnected(other.mIsConnected.load(std::memory_order_relaxed)),
    mRtt(other.mRtt.load(std::memory_order_relaxed)),
    mIsDisconnected(other.mIsDisconnected),
    mExternalTransport(other.mExternalTransport),
    mTransport(std::move(other.mTransport))
{
}

ClientNetwork& ClientNetwork::operator=(ClientNetwork&& other) noexcept
{
    if (this != &other)
    {
        Network::operator=(std::move(other));
        mId            = other.mId;
        mServerNetwork = other.mServerNetwork;
        mIsConnected.store(other.mIsConnected.load(std::memory_order_relaxed), std::memory_order_relaxed);
        mRtt.store(other.mRtt.load(std::memory_order_relaxed), std::memory_order_relaxed);
        mIsDisconnected    = other.mIsDisconnected;
        mExternalTransport = other.mExternalTransport;
        mTransport         = std::move(other.mTransport);
    }
    return *this;
}

bool ClientNetwork::start(const std::string& host, uint16_t port, uint32_t targetHz)
{
    mIsDisconnected = false;
    if (mExternalTransport)
    {
        return false;
    }
    if (!mTransport)
    {
        mTransport = std::make_unique<ENetClient>();
    }
    if (!mTransport->isRunning())
    {
        logout("[ClientNetwork] Activating background network thread for " + host + ":" + std::to_string(port) + "...");
        return mTransport->start(host, port, targetHz);
    }
    return true;
}

void ClientNetwork::stop()
{
    mIsDisconnected = false;
    if (mExternalTransport)
    {
        return;
    }
    if (mTransport && mTransport->isRunning())
    {
        logout("[ClientNetwork] Deactivating network thread...");
        mTransport->sync(*this, mIsConnected);
        mTransport->stop();
        mIsConnected.store(false, std::memory_order_relaxed);
        mRtt.store(0, std::memory_order_relaxed);
    }
}

void ClientNetwork::sync()
{
    if (mTransport && mTransport->isRunning())
    {
        mTransport->sync(*this, mIsConnected);
        mRtt.store(mTransport->getRtt(), std::memory_order_relaxed);
    }
    else
    {
        mRtt.store(0, std::memory_order_relaxed);
    }
}

void ClientNetwork::sendPacket(Packet pkt)
{
    if (!mServerNetwork && mExternalTransport)
    {
        return;
    }

    constexpr uint64_t uplinkUs = (GameConfig::Network::kSimulatedPingMs / 2) * 1000;
    const uint64_t     now      = SimulationClock::nowUs();

    pkt.receiveTime = now + uplinkUs;
    pkt.clientId    = mId;
    pkt.srcAddress  = this;
    pkt.destAddress = mServerNetwork;
    outgoingPacket.push_back(std::move(pkt));
}

void ClientNetwork::sendDisconnect(const std::string& reason)
{
    if (!mIsDisconnected && isConnected())
    {
        mIsDisconnected = true;
        Packet pkt;
        pkt.type = PacketType::Disconnect;
        pkt.data = PacketDisconnect{mId, reason};
        sendPacket(std::move(pkt));
    }
}

bool ClientNetwork::isConnected() const
{
    return mIsConnected.load(std::memory_order_relaxed);
}

bool ClientNetwork::isDisconnected() const
{
    return mIsDisconnected;
}

void ClientNetwork::setDisconnected(bool disconnected)
{
    mIsDisconnected = disconnected;
}

void ClientNetwork::setConnected(bool connected)
{
    mIsConnected.store(connected, std::memory_order_relaxed);
}

uint32_t ClientNetwork::getRtt() const
{
    if (mExternalTransport)
    {
        return GameConfig::Network::kSimulatedPingMs;
    }
    return mRtt.load(std::memory_order_relaxed);
}

void ClientNetwork::setId(int id)
{
    mId = id;
}

int ClientNetwork::getId() const
{
    return mId;
}

void ClientNetwork::setServerNetwork(Network* serverNetwork)
{
    mServerNetwork = serverNetwork;
}

Network* ClientNetwork::getServerNetwork() const
{
    return mServerNetwork;
}

void ClientNetwork::setExternalTransport(bool ext)
{
    mExternalTransport = ext;
    if (ext)
    {
        mIsConnected.store(true, std::memory_order_relaxed);
    }
}

bool ClientNetwork::isExternalTransport() const
{
    return mExternalTransport;
}

bool ClientNetwork::isRunning() const
{
    return mTransport && mTransport->isRunning();
}


ServerNetwork::ServerNetwork() = default;

ServerNetwork::~ServerNetwork()
{
    stop();
}

ServerNetwork::ServerNetwork(ServerNetwork&&) noexcept            = default;
ServerNetwork& ServerNetwork::operator=(ServerNetwork&&) noexcept = default;

bool ServerNetwork::start(uint16_t port, size_t maxClients, uint32_t targetHz)
{
    if (mExternalTransport)
    {
        return false;
    }
    if (!mTransport)
    {
        mTransport = std::make_unique<ENetServer>();
    }
    if (!mTransport->isRunning())
    {
        logout("[ServerNetwork] Starting dedicated server network thread on port " + std::to_string(port) + "...");
        return mTransport->start(port, maxClients, targetHz);
    }
    return true;
}

void ServerNetwork::stop()
{
    if (mExternalTransport)
    {
        return;
    }
    if (mTransport && mTransport->isRunning())
    {
        logout("[ServerNetwork] Stopping server network thread...");
        mTransport->sync(*this);
        mTransport->stop();
    }
}

void ServerNetwork::sync()
{
    if (mTransport && mTransport->isRunning())
    {
        mTransport->sync(*this);
    }
}

void ServerNetwork::sendPacketToClient(Network* clientNetwork, int clientId, Packet pkt, uint64_t now)
{
    if (!clientNetwork)
    {
        return;
    }
    constexpr uint64_t downlinkUs = (GameConfig::Network::kSimulatedPingMs / 2) * 1000;
    pkt.srcAddress                = this;
    pkt.destAddress               = clientNetwork;
    pkt.clientId                  = clientId;
    pkt.receiveTime               = now + downlinkUs;
    outgoingPacket.push_back(std::move(pkt));
}

uint32_t ServerNetwork::getClientRtt(int clientId) const
{
    if (mExternalTransport)
    {
        return GameConfig::Network::kSimulatedPingMs;
    }
    if (mTransport && mTransport->isRunning())
    {
        return mTransport->getClientRtt(clientId);
    }
    return 0;
}

void ServerNetwork::setExternalTransport(bool ext)
{
    mExternalTransport = ext;
}

bool ServerNetwork::isExternalTransport() const
{
    return mExternalTransport;
}

bool ServerNetwork::isRunning() const
{
    return mTransport && mTransport->isRunning();
}

bool ServerNetwork::isAwake() const
{
    if (mTransport && mTransport->isRunning())
    {
        return mTransport->isAwake();
    }
    return true;
}

void ServerNetwork::waitForWakeup(uint32_t timeoutMs)
{
    if (mTransport && mTransport->isRunning())
    {
        mTransport->waitForWakeup(timeoutMs);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
    }
}
