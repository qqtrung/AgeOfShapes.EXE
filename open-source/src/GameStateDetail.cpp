#include "GameStateDetail.hpp"

#include "ComponentBundle.hpp"
#include "Components.hpp"
#include "FloatFixed.hpp"
#include "GameConfig.hpp"
#include "GameState.hpp"
#include "SystemGameState.hpp"

#include <algorithm>

namespace GameStateDetail
{
f32::Vec2 getBuildWallSize(WallKind kind)
{
    return GameConfig::kWallBuildData[static_cast<int>(kind)].size;
}

CWall createBuildWallData(WallKind kind)
{
    CWall wall{};
    wall.kind         = kind;
    wall.playerPlaced = true;

    const auto& wallData = GameConfig::kWallData[static_cast<int>(kind)];
    wall.effectRange     = wallData.effectRange;
    wall.cooldownTick    = wallData.cooldownTick;
    wall.cooldownTimer   = wallData.cooldownTick;
    wall.damage          = wallData.damage;

    return wall;
}

const char* getWallKindLabel(WallKind kind)
{
    switch (kind)
    {
        case WallKind::Defensive:
            return "Defensive";
        case WallKind::Electric:
            return "Electric";
        case WallKind::Fire:
            return "Fire";
        case WallKind::Gun:
            return "Gun";
        case WallKind::Radar:
            return "Radar";
        case WallKind::DesignatedPoint:
            return "Designated";
        case WallKind::Rock:
            return "Rock";
        default:
            return "Wall";
    }
}

const int* getWallStock(const GameState* gameState, WallKind kind, int playerId)
{
    const int teamId = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;
    if (teamId > 0 && teamId < 4)
    {
        const auto& ts = gameState->campaign().teamStocks[teamId];
        switch (kind)
        {
            case WallKind::Defensive:
                return &ts.defensiveWallStock;
            case WallKind::Electric:
                return &ts.electricWallStock;
            case WallKind::Fire:
                return &ts.fireWallStock;
            case WallKind::Gun:
                return &ts.gunWallStock;
            case WallKind::Radar:
                return &ts.radarWallStock;
            case WallKind::DesignatedPoint:
                return &ts.designatedPointStock;
            case WallKind::Rock:
                return nullptr;
            default:
                return nullptr;
        }
    }

    switch (kind)
    {
        case WallKind::Defensive:
            return &gameState->campaign().defensiveWallStock;
        case WallKind::Electric:
            return &gameState->campaign().electricWallStock;
        case WallKind::Fire:
            return &gameState->campaign().fireWallStock;
        case WallKind::Gun:
            return &gameState->campaign().gunWallStock;
        case WallKind::Radar:
            return &gameState->campaign().radarWallStock;
        case WallKind::DesignatedPoint:
            return &gameState->campaign().designatedPointStock;
        case WallKind::Rock:
            return nullptr;
        default:
            return nullptr;
    }
}

void consumeWallStock(GameState* gameState, WallKind kind, int playerId)
{
    const int teamId = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;
    if (teamId > 0 && teamId < 4)
    {
        auto& ts = gameState->campaign().teamStocks[teamId];
        switch (kind)
        {
            case WallKind::Defensive:
                ts.defensiveWallStock--;
                break;
            case WallKind::Electric:
                ts.electricWallStock--;
                break;
            case WallKind::Fire:
                ts.fireWallStock--;
                break;
            case WallKind::Gun:
                ts.gunWallStock--;
                break;
            case WallKind::Radar:
                ts.radarWallStock--;
                break;
            case WallKind::DesignatedPoint:
                ts.designatedPointStock--;
                break;
            default:
                break;
        }
        return;
    }

    switch (kind)
    {
        case WallKind::Defensive:
            gameState->campaign().defensiveWallStock--;
            break;
        case WallKind::Electric:
            gameState->campaign().electricWallStock--;
            break;
        case WallKind::Fire:
            gameState->campaign().fireWallStock--;
            break;
        case WallKind::Gun:
            gameState->campaign().gunWallStock--;
            break;
        case WallKind::Radar:
            gameState->campaign().radarWallStock--;
            break;
        case WallKind::DesignatedPoint:
            gameState->campaign().designatedPointStock--;
            break;
        default:
            break;
    }
}

const int* getUnitStock(const GameState* gameState, UnitClass unitClass, int playerId)
{
    const int teamId = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;
    if (teamId > 0 && teamId < 4)
    {
        const auto& ts = gameState->campaign().teamStocks[teamId];
        switch (unitClass)
        {
            case UnitClass::Warrior:
                return &ts.warriorUnitStock;
            case UnitClass::Archer:
                return &ts.archerUnitStock;
            case UnitClass::Tank:
                return &ts.tankUnitStock;
            case UnitClass::Grunt:
                return &ts.gruntUnitStock;
            case UnitClass::Shooter:
                return &ts.shooterUnitStock;
            case UnitClass::Berserker:
                return &ts.berserkerUnitStock;
        }
        return nullptr;
    }

    switch (unitClass)
    {
        case UnitClass::Warrior:
            return &gameState->campaign().warriorUnitStock;
        case UnitClass::Archer:
            return &gameState->campaign().archerUnitStock;
        case UnitClass::Tank:
            return &gameState->campaign().tankUnitStock;
        case UnitClass::Grunt:
            return &gameState->campaign().gruntUnitStock;
        case UnitClass::Shooter:
            return &gameState->campaign().shooterUnitStock;
        case UnitClass::Berserker:
            return &gameState->campaign().berserkerUnitStock;
    }
    return nullptr;
}

void consumeUnitStock(GameState* gameState, UnitClass unitClass, int playerId)
{
    const int teamId = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;
    if (teamId > 0 && teamId < 4)
    {
        auto& ts = gameState->campaign().teamStocks[teamId];
        switch (unitClass)
        {
            case UnitClass::Warrior:
                ts.warriorUnitStock--;
                break;
            case UnitClass::Archer:
                ts.archerUnitStock--;
                break;
            case UnitClass::Tank:
                ts.tankUnitStock--;
                break;
            case UnitClass::Grunt:
                ts.gruntUnitStock--;
                break;
            case UnitClass::Shooter:
                ts.shooterUnitStock--;
                break;
            case UnitClass::Berserker:
                ts.berserkerUnitStock--;
                break;
        }
        return;
    }

    switch (unitClass)
    {
        case UnitClass::Warrior:
            gameState->campaign().warriorUnitStock--;
            break;
        case UnitClass::Archer:
            gameState->campaign().archerUnitStock--;
            break;
        case UnitClass::Tank:
            gameState->campaign().tankUnitStock--;
            break;
        case UnitClass::Grunt:
            gameState->campaign().gruntUnitStock--;
            break;
        case UnitClass::Shooter:
            gameState->campaign().shooterUnitStock--;
            break;
        case UnitClass::Berserker:
            gameState->campaign().berserkerUnitStock--;
            break;
    }
}

f32::Fix getUnitRadius(UnitClass unitClass)
{
    switch (unitClass)
    {
        case UnitClass::Warrior:
            return 11;
        case UnitClass::Archer:
            return 9;
        case UnitClass::Tank:
            return 15;
        case UnitClass::Grunt:
            return 11;
        case UnitClass::Shooter:
            return 9;
        case UnitClass::Berserker:
            return 12;
    }
    return 11;
}

bool canPlaceUnit(GameState* gameState, f32::Vec2 center, UnitClass unitClass)
{
    const f32::Fix radius = getUnitRadius(unitClass);
    if (center.x - radius < f32::Fix(0) || center.y - radius < f32::Fix(0) ||
        center.x + radius > gameState->mapWidth() || center.y + radius > gameState->mapHeight())
    {
        return false;
    }

    return std::ranges::none_of(gameState->mAdmin.mPool.viewComponent<CRectDragDrop>(),
                                [&](const auto& wallRect)
                                { return isPointInsideRect(center, f32::Rect(wallRect.pos, wallRect.size)); });
}

void placePlayerUnit(GameState* gameState, f32::Vec2 center, UnitClass unitClass, int playerId)
{
    const int* stock = getUnitStock(gameState, unitClass, playerId);
    if (!stock || *stock <= 0 || !canPlaceUnit(gameState, center, unitClass))
    {
        return;
    }

    int teamId = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;

    GameStateDetail::spawnClickIndicator(gameState, center, 4, 20, teamId);
    gameState->playSound("SndBuild", center, f32::Fix(85) / 100);

    spawnAgent(gameState, center, unitClass, false, teamId);
    consumeUnitStock(gameState, unitClass, playerId);
}

bool canPlaceWall(GameState* gameState, const f32::Rect& candidateRect)
{
    if (candidateRect.pos.x < f32::Fix(0) || candidateRect.pos.y < f32::Fix(0) ||
        candidateRect.pos.x + candidateRect.size.x > gameState->mapWidth() ||
        candidateRect.pos.y + candidateRect.size.y > gameState->mapHeight())
    {
        return false;
    }

    return std::ranges::all_of(gameState->mAdmin.mPool.viewComponent<CRectDragDrop>(),
                               [&](const auto& wallRect)
                               { return !candidateRect.intersects(f32::Rect(wallRect.pos, wallRect.size)); });
}

void placePlayerWall(GameState* gameState, f32::Vec2 center, WallKind kind, int playerId)
{
    const int* stock = getWallStock(gameState, kind, playerId);
    if (!stock || *stock <= 0)
    {
        return;
    }

    auto*      wallRect = gameState->alloc<CRectDragDrop>();
    auto*      wallData = gameState->alloc<CWall>(createBuildWallData(kind));
    const auto size     = getBuildWallSize(kind);

    wallRect->size = size;
    wallRect->pos  = center - size * f32::Fix(1) / 2;

    if (!canPlaceWall(gameState, f32::Rect(wallRect->pos, wallRect->size)))
    {
        wallRect->~CRectDragDrop();
        wallData->~CWall();
        return;
    }

    int   teamId       = (playerId >= 0) ? getEntityTeam(gameState->mAdmin.mPool, playerId) : 0;
    auto* teamComp     = gameState->alloc<CTeam>();
    teamComp->teamId   = teamId;
    teamComp->playerId = playerId;

    if (kind == WallKind::DesignatedPoint)
    {
        auto* pos    = gameState->alloc<CPosition>();
        pos->pos     = center;
        pos->posInit = center;
        pos->prevPos = center;
        gameState->mAdmin.add(gameState->createBundle(wallRect, wallData, teamComp, pos));
    }
    else
    {
        gameState->mAdmin.add(gameState->createBundle(wallRect, wallData, teamComp));
    }

    consumeWallStock(gameState, kind, playerId);

    gameState->playSound("SndBuild", center, f32::Fix(85) / 100);

    gameState->nav().requestRecompute();
}

bool isAlive(const GameState* gameState, int entityId)
{
    return entityId >= 0 && gameState->mAdmin.mPool.has(entityId) && !gameState->mAdmin.isPendingRemove(entityId);
}

bool isPointInsideRect(const f32::Vec2& point, const f32::Rect& rect)
{
    return rect.contains(point);
}

bool segmentsIntersect(const f32::Vec2& a, const f32::Vec2& b, const f32::Vec2& c, const f32::Vec2& d)
{
    const auto cross     = [](const f32::Vec2& u, const f32::Vec2& v) { return u.x * v.y - u.y * v.x; };
    const auto direction = [&](const f32::Vec2& p, const f32::Vec2& q, const f32::Vec2& r)
    { return cross(q - p, r - p); };
    const auto onSegment = [](const f32::Vec2& p, const f32::Vec2& q, const f32::Vec2& r)
    {
        return std::min(p.x, r.x) <= q.x && q.x <= std::max(p.x, r.x) && std::min(p.y, r.y) <= q.y &&
               q.y <= std::max(p.y, r.y);
    };

    const f32::Fix d1 = direction(a, b, c);
    const f32::Fix d2 = direction(a, b, d);
    const f32::Fix d3 = direction(c, d, a);
    const f32::Fix d4 = direction(c, d, b);

    const f32::Fix zero = f32::Fix(0);
    if (((d1 > zero && d2 < zero) || (d1 < zero && d2 > zero)) && ((d3 > zero && d4 < zero) || (d3 < zero && d4 > zero)))
    {
        return true;
    }

    if (d1 == zero && onSegment(a, c, b))
    {
        return true;
    }
    if (d2 == zero && onSegment(a, d, b))
    {
        return true;
    }
    if (d3 == zero && onSegment(c, a, d))
    {
        return true;
    }
    if (d4 == zero && onSegment(c, b, d))
    {
        return true;
    }

    return false;
}

bool lineIntersectsRect(const f32::Vec2& start, const f32::Vec2& end, const f32::Rect& rect)
{
    if (isPointInsideRect(start, rect) || isPointInsideRect(end, rect))
    {
        return true;
    }

    const f32::Vec2 topLeft     = rect.pos;
    const f32::Vec2 topRight    = {rect.pos.x + rect.size.x, rect.pos.y};
    const f32::Vec2 bottomLeft  = {rect.pos.x, rect.pos.y + rect.size.y};
    const f32::Vec2 bottomRight = {rect.pos.x + rect.size.x, rect.pos.y + rect.size.y};

    return segmentsIntersect(start, end, topLeft, topRight) || segmentsIntersect(start, end, topRight, bottomRight) ||
           segmentsIntersect(start, end, bottomRight, bottomLeft) || segmentsIntersect(start, end, bottomLeft, topLeft);
}

bool hasLineOfSight(const GameState* gameState, const f32::Vec2& start, const f32::Vec2& end)
{
    const auto& systemField = gameState->mSystems.get<SystemGameState::SystemNavigationField>();
    return systemField.hasLineOfSight(start, end);
}


using namespace GameConfig::AI;

using GameConfig::getDifficultyConfig;
using GameConfig::kTeam0SpawnPositions;
using GameConfig::kTeam1SpawnPositions;

using GameConfig::AI::kPickupValue;
using GameConfig::AI::kProjectileSpeed;
using GameConfig::AI::kPushDecay;
using GameConfig::AI::kRetreatDurationTick;

bool isEntityAlive(GameState* gameState, int entityId)
{
    if (entityId < 0 || !gameState->mAdmin.mPool.has(entityId) || gameState->mAdmin.isPendingRemove(entityId))
    {
        return false;
    }

    const auto* mask = gameState->mAdmin.mPool.getMask(entityId);

    if (gameState->mAdmin.mPool.hasComponent<CPlayer>(mask))
    {
        const auto player = gameState->mAdmin.mPool.getComponent<CPlayer>(entityId);
        if (player->respawnTimer > 0)
        {
            return false;
        }
    }

    return true;
}

bool isEntityAlive(GameState* gameState, int entityId, uint32_t expectedGen)
{
    return isEntityAlive(gameState, entityId) && gameState->mAdmin.mPool.getGeneration(entityId) == expectedGen;
}

void applyLevelConfig(GameState* gameState, int level)
{
    auto& campaign = gameState->campaign();

    campaign.currentLevel     = level;
    const auto& config        = getDifficultyConfig(campaign.difficulty);
    campaign.team0UnitCount   = config.team0UnitCount;
    campaign.team1UnitCount   = config.team1UnitCount;
    campaign.requiredSupplies = config.requiredSupplies;
    campaign.unitDeathLimit   = config.unitDeathLimit;
    campaign.unitConfigs      = config.unitConfigs;

    if (campaign.difficulty == 0)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            campaign.unitConfigs[i].maxHp += 2;
            campaign.unitConfigs[i].hpBonus = 2;
        }
        for (size_t i = 3; i < 6; ++i)
        {
            campaign.unitConfigs[i].maxHp   = std::max(2, campaign.unitConfigs[i].maxHp - 2);
            campaign.unitConfigs[i].hpBonus = -2;
        }
    }
    else if (campaign.difficulty == 1)
    {
        for (size_t i = 0; i < 6; ++i)
        {
            campaign.unitConfigs[i].hpBonus = 0;
        }
    }
    else if (campaign.difficulty == 2)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            campaign.unitConfigs[i].maxHp   = std::max(2, campaign.unitConfigs[i].maxHp - 2);
            campaign.unitConfigs[i].hpBonus = -2;
        }
        for (size_t i = 3; i < 6; ++i)
        {
            campaign.unitConfigs[i].maxHp += 4;
            campaign.unitConfigs[i].damageBonus += 1;
            campaign.unitConfigs[i].hpBonus = 4;
        }
    }

    campaign.unitDeathsThisLevel = 0;
    campaign.defensiveWallStock += config.defensiveWallStock;
    campaign.electricWallStock += config.electricWallStock;
    campaign.fireWallStock += config.fireWallStock;
    campaign.gunWallStock += config.gunWallStock;
    campaign.radarWallStock += config.radarWallStock;
    campaign.designatedPointStock += config.designatedPointStock;
    campaign.warriorUnitStock += config.warriorUnitStock;
    campaign.archerUnitStock += config.archerUnitStock;
    campaign.tankUnitStock += config.tankUnitStock;
    campaign.gruntUnitStock += config.gruntUnitStock;
    campaign.shooterUnitStock += config.shooterUnitStock;
    campaign.berserkerUnitStock += config.berserkerUnitStock;
    for (auto& ts : campaign.teamStocks)
    {
        ts.defensiveWallStock += config.defensiveWallStock;
        ts.electricWallStock += config.electricWallStock;
        ts.fireWallStock += config.fireWallStock;
        ts.gunWallStock += config.gunWallStock;
        ts.radarWallStock += config.radarWallStock;
        ts.designatedPointStock += config.designatedPointStock;
        ts.warriorUnitStock += config.warriorUnitStock;
        ts.archerUnitStock += config.archerUnitStock;
        ts.tankUnitStock += config.tankUnitStock;
        ts.gruntUnitStock += config.gruntUnitStock;
        ts.shooterUnitStock += config.shooterUnitStock;
        ts.berserkerUnitStock += config.berserkerUnitStock;
    }
    campaign.collectedSupplies = 0;
    campaign.pendingLevelReset = 0;
    campaign.victory           = false;
}

