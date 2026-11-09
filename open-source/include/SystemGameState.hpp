#ifndef SYSTEMGAMESTATE_HPP
#define SYSTEMGAMESTATE_HPP

#include "FloatFixed.hpp"
#include "GameStateData.hpp"
#include "ISystem.hpp"

#include <map>
#include <set>
#include <vector>

class GameState;

namespace SystemGameState
{
struct SystemInit : ISystem<GameState>
{
    using CampaignState = ::CampaignState;

    void init() override;
};

struct SystemMatchEnd : ISystem<GameState>
{
    void exit() override;
    void update() override;
};

struct SystemNavigationField : ISystem<GameState>
{
    using DebugRay   = NavData::DebugRay;
    using CachedWall = NavData::CachedWall;

    void init() override;
    void update() override;
    void computeGrid();
    void updateVisionGrid();
    void ensureVisionGridAllocated(int maxTeamId);
    void updateVisionGridForTeam(int teamId, Grid<char>& targetGrid);
    void updateDesignatedFields();
    void updateTeamFields();
    void updateTeamField(int teamId);

    [[nodiscard]] const Grid<char>&  getVisionGrid(int teamId = 0) const;
    [[nodiscard]] VectorField&       getDesignatedField(int teamId = 0);
    [[nodiscard]] const VectorField& getDesignatedField(int teamId = 0) const;
    [[nodiscard]] VectorField&       getTeamField(int teamId = 0);
    [[nodiscard]] const VectorField& getTeamField(int teamId = 0) const;
    [[nodiscard]] bool hasLineOfSight(f32::Vec2 start, f32::Vec2 end, int ignoreWallId = -1, bool allowEndInsideWall = false) const;
    [[nodiscard]] std::set<int> getObstacleInSight(f32::Vec2 start, f32::Vec2 end) const;
    void                        requestRecompute();
};

struct SystemCollision : ISystem<GameState>
{
    void init() override;
    void update() override;
};

struct SystemMovement : ISystem<GameState>
{
    void update() override;
};

struct SystemUnit : ISystem<GameState>
{
    void        update() override;
    static void handleUnitTransition(GameState* context, int entityId, UnitEventKind event, int targetId = -1);
};

struct SystemPlayer : ISystem<GameState>
{
    void update() override;
};

struct SystemWall : ISystem<GameState>
{
    void update() override;
};

struct SystemBase : ISystem<GameState>
{
    void update() override;
};

struct SystemProjectile : ISystem<GameState>
{
    void update() override;
};

struct SystemPickup : ISystem<GameState>
{
    void update() override;
};

struct SystemVisualEffect : ISystem<GameState>
{
    void update() override;
};

} // namespace SystemGameState

#endif
