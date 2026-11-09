#ifndef SYSTEMVIEWSTATE_HPP
#define SYSTEMVIEWSTATE_HPP

#include "Components.hpp"
#include "ISystem.hpp"

#include <SFML/Graphics/View.hpp>

#include <SFML/System/Vector2.hpp>

class ViewState;

namespace SystemViewState
{
struct SystemCamera : ISystem<ViewState>
{
    sf::Vector2f mVelocity          = {0.f, 0.f};
    sf::Vector2f mAcceleration      = {0.f, 0.f};
    sf::Vector2f mTargetPosition    = {0.f, 0.f};
    bool         mIsMoving          = false;
    float        mVelocityThreshold = 0.5f;

    sf::View mWorldView;
    float    mZoomLevel = 0.6f;

    void update() override;
    void setTargetPosition(sf::Vector2f target);
    void calculateAcceleration(sf::Vector2f currentPos, sf::Vector2f targetPos);
};

struct SystemDragDrop : ISystem<ViewState>
{
    void update() override;
};

struct SystemSelection : ISystem<ViewState>
{
    CInput mInput;

    struct DebugOptions
    {
        bool showVelocity    = false;
        bool showVisionGrid  = true;
        bool showVisionRays  = false;
        bool showCombatLinks = false;
        bool showPanel       = false;
        bool showLabels      = false;
        bool showRanges      = false;
        bool showTeam0Field  = false;
        bool showTeam1Field  = false;
        bool showPartition   = false;
    } mDebug;

    void update() override;
};

struct SystemAnimation : ISystem<ViewState>
{
    void update() override;
};

} // namespace SystemViewState

#endif
