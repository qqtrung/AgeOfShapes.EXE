#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "Components.hpp"

#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <cstdint>

enum class CommandType
{
    NoOp,
    MovePlayer,
    PlaceWall,
    MoveWall,
    PlayerAttack,
    PlayerSkill,
    CombatMode,
    SelectUnits,
    OrderUnits,
    SetGameSpeed,
    PlaceUnit,
};

struct CmdMovePlayer
{
    int   playerId = -1;
    float dirX     = 0.f;
    float dirY     = 0.f;
};

struct CmdPlaceWall
{
    int      playerId = -1;
    WallKind kind;
    float    worldX;
    float    worldY;
};

struct CmdMoveWall
{
    int      playerId = -1;
    int      wallId;
    float    worldX;
    float    worldY;
    bool     releasing;
    uint32_t wallGen = 0;
};

struct CmdPlayerAttack
{
    int playerId = -1;
};

struct CmdPlayerSkill
{
    int playerId = -1;
    int skillId  = 1;
};

struct CmdCombatMode
{
    int          playerId = -1;
    CombatStance state    = CombatStance::Attack;
};

struct CmdSelectUnits
{
    int              playerId = -1;
    std::vector<int> unitIds;
};

struct CmdOrderUnits
{
    int              playerId = -1;
    std::vector<int> unitIds;
    float            targetX         = 0.f;
    float            targetY         = 0.f;
    int              targetEntityId  = -1;
    uint32_t         targetEntityGen = 0;
};

struct CmdSetGameSpeed
{
    float timeScale = 1.0f;
};

struct CmdPlaceUnit
{
    int       playerId  = -1;
    UnitClass unitClass = UnitClass::Warrior;
    float     worldX    = 0.f;
    float     worldY    = 0.f;
};

using CommandData = std::
    variant<std::monostate, CmdMovePlayer, CmdPlaceWall, CmdMoveWall, CmdPlayerAttack, CmdPlayerSkill, CmdCombatMode, CmdSelectUnits, CmdOrderUnits, CmdSetGameSpeed, CmdPlaceUnit>;

class Command
{
    CommandType m_type{CommandType::NoOp};
    CommandData m_data;
    uint64_t    m_tick = 0;

public:
    Command() = default;
    Command(CommandType type, CommandData data, uint64_t tick = 0) : m_type(type), m_data(std::move(data)), m_tick(tick)
    {
    }

    [[nodiscard]] CommandType type() const
    {
        return m_type;
    }
    [[nodiscard]] const CommandData& data() const
    {
        return m_data;
    }
    [[nodiscard]] uint64_t tick() const
    {
        return m_tick;
    }
    void setTick(uint64_t tick)
    {
        m_tick = tick;
    }
};

std::string commandToString(const Command& command);

#endif