CWall createWallData(WallKind kind, bool playerPlaced)
{
    CWall wall{};
    wall.kind         = kind;
    wall.playerPlaced = playerPlaced;

    const auto& wallData = GameConfig::kWallData[static_cast<int>(kind)];
    wall.effectRange     = wallData.effectRange;
    wall.cooldownTick    = wallData.cooldownTick;
    wall.cooldownTimer   = wallData.cooldownTick;
    wall.damage          = wallData.damage;

    return wall;
}

int getEntityTeam(const MemoryPool& pool, int entityId)
{
    if (entityId < 0)
        return -1;
    const auto* mask = pool.getMask(entityId);
    if (!mask)
        return -1;
    if (pool.hasComponent<CTeam>(mask))
    {
        return pool.getComponent<CTeam>(entityId)->teamId;
    }
    return -1;
}

int getEntityPlayerId(const MemoryPool& pool, int entityId)
{
    if (entityId < 0)
        return -1;
    const auto* mask = pool.getMask(entityId);
    if (!mask)
        return -1;
    if (pool.hasComponent<CTeam>(mask))
    {
        return pool.getComponent<CTeam>(entityId)->playerId;
    }
    return -1;
}

bool isPlayerWallOwner(const MemoryPool& pool, int wallId, int playerId)
{
    if (wallId < 0 || playerId < 0)
        return true;
    const auto* mask = pool.getMask(wallId);
    if (!mask)
        return false;

    const int entityTeam = getEntityTeam(pool, wallId);
    const int playerTeam = getEntityTeam(pool, playerId);
    if (entityTeam != playerTeam)
    {
        return false;
    }

    if (!pool.hasComponent<CWall>(mask))
        return true;

    const auto wall = pool.getComponent<CWall>(wallId);
    if (wall->kind == WallKind::Rock)
    {
        return false;
    }

    const int wallOwner = getEntityPlayerId(pool, wallId);
    if (wallOwner != -1 && wallOwner != playerId)
    {
        return false;
    }

    return true;
}

bool isFriendlyToPlayer(MemoryPool& pool, int entityId, int playerId)
{
    if (playerId < 0)
        return getEntityTeam(pool, entityId) == 0;
    return getEntityTeam(pool, entityId) == getEntityTeam(pool, playerId);
}

bool areOpposingTeams(MemoryPool& pool, int entityAId, int entityBId)
{
    if (entityAId < 0 || entityBId < 0 || entityAId == entityBId)
        return false;

    int teamA = getEntityTeam(pool, entityAId);
    int teamB = getEntityTeam(pool, entityBId);
    if (teamA < 0 || teamB < 0)
        return false;
    return teamA != teamB;
}

bool areAlliedTeams(MemoryPool& pool, int entityAId, int entityBId)
{
    if (entityAId < 0 || entityBId < 0)
        return false;
    if (entityAId == entityBId)
        return true;
    int teamA = getEntityTeam(pool, entityAId);
    int teamB = getEntityTeam(pool, entityBId);
    if (teamA < 0 || teamB < 0)
        return false;
    return teamA == teamB;
}

int findDraggableEntityAtPosition(const GameState* gameState,
                                  f32::Fix         mouseX,
                                  f32::Fix         mouseY,
                                  bool             checkDesignatedPointOnly,
                                  int              localPlayerId)
{
    const auto& draggableWalls = gameState->mAdmin.mPool.viewComponent<CRectDragDrop>();
    const auto& wallIds        = gameState->mAdmin.mPool.viewIndex<CRectDragDrop>();
    const int   wallCount      = draggableWalls.size();

    for (int i = 0; i < wallCount; i++)
    {
        int         id   = wallIds[i];
        const auto* mask = gameState->mAdmin.mPool.getMask(id);

        if (!gameState->mAdmin.mPool.hasComponent<CRectDragDrop>(mask))
        {
            continue;
        }

        if (checkDesignatedPointOnly)
        {
            if (!(gameState->mAdmin.mPool.hasComponent<CWall>(mask) &&
                  gameState->mAdmin.mPool.getComponent<CWall>(id)->kind == WallKind::DesignatedPoint))
            {
                continue;
            }
        }
        else
        {
        }

        if (gameState->mAdmin.mPool.hasComponent<CWall>(mask))
        {
            const auto wallData = gameState->mAdmin.mPool.getComponent<CWall>(id);
            if (wallData->kind == WallKind::Rock)
            {
                continue;
            }
        }

        if (localPlayerId >= 0 && !isPlayerWallOwner(gameState->mAdmin.mPool, id, localPlayerId))
        {
            continue;
        }

        const auto& wallRect = draggableWalls[i];
        if (mouseX > wallRect.pos.x && mouseX < wallRect.pos.x + wallRect.size.x && mouseY > wallRect.pos.y &&
            mouseY < wallRect.pos.y + wallRect.size.y)
        {
            return id;
        }
    }

    return -1;
}

