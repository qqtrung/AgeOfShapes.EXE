#include "Command.hpp"

#include <sstream>

std::string commandToString(const Command& command)
{
    std::ostringstream ss;
    switch (command.type())
    {
        case CommandType::NoOp:
        {
            ss << "NoOp";
            break;
        }
        case CommandType::MovePlayer:
        {
            const auto& d = std::get<CmdMovePlayer>(command.data());
            ss << "MovePlayer playerId=" << d.playerId << " dir=(" << d.dirX << "," << d.dirY << ")";
            break;
        }
        case CommandType::PlaceWall:
        {
            const auto& d = std::get<CmdPlaceWall>(command.data());
            ss << "PlaceWall playerId=" << d.playerId << " kind=" << static_cast<int>(d.kind) << " pos=(" << d.worldX
               << "," << d.worldY << ")";
            break;
        }
        case CommandType::MoveWall:
        {
            const auto& d = std::get<CmdMoveWall>(command.data());
            ss << "MoveWall playerId=" << d.playerId << " wallId=" << d.wallId << " wallGen=" << d.wallGen << " pos=("
               << d.worldX << "," << d.worldY << ")"
               << " releasing=" << d.releasing;
            break;
        }
        case CommandType::PlayerAttack:
        {
            const auto& d = std::get<CmdPlayerAttack>(command.data());
            ss << "PlayerAttack playerId=" << d.playerId;
            break;
        }
        case CommandType::PlayerSkill:
        {
            const auto& d = std::get<CmdPlayerSkill>(command.data());
            ss << "PlayerSkill playerId=" << d.playerId << " skillId=" << d.skillId;
            break;
        }
        case CommandType::CombatMode:
        {
            const auto& d = std::get<CmdCombatMode>(command.data());
            ss << "CombatMode playerId=" << d.playerId << " state=" << static_cast<int>(d.state);
            break;
        }
        case CommandType::SelectUnits:
        {
            const auto& d = std::get<CmdSelectUnits>(command.data());
            ss << "SelectUnits playerId=" << d.playerId << " count=" << d.unitIds.size() << " ids=[";
            for (size_t i = 0; i < d.unitIds.size(); ++i)
            {
                if (i > 0)
                    ss << ",";
                ss << d.unitIds[i];
            }
            ss << "]";
            break;
        }
        case CommandType::OrderUnits:
        {
            const auto& d = std::get<CmdOrderUnits>(command.data());
            ss << "OrderUnits playerId=" << d.playerId << " count=" << d.unitIds.size() << " target=(" << d.targetX
               << "," << d.targetY << ")"
               << " targetEntityId=" << d.targetEntityId << " targetGen=" << d.targetEntityGen;
            break;
        }
        case CommandType::SetGameSpeed:
        {
            const auto& d = std::get<CmdSetGameSpeed>(command.data());
            ss << "SetGameSpeed timeScale=" << d.timeScale;
            break;
        }
        case CommandType::PlaceUnit:
        {
            const auto& d = std::get<CmdPlaceUnit>(command.data());
            ss << "PlaceUnit playerId=" << d.playerId << " class=" << static_cast<int>(d.unitClass) << " pos=("
               << d.worldX << "," << d.worldY << ")";
            break;
        }
    }
    return ss.str();
}
