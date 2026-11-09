#ifndef PACKET_SERIALIZER_HPP
#define PACKET_SERIALIZER_HPP

#include "Command.hpp"
#include "Packet.hpp"

#include <string>
#include <type_traits>
#include <vector>

#include <cstdint>
#include <cstring>

namespace NetSer
{

constexpr uint16_t kPacketMagic   = 0x474D;
constexpr uint8_t  kPacketVersion = 3;

class ByteWriter
{
public:
    std::vector<uint8_t> buffer;

    template <typename T>
    void write(const T& val)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        const auto* ptr = reinterpret_cast<const uint8_t*>(&val);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }

    void writeString(const std::string& str);
    void writeIntVector(const std::vector<int>& vec);
};

class ByteReader
{
public:
    const uint8_t* data   = nullptr;
    size_t         size   = 0;
    size_t         offset = 0;

    ByteReader(const uint8_t* d, size_t s) : data(d), size(s), offset(0)
    {
    }

    template <typename T>
    bool read(T& val)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        if (offset + sizeof(T) > size)
        {
            return false;
        }
        std::memcpy(&val, data + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }

    bool readString(std::string& str);
    bool readIntVector(std::vector<int>& vec);
};


void serializeCommandWithoutTick(ByteWriter& w, const Command& cmd);
bool deserializeCommandWithoutTick(ByteReader& r, uint64_t tickVal, Command& cmd);

void serializeCommand(ByteWriter& w, const Command& cmd);
bool deserializeCommand(ByteReader& r, Command& cmd);

void serializeCommandOnTick(ByteWriter& w, const CommandOnTick& cot);
bool deserializeCommandOnTick(ByteReader& r, CommandOnTick& cot);

std::vector<uint8_t> serializeCommandListPayload(const std::vector<Command>& commands);


void serializePacketData(ByteWriter& w, const PacketData& data);
bool deserializePacketData(ByteReader& r, PacketData& data);


std::vector<uint8_t> serializePacket(const Packet& pkt);
bool                 deserializePacket(const uint8_t* buffer, size_t size, Packet& outPkt);

} // namespace NetSer

#endif
