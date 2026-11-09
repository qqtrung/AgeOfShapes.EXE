#ifndef GAMESTATEDATA_HPP
#define GAMESTATEDATA_HPP

#include "ChunkGrid.hpp"
#include "Components.hpp"
#include "FloatFixed.hpp"
#include "Grid.hpp"
#include "VectorField.hpp"

#include <SFML/System/Vector2.hpp>

#include <array>
#include <memory>
#include <vector>

struct CampaignState
{
    int                           currentMap          = 1;
    int                           currentLevel        = 1;
    int                           maxLevel            = 3;
    int                           difficulty          = 1;
    int                           currentWave         = 0;
    int                           maxWaves            = 10;
    int                           waveTimerTicks      = 600;
    int                           waveTotalTimerTicks = 600;
    bool                          allWavesCompleted   = false;
    int                           collectedSupplies   = 0;
    int                           requiredSupplies    = 3;
    int                           totalSupplies       = 0;
    int                           team0UnitCount      = 4;
    int                           team1UnitCount      = 6;
    std::array<UnitTypeConfig, 6> unitConfigs{};
    int                           unitDeathsThisLevel = 0;
    int                           unitDeathLimit      = 4;

    [[nodiscard]] UnitTypeConfig&       getUnitConfig(UnitClass uc);
    [[nodiscard]] const UnitTypeConfig& getUnitConfig(UnitClass uc) const;
    int                                 pendingLevelReset    = 0;
    int                                 defensiveWallStock   = 4;
    int                                 electricWallStock    = 1;
    int                                 fireWallStock        = 1;
    int                                 gunWallStock         = 1;
    int                                 radarWallStock       = 1;
    int                                 designatedPointStock = 3;
    int                                 warriorUnitStock     = 0;
    int                                 archerUnitStock      = 0;
    int                                 tankUnitStock        = 0;
    int                                 gruntUnitStock       = 0;
    int                                 shooterUnitStock     = 0;
    int                                 berserkerUnitStock   = 0;

    struct TeamSupplyStock
    {
        int defensiveWallStock   = 4;
        int electricWallStock    = 1;
        int fireWallStock        = 1;
        int gunWallStock         = 1;
        int radarWallStock       = 1;
        int designatedPointStock = 3;
        int warriorUnitStock     = 0;
        int archerUnitStock      = 0;
        int tankUnitStock        = 0;
        int gruntUnitStock       = 0;
        int shooterUnitStock     = 0;
        int berserkerUnitStock   = 0;
    };
    std::array<TeamSupplyStock, 4> teamStocks{};

    bool victory  = true;
    bool gameOver = false;
};

struct NavData
{
    int spacing       = 16;
    int visionSpacing = 16;

    int updateIntervalTick       = 120;
    int ticksUntilUpdate         = 0;
    int visionUpdateIntervalTick = 120;
    int visionTicksUntilUpdate   = 0;
    int delayFrames              = 0;

    Grid<int> mGrid;

    bool                    visionDirty   = true;
    uint64_t                visionVersion = 0;
    Grid<char>              mVisionGrid;
    std::vector<Grid<char>> mTeamVisionGrid;

    [[nodiscard]] const Grid<char>& getVisionGrid(int teamId = 0) const;

    struct DebugRay
    {
        sf::Vector2f start;
        sf::Vector2f end;
        sf::Vector2f hitPos;
        bool         unblocked = true;
    };
    std::vector<DebugRay> mDebugRays;

    std::vector<std::unique_ptr<VectorField>> mTeamFlowFields;
    std::vector<std::unique_ptr<VectorField>> mTeamDesignatedFields;
    std::unique_ptr<VectorField>              mFieldDesignated;

    NavData();

    [[nodiscard]] VectorField&       getDesignatedField(int teamId = 0);
    [[nodiscard]] const VectorField& getDesignatedField(int teamId = 0) const;

    [[nodiscard]] VectorField&       getTeamField(int teamId = 0);
    [[nodiscard]] const VectorField& getTeamField(int teamId = 0) const;

    struct CachedWall
    {
        int       entityId = -1;
        f32::Rect rect;
        bool      isDesignated = false;
        int       teamId       = -1;
    };
    std::vector<CachedWall> mCachedWalls;

    void requestRecompute()
    {
        delayFrames            = 6;
        ticksUntilUpdate       = 0;
        visionTicksUntilUpdate = 0;
        visionDirty            = true;
    }

    struct LightSource
    {
        f32::Vec2 pos;
        f32::Fix  radius;
        int64_t   xScaled             = 0;
        int64_t   yScaled             = 0;
        int64_t   innerRadiusSqScaled = 0;
        int64_t   radiusSqScaled      = 0;
        int       ignoreWallId        = -1;
    };
    std::vector<LightSource> mScratchLights;

    struct WallBox
    {
        int64_t x0    = 0;
        int64_t y0    = 0;
        int64_t x1    = 0;
        int64_t y1    = 0;
        bool    valid = false;
    };
    std::vector<WallBox> mScratchWallBoxes;
    ChunkGrid            mLightChunkGrid;
    std::vector<int>     mScratchTargets;
    std::vector<int>     mScratchDpTargets;
};

struct CollisionData
{
    f32::Fix  spacing = 128;
    ChunkGrid map;

    struct CachedBody
    {
        int           entityId = -1;
        CPosition*    pos      = nullptr;
        CVelocity*    vel      = nullptr;
        CBoundCircle* bound    = nullptr;
    };
    std::vector<CachedBody> mCachedBodies;
};

#endif
