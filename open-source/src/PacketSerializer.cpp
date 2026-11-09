#include "PacketSerializer.hpp"

#include <algorithm>

#include <cmath>
#include <cstring>

namespace NetSer
{

void ByteWriter::writeString(const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.size());
    write(len);
    if (len > 0)
    {
        const auto* ptr = reinterpret_cast<const uint8_t*>(str.data());
        buffer.insert(buffer.end(), ptr, ptr + len);
    }
}

void ByteWriter::writeIntVector(const std::vector<int>& vec)
{
    uint32_t count = static_cast<uint32_t>(vec.size());
    write(count);
    for (int v : vec)
    {
        write(static_cast<int32_t>(v));
    }
}

bool ByteReader::readString(std::string& str)
{
    uint32_t len = 0;
    if (!read(len))
    {
        return false;
    }
    if (offset + len > size)
    {
        return false;
    }
    str.assign(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return true;
}

bool ByteReader::readIntVector(std::vector<int>& vec)
{
    uint32_t count = 0;
    if (!read(count))
    {
        return false;
    }
    if (offset + count * sizeof(int32_t) > size)
    {
        return false;
    }
    vec.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        int32_t val = 0;
        if (!read(val))
        {
            return false;
        }
        vec[i] = val;
    }
    return true;
}


void serializeCommandWithoutTick(ByteWriter& w, const Command& cmd)
{
    w.write(static_cast<uint8_t>(cmd.type()));

    std::visit(
        [&w](const auto& d)
        {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
            }
            else if constexpr (std::is_same_v<T, CmdMovePlayer>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                int8_t qX = static_cast<int8_t>(std::round(std::clamp(d.dirX, -1.f, 1.f) * 127.f));
                int8_t qY = static_cast<int8_t>(std::round(std::clamp(d.dirY, -1.f, 1.f) * 127.f));
                w.write(qX);
                w.write(qY);
            }
            else if constexpr (std::is_same_v<T, CmdPlaceWall>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.write(static_cast<uint8_t>(d.kind));
                w.write(d.worldX);
                w.write(d.worldY);
            }
            else if constexpr (std::is_same_v<T, CmdMoveWall>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.write(static_cast<int32_t>(d.wallId));
                w.write(d.worldX);
                w.write(d.worldY);
                w.write(static_cast<uint8_t>(d.releasing ? 1 : 0));
                w.write(static_cast<uint32_t>(d.wallGen));
            }
            else if constexpr (std::is_same_v<T, CmdPlayerAttack>)
            {
                w.write(static_cast<int16_t>(d.playerId));
            }
            else if constexpr (std::is_same_v<T, CmdPlayerSkill>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.write(static_cast<uint8_t>(d.skillId));
            }
            else if constexpr (std::is_same_v<T, CmdCombatMode>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.write(static_cast<uint8_t>(d.state));
            }
            else if constexpr (std::is_same_v<T, CmdSelectUnits>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.writeIntVector(d.unitIds);
            }
            else if constexpr (std::is_same_v<T, CmdOrderUnits>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.writeIntVector(d.unitIds);
                w.write(d.targetX);
                w.write(d.targetY);
                w.write(static_cast<int32_t>(d.targetEntityId));
                w.write(static_cast<uint32_t>(d.targetEntityGen));
            }
            else if constexpr (std::is_same_v<T, CmdSetGameSpeed>)
            {
                w.write(d.timeScale);
            }
            else if constexpr (std::is_same_v<T, CmdPlaceUnit>)
            {
                w.write(static_cast<int16_t>(d.playerId));
                w.write(static_cast<uint8_t>(d.unitClass));
                w.write(d.worldX);
                w.write(d.worldY);
            }
        },
        cmd.data());
}

