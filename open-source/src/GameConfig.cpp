#include "GameConfig.hpp"

#include <algorithm>

namespace GameConfig
{

const DifficultyConfig& getDifficultyConfig(int difficulty)
{
    int idx = std::clamp(difficulty, 0, kDifficultyCount - 1);
    return kDifficultyConfigs[idx];
}

const std::vector<WallLayout>& getLevelWalls(int level)
{
    int idx = level - 1;
    if (idx < 0 || idx >= kLevelCount)
    {
        return kLevelWalls[0];
    }
    return kLevelWalls[idx];
}

const std::vector<EnvironmentZoneLayout>& getLevelZones(int level)
{
    int idx = level - 1;
    if (idx < 0 || idx >= kLevelCount)
    {
        return kLevelZones[0];
    }
    return kLevelZones[idx];
}

const std::vector<RoadSegment>& getLevelRoads(int level)
{
    int idx = level - 1;
    if (idx < 0 || idx >= kLevelCount)
    {
        return kLevelRoads[0];
    }
    return kLevelRoads[idx];
}

const std::vector<WallLayout>& getMapWalls(int map)
{
    return getLevelWalls(map);
}

const std::vector<EnvironmentZoneLayout>& getMapZones(int map)
{
    return getLevelZones(map);
}

const std::vector<RoadSegment>& getMapRoads(int map)
{
    return getLevelRoads(map);
}

namespace Detail
{

MapLayoutConfig createPvP2PMapLayout(int mapIdx)
{
    MapLayoutConfig    cfg;
    constexpr f32::Fix mapW = 2992;

    auto addWall = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, WallKind kind, int teamId)
    { cfg.walls.push_back({.pos = {x, y}, .size = {w, h}, .kind = kind, .teamId = teamId}); };
    auto addSymWalls = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, WallKind kind)
    {
        addWall(x, y, w, h, kind, 0);
        addWall(mapW - x - w, y, w, h, kind, 1);
    };
    auto addTree = [&](f32::Fix x, f32::Fix y)
    { cfg.zones.push_back({.pos = {x, y}, .size = {75, 75}, .kind = ZoneKind::Tree}); };
    auto addSymTrees = [&](f32::Fix x, f32::Fix y)
    {
        addTree(x, y);
        addTree(mapW - x - f32::Fix(75), y);
    };
    auto addBush = [&](f32::Fix x, f32::Fix y)
    { cfg.zones.push_back({.pos = {x, y}, .size = {46, 40}, .kind = ZoneKind::Bush}); };
    auto addSymBushes = [&](f32::Fix x, f32::Fix y)
    {
        addBush(x, y);
        addBush(mapW - x - f32::Fix(46), y);
    };
    auto addSymZones = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, ZoneKind kind)
    {
        cfg.zones.push_back({.pos = {x, y}, .size = {w, h}, .kind = kind});
        cfg.zones.push_back({.pos = {mapW - x - w, y}, .size = {w, h}, .kind = kind});
    };

    cfg.bases.push_back(
        {.pos            = {220, 1080},
         .size           = {150, 150},
         .hp             = 450,
         .armor          = 10,
         .attackRange    = 120,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(1) / 2,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {500, 170},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {800, 700},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {800, 1230},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {500, 1760},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});

    cfg.bases.push_back(
        {.pos            = {mapW - 220 - 150, 1080},
         .size           = {150, 150},
         .hp             = 450,
         .armor          = 10,
         .attackRange    = 120,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(1) / 2,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {mapW - 500 - 100, 170},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {mapW - 800 - 100, 700},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {mapW - 800 - 100, 1230},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {mapW - 500 - 100, 1760},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});

    addSymWalls(220, 880, 100, 65, WallKind::Defensive);

    if (mapIdx == 1)
    {
        addSymWalls(1000, 650, 95, 60, WallKind::Defensive);
        addSymWalls(1250, 790, 80, 65, WallKind::Gun);
        addSymTrees(950, 800);
        addSymTrees(1250, 650);
        addSymBushes(1170, 760);

        addSymWalls(1000, 1265, 95, 60, WallKind::Defensive);
        addSymWalls(1250, 1130, 80, 65, WallKind::Fire);
        addSymTrees(950, 1130);
        addSymTrees(1250, 1265);
        addSymBushes(1110, 1205);

        addSymWalls(1100, 260, 90, 60, WallKind::Radar);
        addSymTrees(950, 250);
        addSymWalls(1100, 1650, 90, 60, WallKind::Electric);
        addSymTrees(950, 1650);

        addSymWalls(240, 240, 90, 60, WallKind::Defensive);
        addSymWalls(240, 1680, 90, 60, WallKind::Defensive);

        cfg.pickups = {{1110, 840}, {mapW - 1110, 840}, {1110, 1080}, {mapW - 1110, 1080}};
    }
    else if (mapIdx == 2)
    {
        addSymWalls(520, 680, 85, 85, WallKind::Rock);
        addSymWalls(520, 1260, 85, 85, WallKind::Rock);
        addSymTrees(260, 680);

        addSymWalls(1000, 650, 90, 85, WallKind::Rock);
        addSymWalls(1150, 790, 85, 65, WallKind::Radar);
        addSymTrees(1000, 790);

        addSymWalls(1000, 1250, 90, 85, WallKind::Rock);
        addSymWalls(1150, 1130, 85, 65, WallKind::Fire);
        addSymTrees(1000, 1130);

        addSymZones(1000, 250, 130, 110, ZoneKind::Quicksand);
        addSymZones(1000, 1650, 130, 110, ZoneKind::Quicksand);

        addSymWalls(240, 240, 85, 85, WallKind::Rock);
        addSymWalls(240, 1680, 85, 85, WallKind::Rock);

        cfg.pickups = {{1150, 880}, {mapW - 1150, 880}, {1150, 1220}, {mapW - 1150, 1220}};
    }
    else
    {
        addSymWalls(1000, 650, 95, 60, WallKind::Defensive);
        addSymWalls(1250, 650, 85, 65, WallKind::Electric);
        addSymWalls(1250, 790, 85, 70, WallKind::Gun);
        addSymTrees(950, 800);
        addSymBushes(1170, 760);

        addSymWalls(1000, 1265, 95, 60, WallKind::Defensive);
        addSymWalls(1250, 1265, 85, 70, WallKind::Fire);
        addSymWalls(1250, 1130, 85, 65, WallKind::Radar);
        addSymTrees(950, 1130);
        addSymBushes(1110, 1205);

        addSymWalls(520, 600, 90, 60, WallKind::Defensive);
        addSymTrees(520, 820);
        addSymTrees(250, 620);
        addSymBushes(380, 640);

        addSymWalls(520, 1120, 90, 60, WallKind::Defensive);
        addSymTrees(520, 1340);
        addSymTrees(250, 1340);
        addSymBushes(380, 1350);

        addSymWalls(230, 180, 95, 60, WallKind::Defensive);
        addSymTrees(230, 290);
        addSymBushes(360, 310);
        addSymWalls(850, 180, 95, 60, WallKind::Defensive);
        addSymWalls(1080, 180, 85, 65, WallKind::Radar);
        addSymWalls(1260, 180, 95, 60, WallKind::Defensive);
        addSymTrees(860, 290);
        addSymTrees(1260, 290);
        addSymBushes(1090, 310);

        addSymWalls(230, 1640, 95, 60, WallKind::Defensive);
        addSymTrees(230, 1760);
        addSymBushes(360, 1780);
        addSymWalls(850, 1640, 95, 60, WallKind::Defensive);
        addSymWalls(1080, 1640, 85, 65, WallKind::Electric);
        addSymWalls(1260, 1640, 95, 60, WallKind::Defensive);
        addSymTrees(860, 1760);
        addSymTrees(1260, 1760);
        addSymBushes(1090, 1780);

        cfg.pickups = {{1110, 840}, {mapW - 1110, 840}, {1110, 1080}, {mapW - 1110, 1080}};
    }

    return cfg;
}

