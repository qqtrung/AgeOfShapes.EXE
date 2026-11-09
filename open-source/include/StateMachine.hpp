#ifndef STATEMACHINE_HPP
#define STATEMACHINE_HPP

enum class UnitStateKind
{
    Nothing,
    Search,
    Combat,
    Retreat,
    Explode,
};

enum class UnitEventKind
{
    OpponentSpotted,
    TargetLost,
    HealthLow,
    Safe,
};

const char* toStateLabel(UnitStateKind state);

#endif