bool deserializeCommandWithoutTick(ByteReader& r, uint64_t tickVal, Command& cmd)
{
    uint8_t typeVal = 0;

    if (!r.read(typeVal))
    {
        return false;
    }

    uint8_t     dataIdx = typeVal;
    CommandData data;
    switch (dataIdx)
    {
        case 0:
            data = std::monostate{};
            break;
        case 1:
        {
            CmdMovePlayer d;
            int16_t       pid = -1;
            int8_t        qX  = 0;
            int8_t        qY  = 0;
            if (!r.read(pid) || !r.read(qX) || !r.read(qY))
                return false;
            d.playerId = pid;
            d.dirX     = static_cast<float>(qX) / 127.f;
            d.dirY     = static_cast<float>(qY) / 127.f;
            data       = d;
            break;
        }
        case 2:
        {
            CmdPlaceWall d{};
            int16_t      pid     = -1;
            uint8_t      kindVal = 0;
            if (!r.read(pid) || !r.read(kindVal) || !r.read(d.worldX) || !r.read(d.worldY))
                return false;
            d.playerId = pid;
            d.kind     = static_cast<WallKind>(kindVal);
            data       = d;
            break;
        }
        case 3:
        {
            CmdMoveWall d{};
            int16_t     pid    = -1;
            int32_t     wallId = 0;
            uint8_t     rel    = 0;
            uint32_t    gen    = 0;
            if (!r.read(pid) || !r.read(wallId) || !r.read(d.worldX) || !r.read(d.worldY) || !r.read(rel) || !r.read(gen))
                return false;
            d.playerId  = pid;
            d.wallId    = wallId;
            d.releasing = (rel != 0);
            d.wallGen   = gen;
            data        = d;
            break;
        }
        case 4:
        {
            CmdPlayerAttack d;
            int16_t         pid = -1;
            if (!r.read(pid))
                return false;
            d.playerId = pid;
            data       = d;
            break;
        }
        case 5:
        {
            CmdPlayerSkill d;
            int16_t        pid = -1;
            uint8_t        sid = 1;
            if (!r.read(pid) || !r.read(sid))
                return false;
            d.playerId = pid;
            d.skillId  = sid;
            data       = d;
            break;
        }
        case 6:
        {
            CmdCombatMode d;
            int16_t       pid = -1;
            uint8_t       st  = 0;
            if (!r.read(pid) || !r.read(st))
                return false;
            d.playerId = pid;
            d.state    = static_cast<CombatStance>(st);
            data       = d;
            break;
        }
        case 7:
        {
            CmdSelectUnits d;
            int16_t        pid = -1;
            if (!r.read(pid) || !r.readIntVector(d.unitIds))
                return false;
            d.playerId = pid;
            data       = std::move(d);
            break;
        }
        case 8:
        {
            CmdOrderUnits d;
            int16_t       pid         = -1;
            int32_t       targetEntId = -1;
            uint32_t      targetGen   = 0;
            if (!r.read(pid) || !r.readIntVector(d.unitIds) || !r.read(d.targetX) || !r.read(d.targetY) ||
                !r.read(targetEntId) || !r.read(targetGen))
                return false;
            d.playerId        = pid;
            d.targetEntityId  = targetEntId;
            d.targetEntityGen = targetGen;
            data              = std::move(d);
            break;
        }
        case 9:
        {
            CmdSetGameSpeed d;
            if (!r.read(d.timeScale))
                return false;
            data = d;
            break;
        }
        case 10:
        {
            CmdPlaceUnit d{};
            int16_t      pid      = -1;
            uint8_t      classVal = 0;
            if (!r.read(pid) || !r.read(classVal) || !r.read(d.worldX) || !r.read(d.worldY))
                return false;
            if (classVal < static_cast<uint8_t>(UnitClass::Warrior) || classVal > static_cast<uint8_t>(UnitClass::Berserker))
                return false;
            d.playerId  = pid;
            d.unitClass = static_cast<UnitClass>(classVal);
            data        = d;
            break;
        }
        default:
            return false;
    }

    cmd = Command(static_cast<CommandType>(typeVal), std::move(data), tickVal);
    return true;
}

void serializeCommand(ByteWriter& w, const Command& cmd)
{
    w.write(static_cast<uint64_t>(cmd.tick()));
    serializeCommandWithoutTick(w, cmd);
}

bool deserializeCommand(ByteReader& r, Command& cmd)
{
    uint64_t tickVal = 0;
    if (!r.read(tickVal))
    {
        return false;
    }
    return deserializeCommandWithoutTick(r, tickVal, cmd);
}

void serializeCommandOnTick(ByteWriter& w, const CommandOnTick& cot)
{
    w.write(static_cast<uint64_t>(cot.tick));
    uint32_t count = static_cast<uint32_t>(cot.commands.size());
    w.write(count);
    for (const auto& c : cot.commands)
    {
        serializeCommand(w, c);
    }
}