MapLayoutConfig createPvP3PMapLayout(int mapIdx)
{
    MapLayoutConfig    cfg;
    constexpr f32::Fix mapW = 2992;

    auto addWall = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, WallKind kind, int teamId)
    { cfg.walls.push_back({.pos = {x, y}, .size = {w, h}, .kind = kind, .teamId = teamId}); };
    auto addSymNorthWalls = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, WallKind kind)
    {
        addWall(x, y, w, h, kind, 0);
        addWall(mapW - x - w, y, w, h, kind, 0);
    };
    auto addSymSouthWalls = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, WallKind kind)
    {
        addWall(x, y, w, h, kind, 1);
        addWall(mapW - x - w, y, w, h, kind, 2);
    };
    auto addTree = [&](f32::Fix x, f32::Fix y)
    { cfg.zones.push_back({.pos = {x, y}, .size = {75, 75}, .kind = ZoneKind::Tree}); };
    auto addSymTrees = [&](f32::Fix x, f32::Fix y)
    {
        addTree(x, y);
        addTree(mapW - x - f32::Fix(75), y);
    };
    auto addBush = [&](f32::Fix x, f32::Fix y)
    { cfg.zones.push_back({.pos = {x, y}, .size = {46, 40}, .kind = ZoneKind::Bush}); };
    auto addSymBushes = [&](f32::Fix x, f32::Fix y)
    {
        addBush(x, y);
        addBush(mapW - x - f32::Fix(46), y);
    };
    auto addSymZones = [&](f32::Fix x, f32::Fix y, f32::Fix w, f32::Fix h, ZoneKind kind)
    {
        cfg.zones.push_back({.pos = {x, y}, .size = {w, h}, .kind = kind});
        cfg.zones.push_back({.pos = {mapW - x - w, y}, .size = {w, h}, .kind = kind});
    };

    const WallKind primaryWall = (mapIdx == 2) ? WallKind::Rock : WallKind::Defensive;

    cfg.bases.push_back(
        {.pos            = {1250, 65},
         .size           = {150, 150},
         .hp             = 450,
         .armor          = 10,
         .attackRange    = 120,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(1) / 2,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {1275, 310},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {920, 260},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});
    cfg.bases.push_back(
        {.pos            = {mapW - 920 - 100, 260},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 0});

    addWall(mapW - 1250 - 110, 85, 110, 75, primaryWall, 0);
    addWall(mapW - 1275 - 95, 320, 95, 70, (mapIdx == 3) ? WallKind::Electric : WallKind::Gun, 0);

    cfg.bases.push_back(
        {.pos            = {220, 1700},
         .size           = {150, 150},
         .hp             = 450,
         .armor          = 10,
         .attackRange    = 120,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(1) / 2,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {420, 1580},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {520, 1340},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});
    cfg.bases.push_back(
        {.pos            = {860, 1150},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 1});

    cfg.bases.push_back(
        {.pos            = {mapW - 220 - 150, 1700},
         .size           = {150, 150},
         .hp             = 450,
         .armor          = 10,
         .attackRange    = 120,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(1) / 2,
         .teamId         = 2});
    cfg.bases.push_back(
        {.pos            = {mapW - 420 - 100, 1580},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 2});
    cfg.bases.push_back(
        {.pos            = {mapW - 520 - 100, 1340},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 2});
    cfg.bases.push_back(
        {.pos            = {mapW - 860 - 100, 1150},
         .size           = {100, 100},
         .hp             = 250,
         .armor          = 5,
         .attackRange    = 100,
         .defenseRange   = 200,
         .attackCooldown = 20,
         .damage         = f32::Fix(3) / 10,
         .teamId         = 2});

    addSymSouthWalls(240, 180, 95, 60, primaryWall);
    addSymSouthWalls(460, 180, 85, 65, (mapIdx == 3) ? WallKind::Electric : WallKind::Gun);
    addSymTrees(240, 290);
    addSymTrees(560, 290);
    addSymBushes(380, 300);

    addSymNorthWalls(820, 130, 90, 60, primaryWall);
    addSymTrees(1080, 150);
    addSymBushes(1080, 280);

    addSymSouthWalls(240, 600, 95, 60, primaryWall);
    addSymSouthWalls(480, 600, 85, 65, WallKind::Radar);
    addSymSouthWalls(240, 780, 95, 60, primaryWall);
    addSymTrees(480, 760);
    addSymTrees(360, 680);
    addSymBushes(600, 700);

    addSymSouthWalls(230, 1120, 95, 60, primaryWall);
    addSymTrees(230, 1240);
    addSymBushes(360, 1200);

    addSymNorthWalls(850, 600, 95, 60, primaryWall);
    addSymNorthWalls(1100, 600, 85, 65, (mapIdx == 3) ? WallKind::Electric : WallKind::Radar);
    addSymNorthWalls(850, 780, 95, 60, primaryWall);
    addSymNorthWalls(1100, 780, 85, 65, WallKind::Gun);
    addSymTrees(980, 680);
    addSymBushes(980, 800);

    addSymSouthWalls(1020, 1280, 95, 60, primaryWall);
    addSymSouthWalls(1150, 1140, 85, 65, WallKind::Fire);
    addSymTrees(1020, 1140);
    addSymBushes(1160, 1290);

    addSymSouthWalls(850, 1640, 95, 60, primaryWall);
    addSymSouthWalls(1100, 1640, 85, 65, (mapIdx == 3) ? WallKind::Radar : WallKind::Electric);
    addSymSouthWalls(1260, 1760, 95, 60, primaryWall);
    addSymTrees(850, 1770);
    addSymTrees(1100, 1770);
    addSymBushes(980, 1700);

    addSymTrees(200, 1880);
    addSymBushes(380, 1880);

    if (mapIdx == 2)
    {
        addSymZones(600, 840, 115, 90, ZoneKind::Quicksand);
        addSymZones(640, 1680, 120, 95, ZoneKind::Quicksand);
    }

    cfg.pickups = {{1496, 620}, {1180, 980}, {mapW - 1180, 980}, {1496, 1320}};
    return cfg;
}

} // namespace Detail

