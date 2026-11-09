#include "StateMachine.hpp"

const char* toStateLabel(UnitStateKind state)
{
    switch (state)
    {
        case UnitStateKind::Search:
            return "SEARCH";
        case UnitStateKind::Combat:
            return "COMBAT";
        case UnitStateKind::Retreat:
            return "RETREAT";
        case UnitStateKind::Explode:
            return "EXPLODE";
        default:
            return "NONE";
    }
}
