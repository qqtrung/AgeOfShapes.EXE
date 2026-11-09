#ifndef PACKET_HPP
#define PACKET_HPP

#include "Command.hpp"

#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <cstdint>

struct CommandOnTick
{
    size_t               tick = 0;
    std::vector<Command> commands;
};

std::string commandOnTickToString(const CommandOnTick& cot);

enum class PacketType : uint8_t
{
    None = 0,
    Ping,
    Pong,
    Connect,
    ConnectAck,
    Disconnect,
    Ready,
    GameStart,
    GameStartRequest,
    CommandSync,
    StateHash,

    LobbySync,
    RoomCreate,
    RoomJoin,
    RoomLeave,
    RoomInvite,
    RoomInviteNotify,
    RoomInviteResponse,
    RoomKick,
    RoomChangeSettings,
    RoomSync
};

struct PacketPing
{
    uint64_t timestamp = 0;
};

struct PacketPong
{
    uint64_t timestamp = 0;
};

struct PacketConnect
{
    std::string playerName;
    int         clientVersion = 1;
};

struct PacketConnectAck
{
    int  assignedPlayerId = -1;
    bool accepted         = true;
};

struct PacketDisconnect
{
    int         playerId = -1;
    std::string reason;
};

struct PacketReady
{
    int  playerId = -1;
    bool isReady  = true;
};

struct PacketGameStart
{
    uint32_t seed        = 0;
    int      level       = 1;
    int      difficulty  = 1;
    int      playerCount = 1;
    int      slotIndex   = 0;
    int      gameMode    = 0;
};

struct PacketGameStartRequest
{
    int roomId = -1;
};

struct PacketCommandSync
{
    int64_t                    ackTick = -1;
    std::vector<CommandOnTick> history;
};

struct PacketStateHash
{
    uint64_t tick = 0;
    uint64_t hash = 0;
};

struct PacketLobbyClientInfo
{
    int         id = -1;
    std::string name;
    int         roomId = -1;
    int         pingMs = 20;
    std::string status = "Idle";
};

struct PacketLobbyRoomInfo
{
    int         roomId = -1;
    std::string hostName;
    int         playerCount = 0;
    int         maxPlayers  = 3;
    int         level       = 1;
    int         difficulty  = 1;
    bool        inGame      = false;
};

struct PacketLobbySync
{
    std::vector<PacketLobbyClientInfo> clients;
    std::vector<PacketLobbyRoomInfo>   rooms;
};

struct PacketRoomCreate
{
    int         hostClientId = -1;
    std::string roomName;
};

struct PacketRoomJoin
{
    int roomId   = -1;
    int clientId = -1;
};

struct PacketRoomLeave
{
    int         roomId   = -1;
    int         clientId = -1;
    std::string reason;
};

struct PacketRoomInvite
{
    int roomId         = -1;
    int hostClientId   = -1;
    int targetClientId = -1;
};

struct PacketRoomInviteNotify
{
    int         roomId       = -1;
    int         hostClientId = -1;
    std::string hostName;
};

struct PacketRoomInviteResponse
{
    int  roomId         = -1;
    int  targetClientId = -1;
    bool accepted       = false;
};

struct PacketRoomKick
{
    int roomId         = -1;
    int targetClientId = -1;
};

struct PacketRoomChangeSettings
{
    int roomId     = -1;
    int clientId   = -1;
    int level      = 1;
    int difficulty = 1;
    int gameMode   = 0;
};

struct RoomSlotSync
{
    int         clientId = -1;
    std::string name;
    bool        isHost  = false;
    bool        isReady = false;
};

struct PacketRoomSync
{
    int                       roomId       = -1;
    int                       hostClientId = -1;
    int                       level        = 1;
    int                       difficulty   = 1;
    int                       gameMode     = 0;
    std::vector<RoomSlotSync> slots;
    bool                      inGame = false;
    std::string               notice;
};

using PacketData = std::variant<
    std::monostate,
    CommandOnTick,
    PacketPing,
    PacketPong,
    PacketConnect,
    PacketConnectAck,
    PacketDisconnect,
    PacketReady,
    PacketGameStart,
    PacketGameStartRequest,
    PacketCommandSync,
    PacketStateHash,
    PacketLobbySync,
    PacketRoomCreate,
    PacketRoomJoin,
    PacketRoomLeave,
    PacketRoomInvite,
    PacketRoomInviteNotify,
    PacketRoomInviteResponse,
    PacketRoomKick,
    PacketRoomChangeSettings,
    PacketRoomSync>;

struct Network;

struct Packet
{
    PacketType type{PacketType::None};
    PacketData data;
    uint64_t   tick        = 0;
    uint64_t   receiveTime = 0;
    int        clientId    = 0;
    Network*   srcAddress  = nullptr;
    Network*   destAddress = nullptr;

    Packet() = default;
    Packet(PacketType t,
           PacketData d,
           uint64_t   tick_        = 0,
           uint64_t   receiveTime_ = 0,
           int        clientId_    = 0,
           Network*   src_         = nullptr,
           Network*   dest_        = nullptr) :
        type(t),
        data(std::move(d)),
        tick(tick_),
        receiveTime(receiveTime_),
        clientId(clientId_),
        srcAddress(src_),
        destAddress(dest_)
    {
    }

    template <typename T>
    [[nodiscard]] const T* getIf() const
    {
        return std::get_if<T>(&data);
    }

    template <typename T>
    [[nodiscard]] T* getIf()
    {
        return std::get_if<T>(&data);
    }

    template <typename T>
    [[nodiscard]] bool holds() const
    {
        return std::holds_alternative<T>(data);
    }
};

#endif