const MapLayoutConfig& getPvP2PMapLayout(int mapIdx)
{
    static const MapLayoutConfig m1 = Detail::createPvP2PMapLayout(1);
    static const MapLayoutConfig m2 = Detail::createPvP2PMapLayout(2);
    static const MapLayoutConfig m3 = Detail::createPvP2PMapLayout(3);
    if (mapIdx == 2)
        return m2;
    if (mapIdx == 3)
        return m3;
    return m1;
}

const MapLayoutConfig& getPvP3PMapLayout(int mapIdx)
{
    static const MapLayoutConfig m1 = Detail::createPvP3PMapLayout(1);
    static const MapLayoutConfig m2 = Detail::createPvP3PMapLayout(2);
    static const MapLayoutConfig m3 = Detail::createPvP3PMapLayout(3);
    if (mapIdx == 2)
        return m2;
    if (mapIdx == 3)
        return m3;
    return m1;
}

int getCoopMaxWaves(int difficulty)
{
    if (difficulty <= 0)
        return 10;
    if (difficulty == 1)
        return 15;
    return 20;
}

int getPvPBaseUnitCount(int difficulty)
{
    if (difficulty <= 0)
        return 3;
    if (difficulty == 1)
        return 4;
    return 5;
}

static const std::array<LevelWaveConfig, kLevelCount> kLevelWaveConfigs = {{
    {
        .waves                = {{
            {.delaySeconds = 12, .gruntCount = 2, .shooterCount = 0, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 2, .shooterCount = 1, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 3, .shooterCount = 1, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 15, .gruntCount = 3, .shooterCount = 2, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 16, .gruntCount = 4, .shooterCount = 2, .tankCount = 1, .berserkerCount = 0},
            {.delaySeconds = 16, .gruntCount = 4, .shooterCount = 3, .tankCount = 1, .berserkerCount = 1},
            {.delaySeconds = 18, .gruntCount = 5, .shooterCount = 3, .tankCount = 1, .berserkerCount = 2},
            {.delaySeconds = 18, .gruntCount = 6, .shooterCount = 4, .tankCount = 2, .berserkerCount = 2},
            {.delaySeconds = 20, .gruntCount = 8, .shooterCount = 5, .tankCount = 2, .berserkerCount = 3},
            {.delaySeconds = 20, .gruntCount = 10, .shooterCount = 6, .tankCount = 3, .berserkerCount = 3},
            {.delaySeconds = 20, .gruntCount = 10, .shooterCount = 6, .tankCount = 3, .berserkerCount = 4},
            {.delaySeconds = 20, .gruntCount = 11, .shooterCount = 7, .tankCount = 3, .berserkerCount = 4},
            {.delaySeconds = 20, .gruntCount = 11, .shooterCount = 7, .tankCount = 4, .berserkerCount = 4},
            {.delaySeconds = 22, .gruntCount = 12, .shooterCount = 8, .tankCount = 4, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 13, .shooterCount = 8, .tankCount = 5, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 13, .shooterCount = 9, .tankCount = 5, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 14, .shooterCount = 9, .tankCount = 5, .berserkerCount = 6},
            {.delaySeconds = 24, .gruntCount = 15, .shooterCount = 10, .tankCount = 6, .berserkerCount = 6},
            {.delaySeconds = 24, .gruntCount = 16, .shooterCount = 10, .tankCount = 6, .berserkerCount = 7},
            {.delaySeconds = 25, .gruntCount = 18, .shooterCount = 11, .tankCount = 7, .berserkerCount = 7},
        }},
        .finalSurvivalSeconds = 30,
    },
    {
        .waves                = {{
            {.delaySeconds = 12, .gruntCount = 2, .shooterCount = 1, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 3, .shooterCount = 1, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 3, .shooterCount = 2, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 15, .gruntCount = 4, .shooterCount = 2, .tankCount = 1, .berserkerCount = 0},
            {.delaySeconds = 16, .gruntCount = 4, .shooterCount = 3, .tankCount = 1, .berserkerCount = 1},
            {.delaySeconds = 16, .gruntCount = 5, .shooterCount = 3, .tankCount = 2, .berserkerCount = 1},
            {.delaySeconds = 18, .gruntCount = 6, .shooterCount = 4, .tankCount = 2, .berserkerCount = 2},
            {.delaySeconds = 18, .gruntCount = 7, .shooterCount = 5, .tankCount = 2, .berserkerCount = 3},
            {.delaySeconds = 20, .gruntCount = 9, .shooterCount = 6, .tankCount = 3, .berserkerCount = 3},
            {.delaySeconds = 20, .gruntCount = 11, .shooterCount = 7, .tankCount = 4, .berserkerCount = 4},
            {.delaySeconds = 20, .gruntCount = 11, .shooterCount = 8, .tankCount = 4, .berserkerCount = 4},
            {.delaySeconds = 20, .gruntCount = 12, .shooterCount = 8, .tankCount = 4, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 13, .shooterCount = 9, .tankCount = 5, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 14, .shooterCount = 9, .tankCount = 5, .berserkerCount = 6},
            {.delaySeconds = 22, .gruntCount = 15, .shooterCount = 10, .tankCount = 6, .berserkerCount = 6},
            {.delaySeconds = 22, .gruntCount = 16, .shooterCount = 10, .tankCount = 6, .berserkerCount = 6},
            {.delaySeconds = 24, .gruntCount = 17, .shooterCount = 11, .tankCount = 6, .berserkerCount = 7},
            {.delaySeconds = 24, .gruntCount = 18, .shooterCount = 11, .tankCount = 7, .berserkerCount = 7},
            {.delaySeconds = 25, .gruntCount = 19, .shooterCount = 12, .tankCount = 7, .berserkerCount = 8},
            {.delaySeconds = 25, .gruntCount = 21, .shooterCount = 13, .tankCount = 8, .berserkerCount = 8},
        }},
        .finalSurvivalSeconds = 35,
    },
    {
        .waves                = {{
            {.delaySeconds = 12, .gruntCount = 3, .shooterCount = 1, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 3, .shooterCount = 2, .tankCount = 0, .berserkerCount = 0},
            {.delaySeconds = 14, .gruntCount = 4, .shooterCount = 2, .tankCount = 0, .berserkerCount = 1},
            {.delaySeconds = 15, .gruntCount = 5, .shooterCount = 2, .tankCount = 1, .berserkerCount = 1},
            {.delaySeconds = 16, .gruntCount = 6, .shooterCount = 3, .tankCount = 1, .berserkerCount = 1},
            {.delaySeconds = 16, .gruntCount = 6, .shooterCount = 4, .tankCount = 2, .berserkerCount = 2},
            {.delaySeconds = 18, .gruntCount = 7, .shooterCount = 5, .tankCount = 3, .berserkerCount = 2},
            {.delaySeconds = 18, .gruntCount = 9, .shooterCount = 6, .tankCount = 3, .berserkerCount = 3},
            {.delaySeconds = 20, .gruntCount = 10, .shooterCount = 7, .tankCount = 4, .berserkerCount = 4},
            {.delaySeconds = 20, .gruntCount = 12, .shooterCount = 8, .tankCount = 5, .berserkerCount = 5},
            {.delaySeconds = 20, .gruntCount = 13, .shooterCount = 8, .tankCount = 5, .berserkerCount = 5},
            {.delaySeconds = 22, .gruntCount = 14, .shooterCount = 9, .tankCount = 5, .berserkerCount = 6},
            {.delaySeconds = 22, .gruntCount = 15, .shooterCount = 10, .tankCount = 6, .berserkerCount = 6},
            {.delaySeconds = 22, .gruntCount = 16, .shooterCount = 10, .tankCount = 6, .berserkerCount = 7},
            {.delaySeconds = 24, .gruntCount = 18, .shooterCount = 11, .tankCount = 7, .berserkerCount = 7},
            {.delaySeconds = 24, .gruntCount = 19, .shooterCount = 12, .tankCount = 7, .berserkerCount = 7},
            {.delaySeconds = 24, .gruntCount = 20, .shooterCount = 12, .tankCount = 8, .berserkerCount = 8},
            {.delaySeconds = 25, .gruntCount = 21, .shooterCount = 13, .tankCount = 8, .berserkerCount = 8},
            {.delaySeconds = 25, .gruntCount = 22, .shooterCount = 14, .tankCount = 9, .berserkerCount = 9},
            {.delaySeconds = 26, .gruntCount = 24, .shooterCount = 15, .tankCount = 10, .berserkerCount = 10},
        }},
        .finalSurvivalSeconds = 40,
    },
}};

const LevelWaveConfig& getLevelWaveConfig(int level)
{
    const int index = std::clamp(level - 1, 0, static_cast<int>(kLevelWaveConfigs.size()) - 1);
    return kLevelWaveConfigs[index];
}

} // namespace GameConfig