struct CachedBush
{
    int      id;
    f32::Fix minX;
    f32::Fix minY;
    f32::Fix maxX;
    f32::Fix maxY;
};

struct BushZoneCache
{
    size_t                  lastZoneCount = 0;
    const void*             lastPoolPtr   = nullptr;
    std::vector<CachedBush> bushes;

    void update(const GameState* gameState)
    {
        const auto& zones = gameState->mAdmin.mPool.viewComponent<CEnvironmentZone>();
        const auto& ids   = gameState->mAdmin.mPool.viewIndex<CEnvironmentZone>();
        if (bushes.empty() || lastZoneCount != zones.size() || lastPoolPtr != &zones)
        {
            lastZoneCount = zones.size();
            lastPoolPtr   = &zones;
            bushes.clear();
            for (size_t i = 0; i < zones.size(); ++i)
            {
                if (zones[i].kind == ZoneKind::Bush)
                {
                    bushes.push_back({.id   = ids[i],
                                      .minX = zones[i].pos.x,
                                      .minY = zones[i].pos.y,
                                      .maxX = zones[i].pos.x + zones[i].size.x,
                                      .maxY = zones[i].pos.y + zones[i].size.y});
                }
            }
        }
    }
};

static BushZoneCache sBushCache;

bool isPointInBush(const GameState* gameState, const f32::Vec2& pos)
{
    sBushCache.update(gameState);
    for (const auto& b : sBushCache.bushes)
    {
        if (pos.x >= b.minX && pos.x <= b.maxX && pos.y >= b.minY && pos.y <= b.maxY)
        {
            return true;
        }
    }
    return false;
}

bool isEntityInBush(const GameState* gameState, int entityId)
{
    const auto* mask = gameState->mAdmin.mPool.getMask(entityId);
    if (!gameState->mAdmin.mPool.hasComponent<CPosition>(mask))
    {
        return false;
    }
    const auto pos = gameState->mAdmin.mPool.getComponent<CPosition>(entityId);
    return isPointInBush(gameState, pos->pos);
}

int getBushIndexAt(const GameState* gameState, const f32::Vec2& pos)
{
    sBushCache.update(gameState);
    for (const auto& b : sBushCache.bushes)
    {
        if (pos.x >= b.minX && pos.x <= b.maxX && pos.y >= b.minY && pos.y <= b.maxY)
        {
            return b.id;
        }
    }
    return -1;
}

