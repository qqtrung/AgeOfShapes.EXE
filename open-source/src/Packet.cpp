#include "Packet.hpp"

#include <sstream>

std::string commandOnTickToString(const CommandOnTick& cot)
{
    std::ostringstream ss;
    ss << "Tick " << cot.tick << ": ";
    if (cot.commands.empty())
    {
        ss << "None";
    }
    else
    {
        for (size_t i = 0; i < cot.commands.size(); ++i)
        {
            if (i > 0)
                ss << " | ";
            ss << commandToString(cot.commands[i]);
        }
    }
    return ss.str();
}
