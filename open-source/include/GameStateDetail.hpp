#ifndef GAMESTATEDETAIL_HPP
#define GAMESTATEDETAIL_HPP

#include "ComponentBundle.hpp"
#include "Components.hpp"
#include "FloatFixed.hpp"
#include "GameConfig.hpp"

#include <vector>

class GameState;
struct MemoryPool;

namespace SystemGameState
{
struct SystemCollision;
}
enum class UnitClass;
enum class UnitStateKind;

namespace GameStateDetail
{
using namespace GameConfig::Visual;

using GameConfig::AI::kAcquireRange;
using GameConfig::AI::kCombatSteerWeight;
using GameConfig::AI::kImpactImpulse;
using GameConfig::AI::kLowHealthRatio;
using GameConfig::AI::kPickupValue;
using GameConfig::AI::kProjectileSpeed;
using GameConfig::AI::kPushDecay;
using GameConfig::AI::kRetreatDurationTick;
using GameConfig::AI::kRetreatMaxSpeed;
using GameConfig::AI::kRetreatSteerWeight;
using GameConfig::AI::kSearchMaxSpeed;
using GameConfig::AI::kSearchSteerWeight;
using GameConfig::AI::kSeparationWeight;

f32::Vec2   getBuildWallSize(WallKind kind);
CWall       createBuildWallData(WallKind kind);
const char* getWallKindLabel(WallKind kind);
const int*  getWallStock(const GameState* gameState, WallKind kind, int playerId = -1);
void        consumeWallStock(GameState* gameState, WallKind kind, int playerId = -1);
bool        canPlaceWall(GameState* gameState, const f32::Rect& candidateRect);
void        placePlayerWall(GameState* gameState, f32::Vec2 center, WallKind kind, int playerId = -1);
const int*  getUnitStock(const GameState* gameState, UnitClass unitClass, int playerId = -1);
void        consumeUnitStock(GameState* gameState, UnitClass unitClass, int playerId = -1);
f32::Fix    getUnitRadius(UnitClass unitClass);
bool        canPlaceUnit(GameState* gameState, f32::Vec2 center, UnitClass unitClass);
void        placePlayerUnit(GameState* gameState, f32::Vec2 center, UnitClass unitClass, int playerId = -1);

bool     isPointInsideRect(const f32::Vec2& point, const f32::Rect& rect);
bool     segmentsIntersect(const f32::Vec2& a, const f32::Vec2& b, const f32::Vec2& c, const f32::Vec2& d);
bool     lineIntersectsRect(const f32::Vec2& start, const f32::Vec2& end, const f32::Rect& rect);
f32::Fix squaredLength(const f32::Vec2& vector);
bool     hasLineOfSight(const GameState* gameState, const f32::Vec2& start, const f32::Vec2& end);
bool     hasLineOfSight(GameState* gameState, const f32::Vec2& start, const f32::Vec2& end);
bool     hasLineOfSight(GameState* gameState, int entityAId, int entityBId);

bool     isAlive(const GameState* gameState, int entityId);
bool     isEntityAlive(GameState* gameState, int entityId);
bool     isEntityAlive(GameState* gameState, int entityId, uint32_t expectedGen);
int      getEntityTeam(const MemoryPool& pool, int entityId);
int      getEntityPlayerId(const MemoryPool& pool, int entityId);
bool     isFriendlyToPlayer(MemoryPool& pool, int entityId, int playerId = -1);
bool     areOpposingTeams(MemoryPool& pool, int entityAId, int entityBId);
bool     areAlliedTeams(MemoryPool& pool, int entityAId, int entityBId);
bool     isPlayerWallOwner(const MemoryPool& pool, int wallId, int playerId);
f32::Fix getHealthRatio(const CHealth& health);
bool     isLowHealth(const CHealth& health);
bool     isCriticalHealth(const CHealth& health);

f32::Fix         getDistanceBetweenEntities(GameState* gameState, int entityAId, int entityBId);
int              findClosestOpponent(GameState* gameState, int entityId, f32::Fix maxRange);
std::vector<int> findAgentInRange(GameState* gameState, f32::Vec2 origin, f32::Fix range);
int              findClosestOpponentToPoint(GameState*       gameState,
                                            const f32::Vec2& origin,
                                            f32::Fix         maxRange,
                                            int              searchingEntityId = -1,
                                            int              searchingTeamId   = -1);
int              findClosestBase(GameState* gameState, int entityId, f32::Fix maxRange);
int              findDraggableEntityAtPosition(const GameState* gameState,
                                               f32::Fix         mouseX,
                                               f32::Fix         mouseY,
                                               bool             checkDesignatedPointOnly = false,
                                               int              localPlayerId            = -1);

CWall             createWallData(WallKind kind, bool playerPlaced = false);
IComponentBundle* createProjectileBundle(
    GameState*       gameState,
    int              ownerId,
    int              targetId,
    const f32::Vec2& startPosition,
    const f32::Vec2& direction,
    f32::Fix         damage);
IComponentBundle* createWallBundle(GameState*       gameState,
                                   const f32::Vec2& position,
                                   const f32::Vec2& size,
                                   WallKind         kind,
                                   bool             playerPlaced = false,
                                   int              teamId       = 0);
IComponentBundle* createPickupBundle(GameState* gameState, const f32::Vec2& position);
IComponentBundle* createEnvironmentZoneBundle(
    GameState*       gameState,
    const f32::Vec2& position,
    const f32::Vec2& size,
    ZoneKind         kind,
    int              level);

bool isPointInBush(const GameState* gameState, const f32::Vec2& pos);
bool isEntityInBush(const GameState* gameState, int entityId);
int  getBushIndexAt(const GameState* gameState, const f32::Vec2& pos);

void emitUnitEvent(GameState* gameState, int entityId, UnitEventKind event, int targetId = -1);

void updateChunkMembership(GameState* gameState, int entityId, const f32::Vec2& previousPosition, const f32::Vec2& currentPosition);
void applySteering(CVelocity& velocity, f32::Vec2 desiredDirection, f32::Fix maxSpeed, f32::Fix steerWeight);
void moveEntityWithObstacles(GameState* gameState, int entityId);

void applyWallDamage(GameState* gameState, int targetId, f32::Fix damage, const f32::Vec2& hitOrigin, f32::Fix pushScale);
void fireProjectile(GameState* gameState, int entityId, int targetId, const CUnit& unit);

void resetUnitToSpawn(GameState* gameState, int entityId);
bool resetIfDead(GameState* gameState, int entityId);
void spawnAgent(GameState*       gameState,
                const f32::Vec2& spawnPosition,
                UnitClass        unitClass,
                bool             isSpawnAtMainBase = false,
                int              teamId            = 0);
void applyLevelConfig(GameState* gameState, int level);
void queueTeamPopulation(GameState* gameState, int count = -1, int teamId = 0);
void spawnWave(GameState* gameState, int waveIndex);
void replenishBaloSupplies(GameState* gameState, int waveIndex);
void queueLevelPopulation(GameState* gameState);
void queueLevelReset(GameState* gameState, int nextLevel);
void requestLevelReset(GameState* gameState, int level);
void updatePickupsAndProgress(GameState* gameState);
void spawnPlayer(GameState* gameState);

void printStateMismatch(uint64_t tick, const std::string& expected, const std::string& actual);

void spawnClickIndicator(GameState* gameState, f32::Vec2 pos, int style = 0, int lifeTime = 20, int teamId = 0);

} // namespace GameStateDetail

#endif