f32::Fix squaredLength(const f32::Vec2& vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

bool hasLineOfSight(GameState* gameState, const f32::Vec2& start, const f32::Vec2& end)
{
    return hasLineOfSight(static_cast<const GameState*>(gameState), start, end);
}

bool hasLineOfSight(GameState* gameState, int entityAId, int entityBId)
{
    auto&       pool  = gameState->mAdmin.mPool;
    const auto* maskA = pool.getMask(entityAId);
    const auto* maskB = pool.getMask(entityBId);
    if (!pool.hasComponent<CPosition>(maskA) || !pool.hasComponent<CPosition>(maskB))
    {
        return false;
    }

    const auto positionA = pool.getComponent<CPosition>(entityAId);
    const auto positionB = pool.getComponent<CPosition>(entityBId);

    const int bushB = getBushIndexAt(gameState, positionB->pos);
    if (bushB != -1)
    {
        const int bushA = getBushIndexAt(gameState, positionA->pos);
        if (bushA != bushB)
        {
            constexpr f32::Fix kDetectRadiusSq = 70 * 70;
            if (squaredLength(positionA->pos - positionB->pos) > kDetectRadiusSq)
            {
                return false;
            }
        }
    }

    const auto& systemField = gameState->mSystems.get<SystemGameState::SystemNavigationField>();
    return systemField.hasLineOfSight(positionA->pos, positionB->pos, entityBId, true);
}

f32::Fix getHealthRatio(const CHealth& health)
{
    if (health.maxHp <= 0)
    {
        return {0};
    }

    return health.hp / health.maxHp;
}

bool isLowHealth(const CHealth& health)
{
    return getHealthRatio(health) <= kLowHealthRatio;
}

bool isCriticalHealth(const CHealth& health)
{
    return getHealthRatio(health) <= f32::Fix(5) / 100;
}

IComponentBundle* createProjectileBundle(
    GameState*       gameState,
    int              ownerId,
    int              targetId,
    const f32::Vec2& startPosition,
    const f32::Vec2& direction,
    f32::Fix         damage)
{
    auto* position   = gameState->alloc<CPosition>();
    auto* velocity   = gameState->alloc<CVelocity>();
    auto* projectile = gameState->alloc<CProjectile>();

    position->pos     = startPosition;
    position->prevPos = startPosition;
    position->posInit = startPosition;

    velocity->vel = squaredLength(direction) > f32::Fix(1) / 10000 ? f32::normalized(direction) * kProjectileSpeed
                                                                   : f32::Vec2(kProjectileSpeed, f32::Fix(0));

    projectile->ownerId   = ownerId;
    projectile->targetId  = targetId;
    projectile->damage    = damage;
    projectile->lifeTime  = 75;
    projectile->hitRadius = 14;

    return gameState->createBundle(position, velocity, projectile);
}

IComponentBundle* createWallBundle(GameState*       gameState,
                                   const f32::Vec2& position,
                                   const f32::Vec2& size,
                                   WallKind         kind,
                                   bool             playerPlaced,
                                   int              teamId)
{
    auto* wallRect = gameState->alloc<CRectDragDrop>();
    auto* wallData = gameState->alloc<CWall>();
    wallRect->pos  = position;
    wallRect->size = size;
    *wallData      = createWallData(kind, playerPlaced);

    if (kind != WallKind::Rock && teamId >= 0)
    {
        auto* teamComp     = gameState->alloc<CTeam>();
        teamComp->teamId   = teamId;
        teamComp->playerId = -1;

        if (kind == WallKind::DesignatedPoint)
        {
            auto*     wallPos = gameState->alloc<CPosition>();
            f32::Vec2 center  = position + size * f32::Fix(1) / 2;
            wallPos->pos      = center;
            wallPos->posInit  = center;
            wallPos->prevPos  = center;
            return gameState->createBundle(wallRect, wallData, teamComp, wallPos);
        }

        return gameState->createBundle(wallRect, wallData, teamComp);
    }

    if (kind == WallKind::DesignatedPoint)
    {
        auto*     wallPos = gameState->alloc<CPosition>();
        f32::Vec2 center  = position + size * f32::Fix(1) / 2;
        wallPos->pos      = center;
        wallPos->posInit  = center;
        wallPos->prevPos  = center;
        return gameState->createBundle(wallRect, wallData, wallPos);
    }

    return gameState->createBundle(wallRect, wallData);
}

IComponentBundle* createPickupBundle(GameState* gameState, const f32::Vec2& position)
{
    auto* pickup    = gameState->alloc<CPickup>();
    auto* pickupPos = gameState->alloc<CPosition>();

    int r = gameState->mUpdateRandom.randInt(5);
    switch (r)
    {
        case 0:
            pickup->kind = PowerupKind::HealthBoost;
            break;
        case 1:
            pickup->kind = PowerupKind::DamageBoost;
            break;
        case 2:
            pickup->kind = PowerupKind::SpeedBoost;
            break;
        case 3:
            pickup->kind = PowerupKind::AttackRateBoost;
            break;
        case 4:
            pickup->kind = PowerupKind::ShieldBoost;
            break;
    }

    pickup->value      = kPickupValue;
    pickup->lifeTime   = 900;
    pickupPos->pos     = position;
    pickupPos->prevPos = position;
    pickupPos->posInit = position;
    return gameState->createBundle(pickupPos, pickup);
}

IComponentBundle* createEnvironmentZoneBundle(
    GameState*       gameState,
    const f32::Vec2& position,
    const f32::Vec2& size,
    ZoneKind         kind,
    int              level)
{
    auto* zone  = gameState->alloc<CEnvironmentZone>();
    zone->pos   = position;
    zone->size  = size;
    zone->kind  = kind;
    zone->level = level;
    return gameState->createBundle(zone);
}

void emitUnitEvent(GameState* gameState, int entityId, UnitEventKind event, int targetId)
{
    SystemGameState::SystemUnit::handleUnitTransition(gameState, entityId, event, targetId);
}

void updateChunkMembership(GameState* gameState, int entityId, const f32::Vec2& previousPosition, const f32::Vec2& currentPosition)
{
    const auto& collision        = gameState->collision();
    const int   previousChunkRow = static_cast<int>(previousPosition.y / collision.spacing);
    const int   previousChunkCol = static_cast<int>(previousPosition.x / collision.spacing);
    const int   currentChunkRow  = static_cast<int>(currentPosition.y / collision.spacing);
    const int   currentChunkCol  = static_cast<int>(currentPosition.x / collision.spacing);

    if (previousChunkCol != currentChunkCol || previousChunkRow != currentChunkRow)
    {
        gameState->collision().map.remove(previousChunkRow, previousChunkCol, entityId);
        gameState->collision().map.insert(currentChunkRow, currentChunkCol, entityId);
    }
}

void applySteering(CVelocity& velocity, f32::Vec2 desiredDirection, f32::Fix maxSpeed, f32::Fix steerWeight)
{
    if (squaredLength(desiredDirection) > f32::Fix(1) / 10000)
    {
        desiredDirection          = f32::normalized(desiredDirection) * maxSpeed;
        velocity.desiredDirection = desiredDirection;

        const f32::Vec2 steeringForce = (desiredDirection - velocity.vel) * steerWeight;
        velocity.vel += steeringForce;
    }
    else
    {
        velocity.desiredDirection = {};
        velocity.vel              = velocity.vel * f32::Fix(7) / 10;
    }

    velocity.vel += velocity.pushDirection + velocity.separationForce * kSeparationWeight;
    velocity.pushDirection *= kPushDecay;
}

void moveEntityWithObstacles(GameState* gameState, int entityId)
{
    const auto& nav = gameState->nav();

    auto  position = gameState->mAdmin.mPool.getComponent<CPosition>(entityId);
    auto  velocity = gameState->mAdmin.mPool.getComponent<CVelocity>(entityId);
    auto& pool     = gameState->mAdmin.mPool;

    const auto& obstacleGrid = nav.mGrid;

    const f32::Vec2 nextXYPosition = position->pos + f32::Vec2(velocity->vel.x, velocity->vel.y);
    const int       nextXYColumn   = static_cast<int>(nextXYPosition.x / nav.spacing);
    const int       nextXYRow      = static_cast<int>(nextXYPosition.y / nav.spacing);

    const f32::Vec2 nextXPosition = position->pos + f32::Vec2(velocity->vel.x, f32::Fix(0));
    const int       nextXColumn   = static_cast<int>(nextXPosition.x / nav.spacing);
    const int       nextXRow      = static_cast<int>(nextXPosition.y / nav.spacing);

    const f32::Vec2 nextYPosition = position->pos + f32::Vec2(f32::Fix(0), velocity->vel.y);
    const int       nextYColumn   = static_cast<int>(nextYPosition.x / nav.spacing);
    const int       nextYRow      = static_cast<int>(nextYPosition.y / nav.spacing);

    const bool canMoveXY = obstacleGrid.isValidCell(nextXYColumn, nextXYRow) && obstacleGrid[nextXYColumn][nextXYRow] == 0;
    const bool canMoveX = obstacleGrid.isValidCell(nextXColumn, nextXRow) && obstacleGrid[nextXColumn][nextXRow] == 0;
    const bool canMoveY = obstacleGrid.isValidCell(nextYColumn, nextYRow) && obstacleGrid[nextYColumn][nextYRow] == 0;

    const f32::Fix kMinSlideSpeed = f32::Fix(8) / 10;
    auto           boostSlide     = [&](f32::Fix& speed, f32::Fix desired)
    {
        if (f32::abs(speed) < kMinSlideSpeed)
        {
            if (speed > f32::Fix(1) / 1000 || desired > f32::Fix(1) / 1000)
            {
                speed = kMinSlideSpeed;
            }
            else if (speed < -f32::Fix(1) / 1000 || desired < -f32::Fix(1) / 1000)
            {
                speed = -kMinSlideSpeed;
            }
        }
    };

    if (canMoveXY)
    {
    }
    else if (canMoveX && !canMoveY)
    {
        velocity->vel.y = -velocity->vel.y / 10;
        boostSlide(velocity->vel.x, velocity->desiredDirection.x);
    }
    else if (!canMoveX && canMoveY)
    {
        velocity->vel.x = -velocity->vel.x / 10;
        boostSlide(velocity->vel.y, velocity->desiredDirection.y);
    }
    else if (canMoveX && canMoveY)
    {
        if (f32::abs(velocity->vel.x) >= f32::abs(velocity->vel.y))
        {
            velocity->vel.y = -velocity->vel.y / 10;
            boostSlide(velocity->vel.x, velocity->desiredDirection.x);
        }
        else
        {
            velocity->vel.x = -velocity->vel.x / 10;
            boostSlide(velocity->vel.y, velocity->desiredDirection.y);
        }
    }
    else
    {
        const auto* mask = pool.getMask(entityId);
        if (pool.hasComponent<CUnit>(mask) || pool.hasComponent<CTeam>(mask))
        {
            const int  teamId         = getEntityTeam(pool, entityId);
            const bool isTactical     = pool.hasComponent<CUnit>(mask) &&
                                        (pool.getComponent<CUnit>(entityId)->stance == CombatStance::Tactical);
            auto&      directionField = isTactical ? gameState->nav().getDesignatedField(teamId)
                                                   : gameState->nav().getTeamField(teamId);

            const int cellX = static_cast<int>(position->pos.x / gameState->nav().spacing);
            const int cellY = static_cast<int>(position->pos.y / gameState->nav().spacing);

            auto direction = directionField.getDirections(cellX, cellY);
            if (direction.x == 0 && direction.y == 0)
            {
                direction = directionField.getValidDirectionNear(cellX, cellY);
            }

            velocity->vel.x = direction.x;
            velocity->vel.y = direction.y;
        }
    }

    const auto* boundMask = pool.getMask(entityId);

    const auto& envZones = pool.viewComponent<CEnvironmentZone>();
    for (const auto& zone : envZones)
    {
        if (position->pos.x >= zone.pos.x && position->pos.x <= zone.pos.x + zone.size.x &&
            position->pos.y >= zone.pos.y && position->pos.y <= zone.pos.y + zone.size.y)
        {
            if (zone.kind == ZoneKind::Water)
            {
                velocity->vel = velocity->vel * f32::Fix(65) / 100;
            }
            else if (zone.kind == ZoneKind::Quicksand)
            {
                velocity->vel = velocity->vel * f32::Fix(55) / 100;
            }
            else if (zone.kind == ZoneKind::Lava)
            {
                velocity->vel = velocity->vel * f32::Fix(60) / 100;
                if ((gameState->mTick % 30 == 0) && pool.hasComponent<CHealth>(boundMask))
                {
                    auto health = pool.getComponent<CHealth>(entityId);
                    if (health->hp > 1)
                    {
                        health->hp -= 1;
                    }
                }
            }
            break;
        }
    }

    const f32::Vec2 previousPosition = position->pos;
    position->prevPos                = previousPosition;
    position->pos += velocity->vel;

    f32::Fix paddingX = 0;
    f32::Fix paddingY = 0;
    if (pool.hasComponent<CBoundCircle>(boundMask))
    {
        const f32::Fix radius = pool.getComponent<CBoundCircle>(entityId)->radius;
        paddingX              = radius;
        paddingY              = radius;
    }

    position->pos.x = std::clamp(position->pos.x, paddingX, gameState->mapWidth() - paddingX);
    position->pos.y = std::clamp(position->pos.y, paddingY, gameState->mapHeight() - paddingY);

    updateChunkMembership(gameState, entityId, previousPosition, position->pos);
}

f32::Fix getDistanceBetweenEntities(GameState* gameState, int entityAId, int entityBId)
{
    auto& pool      = gameState->mAdmin.mPool;
    auto  positionA = pool.getComponent<CPosition>(entityAId);
    auto  positionB = pool.getComponent<CPosition>(entityBId);

    const auto* maskB = pool.getMask(entityBId);
    if (pool.hasComponent<CBase>(maskB) && pool.hasComponent<CRectDragDrop>(maskB))
    {
        const auto     rect   = pool.getComponent<CRectDragDrop>(entityBId);
        const f32::Fix clampX = std::clamp(positionA->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
        const f32::Fix clampY = std::clamp(positionA->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
        return f32::length(positionA->pos - f32::Vec2{clampX, clampY});
    }

    const auto* maskA = pool.getMask(entityAId);
    if (pool.hasComponent<CBase>(maskA) && pool.hasComponent<CRectDragDrop>(maskA))
    {
        const auto     rect   = pool.getComponent<CRectDragDrop>(entityAId);
        const f32::Fix clampX = std::clamp(positionB->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
        const f32::Fix clampY = std::clamp(positionB->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
        return f32::length(positionB->pos - f32::Vec2{clampX, clampY});
    }

    return f32::length(positionB->pos - positionA->pos);
}

int findClosestOpponent(GameState* gameState, int entityId, f32::Fix maxRange)
{
    const auto& collision = gameState->collision();
    auto&       pool      = gameState->mAdmin.mPool;

    const auto* selfMask = pool.getMask(entityId);
    if (!pool.hasComponent<CPosition>(selfMask))
    {
        return -1;
    }

    const auto position = pool.getComponent<CPosition>(entityId);
    const int  row      = static_cast<int>(position->pos.y / collision.spacing);
    const int  col      = static_cast<int>(position->pos.x / collision.spacing);
    const int  radius   = std::max(1, static_cast<int>(f32::ceil(maxRange / collision.spacing)));

    int      closestOpponentId = -1;
    f32::Fix closestDistanceSq = maxRange * maxRange;

    for (int rowOffset = -radius; rowOffset <= radius; rowOffset++)
    {
        for (int colOffset = -radius; colOffset <= radius; colOffset++)
        {
            const int neighborRow = row + rowOffset;
            const int neighborCol = col + colOffset;

            if (!collision.map.isValid(neighborRow, neighborCol))
            {
                continue;
            }

            for (const int otherEntityId : collision.map.get(neighborRow, neighborCol).getEntityInCell())
            {
                if (otherEntityId == entityId || !isEntityAlive(gameState, otherEntityId))
                {
                    continue;
                }

                const auto* otherMask = pool.getMask(otherEntityId);
                bool        isTarget  = pool.hasComponent<CUnit>(otherMask) || pool.hasComponent<CPlayer>(otherMask) ||
                                        pool.hasComponent<CBase>(otherMask);
                if (!isTarget || !areOpposingTeams(pool, entityId, otherEntityId))
                {
                    continue;
                }
                if (pool.hasComponent<CUnit>(otherMask) &&
                    pool.getComponent<CUnit>(otherEntityId)->state == UnitStateKind::Explode)
                {
                    continue;
                }

                const auto otherPosition = pool.getComponent<CPosition>(otherEntityId);
                f32::Fix   distanceSq;
                if (pool.hasComponent<CBase>(otherMask) && pool.hasComponent<CRectDragDrop>(otherMask))
                {
                    const auto     rect   = pool.getComponent<CRectDragDrop>(otherEntityId);
                    const f32::Fix clampX = std::clamp(position->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
                    const f32::Fix clampY = std::clamp(position->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
                    distanceSq            = squaredLength(position->pos - f32::Vec2{clampX, clampY});
                }
                else
                {
                    distanceSq = squaredLength(otherPosition->pos - position->pos);
                }

                const int otherBush = getBushIndexAt(gameState, otherPosition->pos);
                if (otherBush != -1)
                {
                    const int selfBush = getBushIndexAt(gameState, position->pos);
                    if (selfBush != otherBush && distanceSq > f32::Fix(70 * 70))
                    {
                        continue;
                    }
                }

                if (distanceSq < closestDistanceSq || (distanceSq == closestDistanceSq && otherEntityId < closestOpponentId))
                {
                    closestDistanceSq = distanceSq;
                    closestOpponentId = otherEntityId;
                }
            }
        }
    }

    if (closestOpponentId == -1)
    {
        return findClosestBase(gameState, entityId, maxRange);
    }

    return closestOpponentId;
}

std::vector<int> findAgentInRange(GameState* gameState, f32::Vec2 origin, f32::Fix range)
{
    const auto& collision = gameState->collision();
    auto&       pool      = gameState->mAdmin.mPool;

    f32::Fix spacing = collision.spacing;
    int      maxX    = static_cast<int>((origin.x + range) / spacing);
    int      minX    = static_cast<int>((origin.x - range) / spacing);
    int      maxY    = static_cast<int>((origin.y + range) / spacing);
    int      minY    = static_cast<int>((origin.y - range) / spacing);

    std::vector<int> agents;

    for (int col = minX; col <= maxX; col++)
    {
        for (int row = minY; row <= maxY; row++)
        {
            if (!collision.map.isValid(row, col))
            {
                continue;
            }

            for (const int entityId : collision.map.get(row, col).getEntityInCell())
            {
                const auto* entityMask = pool.getMask(entityId);
                const bool  isAgent    = pool.hasComponent<CUnit>(entityMask) || pool.hasComponent<CPlayer>(entityMask);
                if (isAgent)
                {
                    if (!isEntityAlive(gameState, entityId) || !pool.hasComponent<CPosition>(entityMask))
                    {
                        continue;
                    }
                    if (pool.hasComponent<CUnit>(entityMask) &&
                        pool.getComponent<CUnit>(entityId)->state == UnitStateKind::Explode)
                    {
                        continue;
                    }

                    const f32::Fix distanceSq = squaredLength(pool.getComponent<CPosition>(entityId)->pos - origin);
                    if (distanceSq <= range * range)
                    {
                        agents.emplace_back(entityId);
                    }
                }
            }
        }
    }

    for (const int baseId : pool.viewIndex<CBase>())
    {
        const auto* baseMask = pool.getMask(baseId);
        if (!isEntityAlive(gameState, baseId) || !pool.hasComponent<CPosition>(baseMask))
        {
            continue;
        }

        f32::Fix distanceSq;
        if (pool.hasComponent<CRectDragDrop>(baseMask))
        {
            const auto     rect   = pool.getComponent<CRectDragDrop>(baseId);
            const f32::Fix clampX = std::clamp(origin.x, rect->pos.x, rect->pos.x + rect->size.x);
            const f32::Fix clampY = std::clamp(origin.y, rect->pos.y, rect->pos.y + rect->size.y);
            distanceSq            = squaredLength(origin - f32::Vec2{clampX, clampY});
        }
        else
        {
            distanceSq = squaredLength(pool.getComponent<CPosition>(baseId)->pos - origin);
        }

        if (distanceSq <= range * range)
        {
            agents.emplace_back(baseId);
        }
    }

    return agents;
}

int findClosestOpponentToPoint(GameState* gameState, const f32::Vec2& origin, f32::Fix maxRange, int searchingEntityId, int searchingTeamId)
{
    const auto& collision = gameState->collision();
    auto&       pool      = gameState->mAdmin.mPool;

    int      closestOpponentId = -1;
    f32::Fix closestDistSq     = maxRange * maxRange;

    f32::Fix spacing = collision.spacing;
    int      maxX    = static_cast<int>((origin.x + maxRange) / spacing);
    int      minX    = static_cast<int>((origin.x - maxRange) / spacing);
    int      maxY    = static_cast<int>((origin.y + maxRange) / spacing);
    int      minY    = static_cast<int>((origin.y - maxRange) / spacing);

    for (int col = minX; col <= maxX; col++)
    {
        for (int row = minY; row <= maxY; row++)
        {
            if (!collision.map.isValid(row, col))
            {
                continue;
            }

            for (const int entityId : collision.map.get(row, col).getEntityInCell())
            {
                if (entityId == searchingEntityId)
                {
                    continue;
                }

                const auto* entityMask = pool.getMask(entityId);
                const int   entityTeam = getEntityTeam(pool, entityId);
                if (entityTeam < 0)
                {
                    continue;
                }

                bool isOpponent = false;
                if (searchingEntityId != -1)
                {
                    isOpponent = areOpposingTeams(pool, searchingEntityId, entityId);
                }
                else if (searchingTeamId != -1)
                {
                    isOpponent = (entityTeam != searchingTeamId);
                }
                else
                {
                    isOpponent = (entityTeam != 0);
                }

                if (isOpponent)
                {
                    if (!isEntityAlive(gameState, entityId) || !pool.hasComponent<CPosition>(entityMask))
                    {
                        continue;
                    }
                    if (pool.hasComponent<CUnit>(entityMask) &&
                        pool.getComponent<CUnit>(entityId)->state == UnitStateKind::Explode)
                    {
                        continue;
                    }

                    const f32::Fix distanceSq = squaredLength(pool.getComponent<CPosition>(entityId)->pos - origin);
                    if (distanceSq < closestDistSq || (distanceSq == closestDistSq && entityId < closestOpponentId))
                    {
                        closestDistSq     = distanceSq;
                        closestOpponentId = entityId;
                    }
                }
            }
        }
    }

    for (const int baseId : pool.viewIndex<CBase>())
    {
        if (baseId == searchingEntityId || !isEntityAlive(gameState, baseId))
        {
            continue;
        }

        const int baseTeam = getEntityTeam(pool, baseId);
        if (baseTeam < 0)
        {
            continue;
        }

        bool isOpponent = false;
        if (searchingEntityId != -1)
        {
            isOpponent = areOpposingTeams(pool, searchingEntityId, baseId);
        }
        else if (searchingTeamId != -1)
        {
            isOpponent = (baseTeam != searchingTeamId);
        }
        else
        {
            isOpponent = (baseTeam != 0);
        }

        if (!isOpponent)
        {
            continue;
        }

        const auto* baseMask = pool.getMask(baseId);
        if (!pool.hasComponent<CPosition>(baseMask))
        {
            continue;
        }

        f32::Fix distanceSq;
        if (pool.hasComponent<CRectDragDrop>(baseMask))
        {
            const auto     rect   = pool.getComponent<CRectDragDrop>(baseId);
            const f32::Fix clampX = std::clamp(origin.x, rect->pos.x, rect->pos.x + rect->size.x);
            const f32::Fix clampY = std::clamp(origin.y, rect->pos.y, rect->pos.y + rect->size.y);
            distanceSq            = squaredLength(origin - f32::Vec2{clampX, clampY});
        }
        else
        {
            const auto basePos = pool.getComponent<CPosition>(baseId);
            distanceSq         = squaredLength(basePos->pos - origin);
        }

        if (distanceSq < closestDistSq || (distanceSq == closestDistSq && baseId < closestOpponentId))
        {
            closestDistSq     = distanceSq;
            closestOpponentId = baseId;
        }
    }

    return closestOpponentId;
}

int findClosestBase(GameState* gameState, int entityId, f32::Fix maxRange)
{
    auto& pool = gameState->mAdmin.mPool;

    const auto* entityMask = pool.getMask(entityId);
    if (!pool.hasComponent<CPosition>(entityMask))
    {
        return -1;
    }

    const auto position = pool.getComponent<CPosition>(entityId);

    int      closestBaseId = -1;
    f32::Fix closestDistSq = maxRange * maxRange;

    for (const int baseId : pool.viewIndex<CBase>())
    {
        const auto* baseMask = pool.getMask(baseId);
        if (!isEntityAlive(gameState, baseId) || !pool.hasComponent<CPosition>(baseMask))
        {
            continue;
        }

        if (!areOpposingTeams(pool, entityId, baseId))
        {
            continue;
        }

        f32::Fix distanceSq;
        if (pool.hasComponent<CRectDragDrop>(baseMask))
        {
            const auto     rect   = pool.getComponent<CRectDragDrop>(baseId);
            const f32::Fix clampX = std::clamp(position->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
            const f32::Fix clampY = std::clamp(position->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
            distanceSq            = squaredLength(position->pos - f32::Vec2{clampX, clampY});
        }
        else
        {
            const auto basePosition = pool.getComponent<CPosition>(baseId);
            distanceSq              = squaredLength(basePosition->pos - position->pos);
        }

        if (distanceSq < closestDistSq || (distanceSq == closestDistSq && baseId < closestBaseId))
        {
            closestDistSq = distanceSq;
            closestBaseId = baseId;
        }
    }

    return closestBaseId;
}

void applyWallDamage(GameState* gameState, int targetId, f32::Fix damage, const f32::Vec2& hitOrigin, f32::Fix pushScale)
{
    auto&       pool       = gameState->mAdmin.mPool;
    const auto* targetMask = pool.getMask(targetId);
    if (!isEntityAlive(gameState, targetId) || !pool.hasComponent<CHealth>(targetMask) ||
        !pool.hasComponent<CPosition>(targetMask))
    {
        return;
    }

    if (pool.hasComponent<CUnit>(targetMask))
    {
        auto unit = pool.getComponent<CUnit>(targetId);
        if (unit->buff.shieldHp > 0)
        {
            f32::Fix block = std::min(unit->buff.shieldHp, damage);
            unit->buff.shieldHp -= block;
            damage -= block;
        }
    }
    else if (pool.hasComponent<CPlayer>(targetMask))
    {
        auto player = pool.getComponent<CPlayer>(targetId);
        if (player->buff.shieldHp > 0)
        {
            f32::Fix block = std::min(player->buff.shieldHp, damage);
            player->buff.shieldHp -= block;
            damage -= block;
        }
    }

    auto health = pool.getComponent<CHealth>(targetId);
    if (health->armor > 0)
    {
        f32::Fix armorBlock = std::min(health->armor, damage);
        health->armor -= armorBlock;
        damage -= armorBlock;
    }
    health->hp -= damage;

    if (pool.hasComponent<CVelocity>(targetMask))
    {
        auto direction = pool.getComponent<CPosition>(targetId)->pos - hitOrigin;
        if (squaredLength(direction) > f32::Fix(1) / 10000)
        {
            direction = f32::normalized(direction);
        }
        pool.getComponent<CVelocity>(targetId)->pushDirection += direction * pushScale;
    }

    auto& hitClock = gameState->hitClock;
    if (hitClock > 5)
    {
        hitClock         = 0;
        f32::Vec2 hitPos = pool.hasComponent<CPosition>(targetMask) ? pool.getComponent<CPosition>(targetId)->pos
                                                                    : f32::Vec2{};
        gameState->playSound("SndHit", hitPos, f32::Fix(7) / 10);
    }
    resetIfDead(gameState, targetId);
}

void resetUnitToSpawn(GameState* gameState, int entityId)
{
    auto&       pool       = gameState->mAdmin.mPool;
    const auto* entityMask = pool.getMask(entityId);
    if (!isEntityAlive(gameState, entityId) || !pool.hasComponent<CHealth>(entityMask) ||
        !pool.hasComponent<CPosition>(entityMask) || !pool.hasComponent<CVelocity>(entityMask))
    {
        return;
    }

    auto health   = pool.getComponent<CHealth>(entityId);
    auto position = pool.getComponent<CPosition>(entityId);
    auto velocity = pool.getComponent<CVelocity>(entityId);

    const f32::Vec2 prevPos    = position->pos;
    health->hp                 = health->maxHp;
    position->pos              = position->posInit;
    position->prevPos          = position->posInit;
    velocity->vel              = {};
    velocity->pushDirection    = {};
    velocity->desiredDirection = {};
    velocity->separationForce  = {};

    updateChunkMembership(gameState, entityId, prevPos, position->pos);

    if (pool.hasComponent<CUnit>(entityMask))
    {
        auto unit         = pool.getComponent<CUnit>(entityId);
        unit->state       = UnitStateKind::Search;
        unit->targetId    = -1;
        unit->searchTimer = 0;
    }

    gameState->nav().requestRecompute();
}

bool resetIfDead(GameState* gameState, int entityId)
{
    auto&       pool = gameState->mAdmin.mPool;
    const auto* mask = pool.getMask(entityId);
    if (!isEntityAlive(gameState, entityId) || !pool.hasComponent<CHealth>(mask))
    {
        return false;
    }

    auto health = pool.getComponent<CHealth>(entityId);

    if (pool.hasComponent<CPlayer>(mask))
    {
        if (health->hp > 0)
        {
            return false;
        }
    }
    else if (pool.hasComponent<CUnit>(mask))
    {
        auto unit = pool.getComponent<CUnit>(entityId);
        if (unit->state == UnitStateKind::Explode)
        {
            if (unit->explodeTimer > 0)
            {
                return false;
            }
        }
        else
        {
            if (health->hp > 0)
            {
                return false;
            }

            if (unit->unitClass == UnitClass::Tank && unit->state != UnitStateKind::Explode)
            {
                unit->state        = UnitStateKind::Explode;
                unit->explodeTimer = 10;
                unit->targetId     = -1;
                health->hp         = 0;
                health->armor      = 0;
                if (pool.hasComponent<CBoundCircle>(mask))
                {
                    pool.getComponent<CBoundCircle>(entityId)->radius = 0;
                }
                if (pool.hasComponent<CVelocity>(mask))
                {
                    auto velocity              = pool.getComponent<CVelocity>(entityId);
                    velocity->vel              = {};
                    velocity->pushDirection    = {};
                    velocity->desiredDirection = {};
                    velocity->separationForce  = {};
                }
                return false;
            }
        }
    }
    else
    {
        if (health->hp > 0)
        {
            return false;
        }
    }

    if (pool.hasComponent<CPlayer>(mask))
    {
        auto player          = pool.getComponent<CPlayer>(entityId);
        player->respawnTimer = 180;
        health->hp           = health->maxHp;
        health->armor        = f32::Fix(5);

        f32::Vec2 diePos{};

        if (pool.hasComponent<CPosition>(mask))
        {
            auto position = pool.getComponent<CPosition>(entityId);
            diePos        = position->pos;

            auto* effect            = gameState->alloc<CVisualEffect>();
            effect->kind            = EffectKind::DeathExplosion;
            effect->startPos        = position->pos;
            effect->endPos          = {};
            effect->lifeTime        = 30;
            effect->initialLifeTime = 30;
            const int playerTeam    = getEntityTeam(pool, entityId);
            effect->teamId          = playerTeam;
            gameState->mAdmin.add(gameState->createBundle(effect));

            f32::Vec2 respawnPos        = position->posInit;
            bool      foundMainBase     = false;
            int       fallbackSubBaseId = -1;

            for (int baseId : pool.viewIndex<CBase>())
            {
                if (getEntityTeam(pool, baseId) != playerTeam)
                    continue;

                const auto* baseMask = pool.getMask(baseId);
                if (pool.hasComponent<CRectDragDrop>(baseMask) && pool.hasComponent<CPosition>(baseMask))
                {
                    const auto rect = pool.getComponent<CRectDragDrop>(baseId);
                    if (rect->size.x > f32::Fix(120))
                    {
                        respawnPos = pool.getComponent<CPosition>(baseId)->pos + f32::Vec2(f32::Fix(0), f32::Fix(100));
                        foundMainBase = true;
                        break;
                    }
                    else if (fallbackSubBaseId == -1)
                    {
                        fallbackSubBaseId = baseId;
                    }
                }
            }

            if (!foundMainBase && fallbackSubBaseId != -1)
            {
                respawnPos = pool.getComponent<CPosition>(fallbackSubBaseId)->pos + f32::Vec2(f32::Fix(0), f32::Fix(80));
            }

            position->pos     = respawnPos;
            position->prevPos = respawnPos;
            updateChunkMembership(gameState, entityId, diePos, respawnPos);
        }

        if (pool.hasComponent<CVelocity>(mask))
        {
            auto velocity              = pool.getComponent<CVelocity>(entityId);
            velocity->vel              = {};
            velocity->pushDirection    = {};
            velocity->desiredDirection = {};
            velocity->separationForce  = {};
        }

        player->buff.shieldHp = 0;

        gameState->playSound("SndDie", diePos, f32::Fix(1));
        return false;
    }
    else if (pool.hasComponent<CUnit>(mask))
    {
        const int entityTeam = getEntityTeam(pool, entityId);
        if (gameState->getGameMode() == GameMode::Coop)
        {
            if (entityTeam == 0)
            {
                gameState->campaign().unitDeathsThisLevel++;
            }
            else
            {
                if (pool.hasComponent<CPosition>(mask))
                {
                    gameState->mAdmin.add(createPickupBundle(gameState, pool.getComponent<CPosition>(entityId)->pos));
                }
            }
        }

        if (pool.hasComponent<CPosition>(mask))
        {
            auto* effect            = gameState->alloc<CVisualEffect>();
            effect->kind            = EffectKind::DeathExplosion;
            effect->startPos        = pool.getComponent<CPosition>(entityId)->pos;
            effect->endPos          = {};
            effect->lifeTime        = 20;
            effect->initialLifeTime = 20;
            effect->teamId          = entityTeam;
            gameState->mAdmin.add(gameState->createBundle(effect));
        }
    }
    else if (pool.hasComponent<CBase>(mask) && pool.hasComponent<CPosition>(mask))
    {
        auto* effect            = gameState->alloc<CVisualEffect>();
        effect->kind            = EffectKind::DeathExplosion;
        effect->startPos        = pool.getComponent<CPosition>(entityId)->pos;
        effect->endPos          = {};
        effect->lifeTime        = 30;
        effect->initialLifeTime = 30;
        effect->teamId          = getEntityTeam(pool, entityId);
        gameState->mAdmin.add(gameState->createBundle(effect));

        gameState->nav().requestRecompute();
    }
    else if (pool.hasComponent<CWall>(mask))
    {
        if (pool.hasComponent<CPosition>(mask))
        {
            auto* effect            = gameState->alloc<CVisualEffect>();
            effect->kind            = EffectKind::DeathExplosion;
            effect->startPos        = pool.getComponent<CPosition>(entityId)->pos;
            effect->endPos          = {};
            effect->lifeTime        = 20;
            effect->initialLifeTime = 20;
            effect->teamId          = getEntityTeam(pool, entityId);
            gameState->mAdmin.add(gameState->createBundle(effect));
        }
        gameState->nav().requestRecompute();
    }

    f32::Vec2 diePos   = pool.hasComponent<CPosition>(mask) ? pool.getComponent<CPosition>(entityId)->pos : f32::Vec2{};
    bool      isPlayer = pool.hasComponent<CPlayer>(mask);
    gameState->playSound("SndDie", diePos, isPlayer ? f32::Fix(1) : f32::Fix(4) / 10);
    gameState->mAdmin.remove(entityId);

    return true;
}

void fireProjectile(GameState* gameState, int entityId, int targetId, const CUnit& unit)
{
    auto&       pool       = gameState->mAdmin.mPool;
    const auto* selfMask   = pool.getMask(entityId);
    const auto* targetMask = pool.getMask(targetId);
    if (!pool.hasComponent<CPosition>(selfMask) || !pool.hasComponent<CPosition>(targetMask))
    {
        return;
    }

    const auto selfPosition   = pool.getComponent<CPosition>(entityId);
    const auto targetPosition = pool.getComponent<CPosition>(targetId);

    f32::Vec2 targetPoint = targetPosition->pos;
    if (pool.hasComponent<CBase>(targetMask) && pool.hasComponent<CRectDragDrop>(targetMask))
    {
        const auto     rect   = pool.getComponent<CRectDragDrop>(targetId);
        const f32::Fix clampX = std::clamp(selfPosition->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
        const f32::Fix clampY = std::clamp(selfPosition->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
        targetPoint           = f32::Vec2{clampX, clampY};
    }
    const auto direction = targetPoint - selfPosition->pos;

    gameState->mAdmin.add(createProjectileBundle(gameState, entityId, targetId, selfPosition->pos, direction, unit.damage));

    auto& shootClock = gameState->shootClock;
    if (shootClock > 5)
    {
        shootClock = 0;
        gameState->playSound("SndShoot", selfPosition->pos, f32::Fix(75) / 100);
    }
}

void spawnAgent(GameState* gameState, const f32::Vec2& spawnPosition, UnitClass unitClass, bool isSpawnAtMainBase, int teamId)
{
    if (teamId == -1)
    {
        teamId = 0;
    }

    auto* position = gameState->alloc<CPosition>();
    auto* velocity = gameState->alloc<CVelocity>();
    auto* bound    = gameState->alloc<CBoundCircle>();
    auto* health   = gameState->alloc<CHealth>();
    auto* team     = gameState->alloc<CTeam>();
    auto* unit     = gameState->alloc<CUnit>();

    team->teamId   = teamId;
    team->playerId = -1;

    unit->unitClass     = unitClass;
    const auto& unitCfg = gameState->campaign().getUnitConfig(unitClass);

    switch (unitClass)
    {
        case UnitClass::Warrior:
            unit->attackRange    = 36;
            unit->engageRange    = 44;
            unit->disengageRange = 72;
            unit->moveSpeed      = f32::Fix(11) / 10;
            unit->attackCooldown = 16;
            unit->damage         = 2;
            bound->radius        = 11;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor;
            break;
        case UnitClass::Archer:
            unit->attackRange    = 155;
            unit->engageRange    = 185;
            unit->disengageRange = 230;
            unit->moveSpeed      = f32::Fix(65) / 100;
            unit->attackCooldown = 28;
            unit->damage         = f32::Fix(3) / 2;
            bound->radius        = 9;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor;
            break;
        case UnitClass::Tank:
            unit->attackRange    = 36;
            unit->engageRange    = 38;
            unit->disengageRange = 60;
            unit->moveSpeed      = f32::Fix(55) / 100;
            unit->attackCooldown = 24;
            unit->damage         = 2;
            bound->radius        = 15;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor > 0 ? unitCfg.armor : 5;
            break;
        case UnitClass::Grunt:
            unit->attackRange    = 36;
            unit->engageRange    = 60;
            unit->disengageRange = 100;
            unit->moveSpeed      = f32::Fix(95) / 100;
            unit->attackCooldown = 18;
            unit->damage         = 2;
            bound->radius        = 11;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor;
            break;
        case UnitClass::Shooter:
            unit->attackRange    = 155;
            unit->engageRange    = 130;
            unit->disengageRange = 160;
            unit->moveSpeed      = f32::Fix(85) / 100;
            unit->attackCooldown = 30;
            unit->damage         = 1;
            bound->radius        = 9;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor;
            break;
        case UnitClass::Berserker:
            unit->attackRange    = 36;
            unit->engageRange    = 60;
            unit->disengageRange = 100;
            unit->moveSpeed      = f32::Fix(135) / 100;
            unit->attackCooldown = 10;
            unit->damage         = 2;
            bound->radius        = 12;
            health->maxHp        = unitCfg.maxHp;
            health->armor        = unitCfg.armor;
            break;
    }

    position->posInit = spawnPosition;
    position->pos     = spawnPosition;
    position->prevPos = spawnPosition;
    health->hp        = health->maxHp;

    if (isSpawnAtMainBase)
    {
        unit->damage += 1;
    }

    unit->damage += unitCfg.damageBonus;
    if (gameState->getGameMode() == GameMode::Coop && teamId != 0)
    {
        unit->attackCooldown = std::max(10, unit->attackCooldown - gameState->campaign().currentLevel);
    }

    unit->state      = UnitStateKind::Search;
    unit->stance     = CombatStance::Attack;
    unit->isSelected = false;

    gameState->mAdmin.add(gameState->createBundle(bound, unit, position, velocity, health, team));
}

void queueTeamPopulation(GameState* gameState, int count, int teamId)
{
    std::vector<f32::Vec2> basePositions;
    f32::Vec2              mainBasePos;
    bool                   foundMainBase = false;

    auto& pool = gameState->mAdmin.mPool;
    for (const int baseId : pool.viewIndex<CBase>())
    {
        if (getEntityTeam(pool, baseId) != teamId)
            continue;

        const auto* baseMask = pool.getMask(baseId);
        if (pool.hasComponent<CPosition>(baseMask))
        {
            const auto pos = pool.getComponent<CPosition>(baseId);
            basePositions.push_back(pos->pos);

            if (pool.hasComponent<CRectDragDrop>(baseMask))
            {
                const auto rect = pool.getComponent<CRectDragDrop>(baseId);
                if (rect->size.x > f32::Fix(120))
                {
                    mainBasePos   = pos->pos;
                    foundMainBase = true;
                }
            }
        }
    }

    if (basePositions.empty())
    {
        if (teamId == 1)
        {
            basePositions.assign(kTeam1SpawnPositions.begin(), kTeam1SpawnPositions.end());
        }
        else
        {
            basePositions.assign(kTeam0SpawnPositions.begin(), kTeam0SpawnPositions.end());
        }
    }

    const std::array<UnitClass, 5> kTeamSpawnClasses =
        {UnitClass::Warrior, UnitClass::Archer, UnitClass::Warrior, UnitClass::Archer, UnitClass::Tank};

    int mainBaseCount      = 0;
    int secondaryBaseCount = 0;

    const int defaultCount   = gameState->campaign().team0UnitCount;
    const int totalTeamCount = (count > 0) ? count : defaultCount;

    if (foundMainBase)
    {
        mainBaseCount      = (totalTeamCount * 2) / 3;
        secondaryBaseCount = totalTeamCount - mainBaseCount;
    }
    else
    {
        mainBaseCount      = 0;
        secondaryBaseCount = totalTeamCount;
    }

    for (int i = 0; i < mainBaseCount; i++)
    {
        const UnitClass uClass = kTeamSpawnClasses[i % 5];

        const f32::Fix angleRad = (f32::Fix(i) * (f32::pi * f32::Fix(2))) / f32::Fix(std::max(1, mainBaseCount)) +
                                  f32::Fix(35) / 100;
        const f32::Fix dist     = f32::Fix(128) + f32::Fix(gameState->mUpdateRandom.randIntRange(-6, 12));

        f32::Vec2 spawnPos;
        spawnPos.x = mainBasePos.x + f32::cos(angleRad) * dist;
        spawnPos.y = mainBasePos.y + f32::sin(angleRad) * dist;

        spawnAgent(gameState, spawnPos, uClass, true, teamId);
    }

    if (secondaryBaseCount > 0 && !basePositions.empty())
    {
        std::vector<f32::Vec2> secondaryBasePositions;
        for (const auto& pos : basePositions)
        {
            if (foundMainBase)
            {
                f32::Fix dist = f32::hypot(pos.x - mainBasePos.x, pos.y - mainBasePos.y);
                if (dist >= f32::Fix(100))
                {
                    secondaryBasePositions.push_back(pos);
                }
            }
            else
            {
                secondaryBasePositions.push_back(pos);
            }
        }

        if (secondaryBasePositions.empty())
        {
            secondaryBasePositions = basePositions;
        }

        for (int i = 0; i < secondaryBaseCount; i++)
        {
            const int       spawnIndex = i % secondaryBasePositions.size();
            const UnitClass uClass     = kTeamSpawnClasses[i % 5];
            const f32::Vec2 baseCenter = secondaryBasePositions[spawnIndex];

            const f32::Fix angleRad = (f32::Fix(i) * (f32::pi * f32::Fix(2))) / f32::Fix(std::max(1, secondaryBaseCount)) +
                                      f32::Fix(25) / 100;
            const f32::Fix dist = f32::Fix(95) + f32::Fix(gameState->mUpdateRandom.randIntRange(-5, 10));

            f32::Vec2 spawnPos;
            spawnPos.x = baseCenter.x + f32::cos(angleRad) * dist;
            spawnPos.y = baseCenter.y + f32::sin(angleRad) * dist;

            spawnAgent(gameState, spawnPos, uClass, false, teamId);
        }
    }
}

void replenishBaloSupplies(GameState* gameState, int waveIndex)
{
    if (!gameState || waveIndex < 2)
        return;

    auto&     c   = gameState->campaign();
    const int lvl = std::clamp(c.currentLevel, 1, 3);

    int defWallAdd  = 0;
    int gunWallAdd  = 0;
    int eleWallAdd  = 0;
    int fireWallAdd = 0;
    int radarAdd    = 0;
    int pointAdd    = 0;

    int warriorAdd = 0;
    int archerAdd  = 0;
    int tankAdd    = 0;
    int gruntAdd   = 0;
    int shooterAdd = 0;
    int berserkAdd = 0;

    switch (waveIndex)
    {
        case 2:
            defWallAdd = 2;
            gunWallAdd = 1;
            warriorAdd = 2;
            archerAdd  = 1;
            gruntAdd   = 1;
            pointAdd   = 1;
            break;
        case 3:
            defWallAdd = 2;
            gunWallAdd = 1;
            eleWallAdd = 1;
            warriorAdd = 2;
            archerAdd  = 2;
            gruntAdd   = 1;
            break;
        case 4:
            defWallAdd  = 2;
            gunWallAdd  = 1;
            fireWallAdd = 1;
            warriorAdd  = 2;
            archerAdd   = 2;
            tankAdd     = 1;
            shooterAdd  = 1;
            pointAdd    = 1;
            break;
        case 5:
            defWallAdd = 3;
            gunWallAdd = 2;
            eleWallAdd = 1;
            radarAdd   = 1;
            warriorAdd = 3;
            archerAdd  = 2;
            tankAdd    = 1;
            berserkAdd = 1;
            break;
        case 6:
            defWallAdd  = 3;
            gunWallAdd  = 2;
            fireWallAdd = 1;
            radarAdd    = 1;
            warriorAdd  = 3;
            archerAdd   = 3;
            tankAdd     = 1;
            shooterAdd  = 2;
            pointAdd    = 1;
            break;
        case 7:
            defWallAdd  = 3;
            gunWallAdd  = 2;
            eleWallAdd  = 2;
            fireWallAdd = 1;
            warriorAdd  = 4;
            archerAdd   = 3;
            tankAdd     = 2;
            berserkAdd  = 1;
            gruntAdd    = 2;
            break;
        case 8:
            defWallAdd = 4;
            gunWallAdd = 3;
            eleWallAdd = 2;
            radarAdd   = 1;
            warriorAdd = 4;
            archerAdd  = 4;
            tankAdd    = 2;
            shooterAdd = 2;
            berserkAdd = 2;
            pointAdd   = 1;
            break;
        case 9:
            defWallAdd  = 4;
            gunWallAdd  = 3;
            fireWallAdd = 2;
            radarAdd    = 2;
            warriorAdd  = 5;
            archerAdd   = 4;
            tankAdd     = 3;
            shooterAdd  = 2;
            berserkAdd  = 2;
            break;
        case 10:
        default:
            defWallAdd  = 5;
            gunWallAdd  = 4;
            eleWallAdd  = 2;
            fireWallAdd = 2;
            radarAdd    = 2;
            pointAdd    = 2;
            warriorAdd  = 6;
            archerAdd   = 5;
            tankAdd     = 3;
            shooterAdd  = 3;
            berserkAdd  = 3;
            break;
    }

    if (lvl >= 2)
    {
        defWallAdd += 1;
        gunWallAdd += 1;
        warriorAdd += 1;
        archerAdd += 1;
    }
    if (lvl >= 3)
    {
        defWallAdd += 1;
        eleWallAdd += 1;
        tankAdd += 1;
    }

    c.defensiveWallStock += defWallAdd;
    c.gunWallStock += gunWallAdd;
    c.electricWallStock += eleWallAdd;
    c.fireWallStock += fireWallAdd;
    c.radarWallStock += radarAdd;
    c.designatedPointStock += pointAdd;

    c.warriorUnitStock += warriorAdd;
    c.archerUnitStock += archerAdd;
    c.tankUnitStock += tankAdd;
    c.gruntUnitStock += gruntAdd;
    c.shooterUnitStock += shooterAdd;
    c.berserkerUnitStock += berserkAdd;

    for (auto& ts : c.teamStocks)
    {
        ts.defensiveWallStock += defWallAdd;
        ts.gunWallStock += gunWallAdd;
        ts.electricWallStock += eleWallAdd;
        ts.fireWallStock += fireWallAdd;
        ts.radarWallStock += radarAdd;
        ts.designatedPointStock += pointAdd;

        ts.warriorUnitStock += warriorAdd;
        ts.archerUnitStock += archerAdd;
        ts.tankUnitStock += tankAdd;
        ts.gruntUnitStock += gruntAdd;
        ts.shooterUnitStock += shooterAdd;
        ts.berserkerUnitStock += berserkAdd;
    }
}

void spawnWave(GameState* gameState, int waveIndex)
{
    const int   lvl        = std::clamp(gameState->campaign().currentLevel, 1, 3);
    const auto& lvlWaveCfg = GameConfig::getLevelWaveConfig(lvl);
    const int   totalWaves = static_cast<int>(lvlWaveCfg.waves.size());
    if (waveIndex < 1 || waveIndex > totalWaves)
        return;

    const auto& wCfg = lvlWaveCfg.waves[waveIndex - 1];

    std::vector<UnitClass> toSpawn;
    toSpawn.reserve(wCfg.gruntCount + wCfg.shooterCount + wCfg.tankCount + wCfg.berserkerCount);
    for (int i = 0; i < wCfg.gruntCount; i++)
        toSpawn.push_back(UnitClass::Grunt);
    for (int i = 0; i < wCfg.shooterCount; i++)
        toSpawn.push_back(UnitClass::Shooter);
    for (int i = 0; i < wCfg.tankCount; i++)
        toSpawn.push_back(UnitClass::Tank);
    for (int i = 0; i < wCfg.berserkerCount; i++)
        toSpawn.push_back(UnitClass::Berserker);

    const auto& spawnPositions = kTeam1SpawnPositions;
    for (size_t i = 0; i < toSpawn.size(); i++)
    {
        const int       spawnIndex = gameState->mUpdateRandom.randInt(spawnPositions.size());
        const f32::Vec2 baseSpawn  = spawnPositions[spawnIndex];
        const f32::Vec2 jitterPos  = baseSpawn + f32::Vec2(f32::Fix(gameState->mUpdateRandom.randIntRange(-35, 35)),
                                                           f32::Fix(gameState->mUpdateRandom.randIntRange(-35, 35)));
        spawnAgent(gameState, jitterPos, toSpawn[i], false, 1);
    }

    int reinfCount = 0;
    if (waveIndex == 2)
        reinfCount = 1;
    else if (waveIndex == 3)
        reinfCount = 2;
    else if (waveIndex == 4)
        reinfCount = 2;
    else if (waveIndex == 5)
        reinfCount = 3;
    else if (waveIndex == 6)
        reinfCount = 4;
    else if (waveIndex == 7)
        reinfCount = 5;
    else if (waveIndex == 8)
        reinfCount = 6;
    else if (waveIndex == 9)
        reinfCount = 7;
    else if (waveIndex >= 10)
        reinfCount = 9;

    if (lvl > 1 && waveIndex >= 6)
    {
        reinfCount += (lvl - 1);
    }

    int currentTeam0Count = 0;
    for (int id : gameState->mAdmin.mPool.viewIndex<CUnit>())
    {
        if (getEntityTeam(gameState->mAdmin.mPool, id) == 0)
            currentTeam0Count++;
    }
    if (currentTeam0Count < 3 && reinfCount < 2)
    {
        reinfCount = 2;
    }

    if (reinfCount > 0)
    {
        queueTeamPopulation(gameState, reinfCount, 0);
        gameState->playSound("SndBuild",
                             f32::Vec2(gameState->mapWidth() / f32::Fix(2), gameState->mapHeight() / f32::Fix(2)),
                             f32::Fix(1));
    }

    replenishBaloSupplies(gameState, waveIndex);

    gameState->playSound("SndRadar",
                         f32::Vec2(gameState->mapWidth() / f32::Fix(2), gameState->mapHeight() / f32::Fix(2)),
                         f32::Fix(1));
}

void queueLevelPopulation(GameState* gameState)
{
    queueTeamPopulation(gameState, -1, 0);
}

void queueLevelReset(GameState* gameState, int nextLevel)
{
    applyLevelConfig(gameState, nextLevel);
    gameState->campaign().victory           = false;
    gameState->campaign().gameOver          = false;
    gameState->campaign().pendingLevelReset = 0;

    auto& c                = gameState->campaign();
    c.currentWave          = 0;
    c.allWavesCompleted    = false;
    const auto& lvlWaveCfg = GameConfig::getLevelWaveConfig(c.currentLevel);
    c.maxWaves             = GameConfig::getCoopMaxWaves(c.difficulty);
    c.waveTimerTicks       = lvlWaveCfg.waves[0].delaySeconds * 60;
    c.waveTotalTimerTicks  = c.waveTimerTicks;

    gameState->campaign().unitDeathLimit += 2;

    queueLevelPopulation(gameState);

    gameState->nav().requestRecompute();
}

void requestLevelReset(GameState* gameState, int level)
{
    gameState->campaign().pendingLevelReset = level;
}

void updatePickupsAndProgress(GameState* gameState)
{
    const auto& collision = gameState->collision();
    auto&       pool      = gameState->mAdmin.mPool;

    for (const int pickupId : pool.viewIndex<CPickup>())
    {
        const auto* pickupMask = pool.getMask(pickupId);
        if (!isEntityAlive(gameState, pickupId) || !pool.hasComponent<CPosition>(pickupMask))
        {
            continue;
        }

        auto pickup    = pool.getComponent<CPickup>(pickupId);
        auto pickupPos = pool.getComponent<CPosition>(pickupId);
        pickup->lifeTime--;
        if (pickup->lifeTime <= 0)
        {
            gameState->mAdmin.remove(pickupId);
            continue;
        }

        bool collected = false;

        int chunkX = static_cast<int>(pickupPos->pos.x / collision.spacing);
        int chunkY = static_cast<int>(pickupPos->pos.y / collision.spacing);

        const auto& agentIdInChunks = collision.map.get(chunkY, chunkX).getEntityInCell();

        for (const int collectorId : agentIdInChunks)
        {
            const auto* collectorMask = pool.getMask(collectorId);
            const int   collectorTeam = getEntityTeam(pool, collectorId);
            if (gameState->getGameMode() == GameMode::Coop && collectorTeam != 0)
                continue;

            if (!isEntityAlive(gameState, collectorId) || !pool.hasComponent<CPosition>(collectorMask))
            {
                continue;
            }

            const auto collectorPos = pool.getComponent<CPosition>(collectorId);
            if (f32::length(collectorPos->pos - pickupPos->pos) > pickup->radius)
            {
                continue;
            }

            auto* effect            = gameState->alloc<CVisualEffect>();
            effect->kind            = EffectKind::CollectPulse;
            effect->startPos        = collectorPos->pos;
            effect->endPos          = {};
            effect->lifeTime        = 15;
            effect->initialLifeTime = 15;
            effect->teamId          = collectorTeam;
            gameState->mAdmin.add(gameState->createBundle(effect));
            collected = true;

            if (pool.hasComponent<CUnit>(collectorMask))
            {
                auto unit = pool.getComponent<CUnit>(collectorId);

                switch (pickup->kind)
                {
                    case PowerupKind::HealthBoost:
                        if (pool.hasComponent<CHealth>(collectorMask))
                        {
                            auto h = pool.getComponent<CHealth>(collectorId);
                            h->maxHp += 2;
                            h->hp = h->maxHp;
                        }
                        break;
                    case PowerupKind::DamageBoost:
                        unit->buff.damageBonus += 1;
                        unit->damage += 1;
                        break;
                    case PowerupKind::SpeedBoost:
                        unit->buff.speedBonus += f32::Fix(18) / 100;
                        unit->moveSpeed += f32::Fix(18) / 100;
                        break;
                    case PowerupKind::AttackRateBoost:
                        unit->buff.cooldownBonus -= 4;
                        unit->attackCooldown = std::max(5, unit->attackCooldown - 4);
                        break;
                    case PowerupKind::ShieldBoost:
                        unit->buff.shieldHp += 3;
                        break;
                }
            }
            else if (pool.hasComponent<CPlayer>(collectorMask))
            {
                auto player = pool.getComponent<CPlayer>(collectorId);

                switch (pickup->kind)
                {
                    case PowerupKind::HealthBoost:
                        if (pool.hasComponent<CHealth>(collectorMask))
                        {
                            auto h = pool.getComponent<CHealth>(collectorId);
                            h->maxHp += 2;
                            h->hp = h->maxHp;
                        }
                        break;
                    case PowerupKind::DamageBoost:
                        player->buff.damageBonus += 1;
                        player->damage += 1;
                        break;
                    case PowerupKind::SpeedBoost:
                        player->buff.speedBonus += f32::Fix(18) / 100;
                        player->moveSpeed += f32::Fix(18) / 100;
                        break;
                    case PowerupKind::AttackRateBoost:
                        player->buff.cooldownBonus -= 4;
                        player->attackCooldown = std::max(5, player->attackCooldown - 4);
                        break;
                    case PowerupKind::ShieldBoost:
                        player->buff.shieldHp += 3;
                        break;
                }
            }

            break;
        }

        if (!collected)
        {
            continue;
        }

        gameState->campaign().collectedSupplies += pickup->value;
        gameState->campaign().totalSupplies += pickup->value;
        gameState->mAdmin.remove(pickupId);
        gameState->playSound("SndPickup", pickupPos->pos, f32::Fix(85) / 100);
    }
}

void spawnPlayer(GameState* gameState)
{
    f32::Vec2 spawnPos          = {gameState->mapWidth() / f32::Fix(2), gameState->mapHeight() / f32::Fix(2)};
    bool      foundMainBase     = false;
    int       fallbackSubBaseId = -1;

    auto& pool = gameState->mAdmin.mPool;
    for (int baseId : pool.viewIndex<CBase>())
    {
        const auto* baseMask = pool.getMask(baseId);
        if (pool.hasComponent<CRectDragDrop>(baseMask))
        {
            const auto rect = pool.getComponent<CRectDragDrop>(baseId);
            if (rect->size.x > f32::Fix(120))
            {
                if (pool.hasComponent<CPosition>(baseMask))
                {
                    spawnPos      = pool.getComponent<CPosition>(baseId)->pos + f32::Vec2(f32::Fix(0), f32::Fix(100));
                    foundMainBase = true;
                    break;
                }
            }
            else if (fallbackSubBaseId == -1 && pool.hasComponent<CPosition>(baseMask))
            {
                fallbackSubBaseId = baseId;
            }
        }
    }

    if (!foundMainBase && fallbackSubBaseId != -1)
    {
        spawnPos = pool.getComponent<CPosition>(fallbackSubBaseId)->pos + f32::Vec2(f32::Fix(0), f32::Fix(80));
    }

    int count = std::max(1, gameState->mPlayerCount);
    for (int i = 0; i < count; ++i)
    {
        f32::Vec2 pos = spawnPos;
        if (gameState->mGameMode == GameMode::PvP)
        {
            bool foundBase   = false;
            int  fallbackSub = -1;
            for (const int baseId : pool.viewIndex<CBase>())
            {
                if (getEntityTeam(pool, baseId) == i)
                {
                    const auto* baseMask = pool.getMask(baseId);
                    if (pool.hasComponent<CRectDragDrop>(baseMask) && pool.hasComponent<CPosition>(baseMask))
                    {
                        const auto rect = pool.getComponent<CRectDragDrop>(baseId);
                        if (rect->size.x > f32::Fix(120))
                        {
                            pos = pool.getComponent<CPosition>(baseId)->pos + f32::Vec2(f32::Fix(0), f32::Fix(100));
                            foundBase = true;
                            break;
                        }
                        else if (fallbackSub == -1)
                        {
                            fallbackSub = baseId;
                        }
                    }
                }
            }
            if (!foundBase && fallbackSub != -1)
            {
                pos       = pool.getComponent<CPosition>(fallbackSub)->pos + f32::Vec2(f32::Fix(0), f32::Fix(80));
                foundBase = true;
            }
            if (!foundBase)
            {
                if (i % 2 == 1)
                {
                    pos.x = gameState->mapWidth() - pos.x;
                }
                if (i >= 2)
                {
                    pos.y = gameState->mapHeight() - pos.y;
                }
            }
        }
        else if (count > 1)
        {
            pos.x += (f32::Fix(i) - f32::Fix(count - 1) * f32::Fix(1) / 2) * f32::Fix(45);
        }

        auto* position = gameState->alloc<CPosition>();
        auto* velocity = gameState->alloc<CVelocity>();
        auto* bound    = gameState->alloc<CBoundCircle>();
        auto* health   = gameState->alloc<CHealth>();
        auto* team     = gameState->alloc<CTeam>();
        auto* player   = gameState->alloc<CPlayer>();

        team->playerId = i;
        team->teamId   = (gameState->mGameMode == GameState::GameMode::PvP) ? i : 0;

        player->skill1Cooldown = 12;
        player->skill1Timer    = 0;
        player->skill2Cooldown = 60;
        player->skill2Timer    = 0;
        player->skill3Cooldown = 18;
        player->skill3Timer    = 0;
        player->attackCooldown = 15;
        player->attackTimer    = 0;
        player->showRangeTimer = 0;
        player->attackRange    = 200;
        player->moveSpeed      = f32::Fix(3) / 2;
        player->damage         = 2;

        bound->radius = 12;

        health->maxHp = 35;
        health->hp    = health->maxHp;
        health->armor = 5;

        position->posInit = pos;
        position->pos     = pos;
        position->prevPos = pos;

        gameState->mAdmin.add(gameState->createBundle(player, bound, position, velocity, health, team));
    }
}

void spawnClickIndicator(GameState* gameState, f32::Vec2 pos, int style, int lifeTime, int teamId)
{
    auto* position    = gameState->alloc<CPosition>();
    position->pos     = pos;
    position->posInit = pos;
    position->prevPos = pos;

    auto* effect            = gameState->alloc<CVisualEffect>();
    effect->kind            = EffectKind::ClickIndicator;
    effect->startPos        = pos;
    effect->endPos          = {f32::Fix(style), f32::Fix(0)};
    effect->lifeTime        = lifeTime;
    effect->initialLifeTime = lifeTime;
    effect->teamId          = teamId;

    gameState->mAdmin.add(gameState->createBundle(position, effect));
}

void printStateMismatch(uint64_t tick, const std::string& expected, const std::string& actual)
{
    logout("\n================================================================================", true);
    logout("[REPLAY DESYNC DETECTED AT TICK " + std::to_string(tick) + "]", true);
    logout("Stopping simulation immediately so you can inspect where it desynchronized!", true);
    logout("--------------------------------------------------------------------------------", true);

    auto splitSections = [](const std::string& str)
    {
        std::vector<std::string> sections;
        size_t                   start = 0;
        while (start < str.size())
        {
            size_t end = str.find('|', start);
            if (end == std::string::npos)
            {
                sections.push_back(str.substr(start));
                break;
            }
            sections.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        return sections;
    };

    auto expectedSecs = splitSections(expected);
    auto actualSecs   = splitSections(actual);

    size_t maxCount      = std::max(expectedSecs.size(), actualSecs.size());
    int    mismatchCount = 0;

    for (size_t i = 0; i < maxCount; i++)
    {
        const std::string& exp = (i < expectedSecs.size()) ? expectedSecs[i] : "<MISSING>";
        const std::string& act = (i < actualSecs.size()) ? actualSecs[i] : "<MISSING>";

        if (exp != act)
        {
            mismatchCount++;
            logout("[MISMATCH #" + std::to_string(mismatchCount) + "]", true);
            logout("  Expected: " + exp, true);
            logout("  Actual:   " + act, true);
        }
    }

    logout("================================================================================\n", true);
}

} // namespace GameStateDetail