bool deserializeCommandOnTick(ByteReader& r, CommandOnTick& cot)
{
    uint64_t tickVal = 0;
    uint32_t count   = 0;
    if (!r.read(tickVal) || !r.read(count))
    {
        return false;
    }
    cot.tick = tickVal;
    cot.commands.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        if (!deserializeCommand(r, cot.commands[i]))
        {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> serializeCommandListPayload(const std::vector<Command>& commands)
{
    ByteWriter cw;
    uint8_t    count = static_cast<uint8_t>(std::min<size_t>(commands.size(), 255));
    cw.write(count);
    for (size_t i = 0; i < count; ++i)
    {
        serializeCommandWithoutTick(cw, commands[i]);
    }
    return cw.buffer;
}


void serializePacketData(ByteWriter& w, const PacketData& data)
{
    uint8_t idx = static_cast<uint8_t>(data.index());
    w.write(idx);

    std::visit(
        [&w](const auto& d)
        {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, std::monostate>)
            {
            }
            else if constexpr (std::is_same_v<T, CommandOnTick>)
            {
                serializeCommandOnTick(w, d);
            }
            else if constexpr (std::is_same_v<T, PacketPing>)
            {
                w.write(static_cast<uint64_t>(d.timestamp));
            }
            else if constexpr (std::is_same_v<T, PacketPong>)
            {
                w.write(static_cast<uint64_t>(d.timestamp));
            }
            else if constexpr (std::is_same_v<T, PacketConnect>)
            {
                w.writeString(d.playerName);
                w.write(static_cast<int32_t>(d.clientVersion));
            }
            else if constexpr (std::is_same_v<T, PacketConnectAck>)
            {
                w.write(static_cast<int32_t>(d.assignedPlayerId));
                w.write(static_cast<uint8_t>(d.accepted ? 1 : 0));
            }
            else if constexpr (std::is_same_v<T, PacketDisconnect>)
            {
                w.write(static_cast<int32_t>(d.playerId));
                w.writeString(d.reason);
            }
            else if constexpr (std::is_same_v<T, PacketReady>)
            {
                w.write(static_cast<int32_t>(d.playerId));
                w.write(static_cast<uint8_t>(d.isReady ? 1 : 0));
            }
            else if constexpr (std::is_same_v<T, PacketGameStart>)
            {
                w.write(static_cast<uint32_t>(d.seed));
                w.write(static_cast<int32_t>(d.level));
                w.write(static_cast<int32_t>(d.difficulty));
                w.write(static_cast<int32_t>(d.playerCount));
                w.write(static_cast<int32_t>(d.slotIndex));
                w.write(static_cast<int32_t>(d.gameMode));
            }
            else if constexpr (std::is_same_v<T, PacketGameStartRequest>)
            {
                w.write(static_cast<int32_t>(d.roomId));
            }
            else if constexpr (std::is_same_v<T, PacketCommandSync>)
            {
                w.write(static_cast<int32_t>(d.ackTick));

                struct RleRun
                {
                    uint32_t             startTick = 0;
                    uint8_t              runLength = 0;
                    std::vector<uint8_t> cmdPayload;
                };

                std::vector<RleRun> runs;
                for (const auto& cot : d.history)
                {
                    std::vector<uint8_t> payload = serializeCommandListPayload(cot.commands);
                    if (!runs.empty() &&
                        static_cast<uint64_t>(cot.tick) ==
                            static_cast<uint64_t>(runs.back().startTick) + runs.back().runLength &&
                        runs.back().runLength < 255 && runs.back().cmdPayload == payload)
                    {
                        runs.back().runLength++;
                    }
                    else
                    {
                        runs.push_back(RleRun{static_cast<uint32_t>(cot.tick), 1, std::move(payload)});
                    }
                }

                uint8_t runCount = static_cast<uint8_t>(std::min<size_t>(runs.size(), 255));
                w.write(runCount);
                for (const auto& run : runs)
                {
                    w.write(run.startTick);
                    w.write(run.runLength);
                    w.buffer.insert(w.buffer.end(), run.cmdPayload.begin(), run.cmdPayload.end());
                }
            }
            else if constexpr (std::is_same_v<T, PacketStateHash>)
            {
                w.write(static_cast<uint64_t>(d.tick));
                w.write(static_cast<uint64_t>(d.hash));
            }
            else if constexpr (std::is_same_v<T, PacketLobbySync>)
            {
                uint32_t cCount = static_cast<uint32_t>(d.clients.size());
                w.write(cCount);
                for (const auto& c : d.clients)
                {
                    w.write(static_cast<int32_t>(c.id));
                    w.writeString(c.name);
                    w.write(static_cast<int32_t>(c.roomId));
                    w.write(static_cast<int32_t>(c.pingMs));
                    w.writeString(c.status);
                }

                uint32_t rCount = static_cast<uint32_t>(d.rooms.size());
                w.write(rCount);
                for (const auto& r : d.rooms)
                {
                    w.write(static_cast<int32_t>(r.roomId));
                    w.writeString(r.hostName);
                    w.write(static_cast<int32_t>(r.playerCount));
                    w.write(static_cast<int32_t>(r.maxPlayers));
                    w.write(static_cast<int32_t>(r.level));
                    w.write(static_cast<int32_t>(r.difficulty));
                    w.write(static_cast<uint8_t>(r.inGame ? 1 : 0));
                }
            }
            else if constexpr (std::is_same_v<T, PacketRoomCreate>)
            {
                w.write(static_cast<int32_t>(d.hostClientId));
                w.writeString(d.roomName);
            }
            else if constexpr (std::is_same_v<T, PacketRoomJoin>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.clientId));
            }
            else if constexpr (std::is_same_v<T, PacketRoomLeave>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.clientId));
                w.writeString(d.reason);
            }
            else if constexpr (std::is_same_v<T, PacketRoomInvite>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.hostClientId));
                w.write(static_cast<int32_t>(d.targetClientId));
            }
            else if constexpr (std::is_same_v<T, PacketRoomInviteNotify>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.hostClientId));
                w.writeString(d.hostName);
            }
            else if constexpr (std::is_same_v<T, PacketRoomInviteResponse>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.targetClientId));
                w.write(static_cast<uint8_t>(d.accepted ? 1 : 0));
            }
            else if constexpr (std::is_same_v<T, PacketRoomKick>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.targetClientId));
            }
            else if constexpr (std::is_same_v<T, PacketRoomChangeSettings>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.clientId));
                w.write(static_cast<int32_t>(d.level));
                w.write(static_cast<int32_t>(d.difficulty));
                w.write(static_cast<int32_t>(d.gameMode));
            }
            else if constexpr (std::is_same_v<T, PacketRoomSync>)
            {
                w.write(static_cast<int32_t>(d.roomId));
                w.write(static_cast<int32_t>(d.hostClientId));
                w.write(static_cast<int32_t>(d.level));
                w.write(static_cast<int32_t>(d.difficulty));
                w.write(static_cast<int32_t>(d.gameMode));

                uint32_t sCount = static_cast<uint32_t>(d.slots.size());
                w.write(sCount);
                for (const auto& s : d.slots)
                {
                    w.write(static_cast<int32_t>(s.clientId));
                    w.writeString(s.name);
                    w.write(static_cast<uint8_t>(s.isHost ? 1 : 0));
                    w.write(static_cast<uint8_t>(s.isReady ? 1 : 0));
                }

                w.write(static_cast<uint8_t>(d.inGame ? 1 : 0));
                w.writeString(d.notice);
            }
        },
        data);
}

