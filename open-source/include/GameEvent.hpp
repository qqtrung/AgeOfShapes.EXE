#ifndef GAMEEVENT_HPP
#define GAMEEVENT_HPP

#include "FloatFixed.hpp"

#include <optional>
#include <string>
#include <variant>

struct EventSound
{
    std::string              soundName;
    std::optional<f32::Vec2> worldPos;
    std::optional<f32::Fix>  customImportance;
};

using GameEvent = std::variant<EventSound>;

#endif