bool deserializePacketData(ByteReader& r, PacketData& data)
{
    uint8_t idx = 0;
    if (!r.read(idx))
    {
        return false;
    }

    switch (idx)
    {
        case 0:
            data = std::monostate{};
            break;
        case 1:
        {
            CommandOnTick d;
            if (!deserializeCommandOnTick(r, d))
                return false;
            data = std::move(d);
            break;
        }
        case 2:
        {
            PacketPing d;
            uint64_t   ts = 0;
            if (!r.read(ts))
                return false;
            d.timestamp = ts;
            data        = d;
            break;
        }
        case 3:
        {
            PacketPong d;
            uint64_t   ts = 0;
            if (!r.read(ts))
                return false;
            d.timestamp = ts;
            data        = d;
            break;
        }
        case 4:
        {
            PacketConnect d;
            int32_t       ver = 0;
            if (!r.readString(d.playerName) || !r.read(ver))
                return false;
            d.clientVersion = ver;
            data            = std::move(d);
            break;
        }
        case 5:
        {
            PacketConnectAck d;
            int32_t          pid = 0;
            uint8_t          acc = 0;
            if (!r.read(pid) || !r.read(acc))
                return false;
            d.assignedPlayerId = pid;
            d.accepted         = (acc != 0);
            data               = d;
            break;
        }
        case 6:
        {
            PacketDisconnect d;
            int32_t          pid = 0;
            if (!r.read(pid) || !r.readString(d.reason))
                return false;
            d.playerId = pid;
            data       = std::move(d);
            break;
        }
        case 7:
        {
            PacketReady d;
            int32_t     pid   = 0;
            uint8_t     ready = 0;
            if (!r.read(pid) || !r.read(ready))
                return false;
            d.playerId = pid;
            d.isReady  = (ready != 0);
            data       = d;
            break;
        }
        case 8:
        {
            PacketGameStart d;
            uint32_t        seed = 0;
            int32_t         lvl = 0, diff = 0, pCount = 0, slot = 0, gmode = 0;
            if (!r.read(seed) || !r.read(lvl) || !r.read(diff) || !r.read(pCount) || !r.read(slot) || !r.read(gmode))
                return false;
            d.seed        = seed;
            d.level       = lvl;
            d.difficulty  = diff;
            d.playerCount = pCount;
            d.slotIndex   = slot;
            d.gameMode    = gmode;
            data          = d;
            break;
        }
        case 9:
        {
            PacketGameStartRequest d;
            int32_t                rid = 0;
            if (!r.read(rid))
                return false;
            d.roomId = rid;
            data     = d;
            break;
        }
        case 10:
        {
            PacketCommandSync d;
            int32_t           ack      = -1;
            uint8_t           runCount = 0;
            if (!r.read(ack) || !r.read(runCount))
                return false;

            d.ackTick = static_cast<int64_t>(ack);
            for (uint8_t i = 0; i < runCount; ++i)
            {
                uint32_t startTick = 0;
                uint8_t  runLength = 0;
                uint8_t  cmdCount  = 0;
                if (!r.read(startTick) || !r.read(runLength) || !r.read(cmdCount))
                    return false;
                if (runLength == 0)
                    return false;
                if (d.history.size() + runLength > 8192)
                    return false;

                std::vector<Command> baseCmds(cmdCount);
                for (uint8_t c = 0; c < cmdCount; ++c)
                {
                    if (!deserializeCommandWithoutTick(r, static_cast<uint64_t>(startTick), baseCmds[c]))
                        return false;
                }

                for (uint16_t offset = 0; offset < runLength; ++offset)
                {
                    CommandOnTick cot;
                    cot.tick     = static_cast<size_t>(startTick + offset);
                    cot.commands = baseCmds;
                    if (offset > 0)
                    {
                        for (auto& cmd : cot.commands)
                        {
                            cmd.setTick(cot.tick);
                        }
                    }
                    d.history.push_back(std::move(cot));
                }
            }
            data = std::move(d);
            break;
        }
        case 11:
        {
            PacketStateHash d;
            uint64_t        t = 0, h = 0;
            if (!r.read(t) || !r.read(h))
                return false;
            d.tick = t;
            d.hash = h;
            data   = d;
            break;
        }
        case 12:
        {
            PacketLobbySync d;
            uint32_t        cCount = 0;
            if (!r.read(cCount))
                return false;
            d.clients.resize(cCount);
            for (uint32_t i = 0; i < cCount; ++i)
            {
                int32_t id = 0, roomId = 0, pingMs = 0;
                if (!r.read(id) || !r.readString(d.clients[i].name) || !r.read(roomId) || !r.read(pingMs) ||
                    !r.readString(d.clients[i].status))
                    return false;
                d.clients[i].id     = id;
                d.clients[i].roomId = roomId;
                d.clients[i].pingMs = pingMs;
            }

            uint32_t rCount = 0;
            if (!r.read(rCount))
                return false;
            d.rooms.resize(rCount);
            for (uint32_t i = 0; i < rCount; ++i)
            {
                int32_t rid = 0, pCount = 0, maxP = 0, lvl = 0, diff = 0;
                uint8_t inG = 0;
                if (!r.read(rid) || !r.readString(d.rooms[i].hostName) || !r.read(pCount) || !r.read(maxP) ||
                    !r.read(lvl) || !r.read(diff) || !r.read(inG))
                    return false;
                d.rooms[i].roomId      = rid;
                d.rooms[i].playerCount = pCount;
                d.rooms[i].maxPlayers  = maxP;
                d.rooms[i].level       = lvl;
                d.rooms[i].difficulty  = diff;
                d.rooms[i].inGame      = (inG != 0);
            }
            data = std::move(d);
            break;
        }
        case 13:
        {
            PacketRoomCreate d;
            int32_t          hid = 0;
            if (!r.read(hid) || !r.readString(d.roomName))
                return false;
            d.hostClientId = hid;
            data           = std::move(d);
            break;
        }
        case 14:
        {
            PacketRoomJoin d;
            int32_t        rid = 0, cid = 0;
            if (!r.read(rid) || !r.read(cid))
                return false;
            d.roomId   = rid;
            d.clientId = cid;
            data       = d;
            break;
        }
        case 15:
        {
            PacketRoomLeave d;
            int32_t         rid = 0, cid = 0;
            if (!r.read(rid) || !r.read(cid) || !r.readString(d.reason))
                return false;
            d.roomId   = rid;
            d.clientId = cid;
            data       = std::move(d);
            break;
        }
        case 16:
        {
            PacketRoomInvite d;
            int32_t          rid = 0, hid = 0, tid = 0;
            if (!r.read(rid) || !r.read(hid) || !r.read(tid))
                return false;
            d.roomId         = rid;
            d.hostClientId   = hid;
            d.targetClientId = tid;
            data             = d;
            break;
        }
        case 17:
        {
            PacketRoomInviteNotify d;
            int32_t                rid = 0, hid = 0;
            if (!r.read(rid) || !r.read(hid) || !r.readString(d.hostName))
                return false;
            d.roomId       = rid;
            d.hostClientId = hid;
            data           = std::move(d);
            break;
        }
        case 18:
        {
            PacketRoomInviteResponse d;
            int32_t                  rid = 0, tid = 0;
            uint8_t                  acc = 0;
            if (!r.read(rid) || !r.read(tid) || !r.read(acc))
                return false;
            d.roomId         = rid;
            d.targetClientId = tid;
            d.accepted       = (acc != 0);
            data             = d;
            break;
        }
        case 19:
        {
            PacketRoomKick d;
            int32_t        rid = 0, tid = 0;
            if (!r.read(rid) || !r.read(tid))
                return false;
            d.roomId         = rid;
            d.targetClientId = tid;
            data             = d;
            break;
        }
        case 20:
        {
            PacketRoomChangeSettings d;
            int32_t                  rid = 0, cid = 0, lvl = 0, diff = 0, gmode = 0;
            if (!r.read(rid) || !r.read(cid) || !r.read(lvl) || !r.read(diff) || !r.read(gmode))
                return false;
            d.roomId     = rid;
            d.clientId   = cid;
            d.level      = lvl;
            d.difficulty = diff;
            d.gameMode   = gmode;
            data         = d;
            break;
        }
        case 21:
        {
            PacketRoomSync d;
            int32_t        rid = 0, hid = 0, lvl = 0, diff = 0, gmode = 0;
            uint32_t       sCount = 0;
            if (!r.read(rid) || !r.read(hid) || !r.read(lvl) || !r.read(diff) || !r.read(gmode) || !r.read(sCount))
                return false;
            d.roomId       = rid;
            d.hostClientId = hid;
            d.level        = lvl;
            d.difficulty   = diff;
            d.gameMode     = gmode;
            d.slots.resize(sCount);
            for (uint32_t i = 0; i < sCount; ++i)
            {
                int32_t cid = 0;
                uint8_t isH = 0, isR = 0;
                if (!r.read(cid) || !r.readString(d.slots[i].name) || !r.read(isH) || !r.read(isR))
                    return false;
                d.slots[i].clientId = cid;
                d.slots[i].isHost   = (isH != 0);
                d.slots[i].isReady  = (isR != 0);
            }
            uint8_t inG = 0;
            if (!r.read(inG) || !r.readString(d.notice))
                return false;
            d.inGame = (inG != 0);
            data     = std::move(d);
            break;
        }
        default:
            return false;
    }
    return true;
}

std::vector<uint8_t> serializePacket(const Packet& pkt)
{
    ByteWriter w;
    w.write(kPacketMagic);
    w.write(kPacketVersion);
    w.write(static_cast<uint8_t>(pkt.type));

    serializePacketData(w, pkt.data);
    return w.buffer;
}

bool deserializePacket(const uint8_t* buffer, size_t size, Packet& outPkt)
{
    ByteReader r(buffer, size);

    uint16_t magic   = 0;
    uint8_t  version = 0;
    if (!r.read(magic) || !r.read(version))
    {
        return false;
    }
    if (magic != kPacketMagic || version != kPacketVersion)
    {
        return false;
    }

    uint8_t typeVal = 0;

    if (!r.read(typeVal))
    {
        return false;
    }

    PacketData data;
    if (!deserializePacketData(r, data))
    {
        return false;
    }

    outPkt.type     = static_cast<PacketType>(typeVal);
    outPkt.tick     = 0;
    outPkt.clientId = -1;
    outPkt.data     = std::move(data);

    return true;
}

} // namespace NetSer
