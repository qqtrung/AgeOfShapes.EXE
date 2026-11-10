#include "SystemGameState.hpp"

#include "ComponentBundle.hpp"
#include "Components.hpp"
#include "Debug.hpp"
#include "DeterministicRandom.hpp"
#include "FloatFixed.hpp"
#include "GameConfig.hpp"
#include "GameState.hpp"
#include "GameStateDetail.hpp"
#include "Profiler.hpp"
#include "StateMachine.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>

namespace SystemGameState
{
void SystemMovement::update()
{
    auto& pool = context->mAdmin.mPool;

    for (int entityId : pool.viewIndex<CVelocity>())
    {
        const auto* mask = pool.getMask(entityId);
        if (!pool.hasComponent<CPosition>(mask) || !GameStateDetail::isEntityAlive(context, entityId))
        {
            continue;
        }

        if (pool.hasComponent<CBoundCircle>(mask))
        {
            GameStateDetail::moveEntityWithObstacles(context, entityId);
        }
        else
        {
            auto position     = pool.getComponent<CPosition>(entityId);
            auto velocity     = pool.getComponent<CVelocity>(entityId);
            position->prevPos = position->pos;
            position->pos += velocity->vel;
        }
    }
}

void SystemBase::update()
{
    PROFILE_FUNCTION();
    auto& pool = context->mAdmin.mPool;

    for (const int baseId : pool.viewIndex<CBase>())
    {
        const auto* baseMask = pool.getMask(baseId);
        if (!GameStateDetail::isEntityAlive(context, baseId) || !pool.hasComponent<CRectDragDrop>(baseMask) ||
            !pool.hasComponent<CPosition>(baseMask))
        {
            continue;
        }

        auto base = pool.getComponent<CBase>(baseId);

        if (base->attackTimer > 0)
        {
            base->attackTimer--;
        }

        if (base->attackTimer > 0)
        {
            continue;
        }


        const auto baseCenter = pool.getComponent<CPosition>(baseId)->pos;

        auto candidateIds = GameStateDetail::findAgentInRange(context, baseCenter, base->defenseRange);
        bool hasTarget    = false;

        for (const int targetId : candidateIds)
        {
            const auto* targetMask = pool.getMask(targetId);
            if (!GameStateDetail::areOpposingTeams(pool, baseId, targetId))
                continue;

            if (!GameStateDetail::isEntityAlive(context, targetId) || !pool.hasComponent<CPosition>(targetMask))
                continue;

            if (pool.hasComponent<CUnit>(targetMask) && pool.getComponent<CUnit>(targetId)->state == UnitStateKind::Explode)
                continue;

            hasTarget                 = true;
            const auto targetPosition = pool.getComponent<CPosition>(targetId);

            {
                auto* electricEffect            = context->alloc<CVisualEffect>();
                electricEffect->kind            = EffectKind::ElectricBeam;
                electricEffect->startPos        = baseCenter;
                electricEffect->endPos          = targetPosition->pos;
                electricEffect->lifeTime        = 10;
                electricEffect->initialLifeTime = 10;
                electricEffect->teamId          = GameStateDetail::getEntityTeam(pool, baseId);
                context->mAdmin.add(context->createBundle(electricEffect));
            }

            GameStateDetail::applyWallDamage(context,
                                             targetId,
                                             base->damage,
                                             baseCenter,
                                             GameStateDetail::kImpactImpulse * f32::Fix(1) / 2);
        }

        if (hasTarget)
        {
            base->attackTimer = base->attackCooldown;

            auto& shootClock = context->shootClock;
            if (shootClock > 5)
            {
                shootClock = 0;
                context->playSound("SndShoot", baseCenter, f32::Fix(75) / 100);
            }
        }
    }
}

void SystemWall::update()
{
    PROFILE_FUNCTION();
    auto& pool = context->mAdmin.mPool;

    for (const int wallId : pool.viewIndex<CWall>())
    {
        const auto* wallMask = pool.getMask(wallId);
        if (!GameStateDetail::isEntityAlive(context, wallId) || !pool.hasComponent<CRectDragDrop>(wallMask))
        {
            continue;
        }

        auto wall = pool.getComponent<CWall>(wallId);
        if (wall->cooldownTick <= 0)
        {
            continue;
        }

        const auto wallRect   = pool.getComponent<CRectDragDrop>(wallId);
        const auto wallCenter = wallRect->pos + wallRect->size * f32::Fix(1) / 2;

        const int wallTeam = GameStateDetail::getEntityTeam(pool, wallId);
        if (wallTeam < 0)
        {
            continue;
        }

        const auto& visionGrid = (context->getGameMode() == GameMode::PvP) ? context->nav().getVisionGrid(wallTeam)
                                                                           : context->nav().mVisionGrid;

        const int spacing = context->nav().visionSpacing > 0 ? context->nav().visionSpacing : 16;
        const int x       = static_cast<int>(wallCenter.x / spacing);
        const int y       = static_cast<int>(wallCenter.y / spacing);

        if (!visionGrid.isValidCell(x, y) || visionGrid[x][y] == 0)
        {
            continue;
        }

        if (wall->kind == WallKind::Gun || wall->kind == WallKind::Electric)
        {
            const int targetId = GameStateDetail::findClosestOpponentToPoint(context, wallCenter, wall->effectRange, wallId);
            if (targetId != -1 && pool.hasComponent<CPosition>(pool.getMask(targetId)))
            {
                f32::Vec2 dir  = pool.getComponent<CPosition>(targetId)->pos - wallCenter;
                wall->gunAngle = f32::atan2(dir.y, dir.x) * f32::Fix(180) / f32::pi;
            }
        }
        else if (wall->kind == WallKind::Fire)
        {
            wall->spinAngle += f32::Fix(4);
            if (wall->spinAngle >= f32::Fix(360))
                wall->spinAngle -= f32::Fix(360);
        }
        else if (wall->kind == WallKind::Radar)
        {
            wall->spinAngle -= f32::Fix(2);
            if (wall->spinAngle < f32::Fix(0))
                wall->spinAngle += f32::Fix(360);
        }

        if (wall->cooldownTimer > 0)
        {
            wall->cooldownTimer--;
        }

        if (wall->cooldownTimer > 0)
        {
            continue;
        }

        switch (wall->kind)
        {
            case WallKind::Electric:
            {
                const int targetId = GameStateDetail::findClosestOpponentToPoint(context, wallCenter, wall->effectRange, wallId);
                if (targetId != -1 && pool.hasComponent<CPosition>(pool.getMask(targetId)))
                {
                    const f32::Vec2 targetPos = pool.getComponent<CPosition>(targetId)->pos;
                    auto&           elecClock = context->elecClock;
                    if (elecClock > 5)
                    {
                        elecClock = 0;
                        context->playSound("SndWallElectric", wallCenter, f32::Fix(65) / 100);
                    }
                    GameStateDetail::applyWallDamage(context,
                                                     targetId,
                                                     wall->damage,
                                                     wallCenter,
                                                     GameStateDetail::kImpactImpulse * f32::Fix(12) / 10);

                    auto* effect            = context->alloc<CVisualEffect>();
                    effect->kind            = EffectKind::ElectricBeam;
                    effect->startPos        = wallCenter;
                    effect->endPos          = targetPos;
                    effect->lifeTime        = 10;
                    effect->initialLifeTime = 10;
                    effect->teamId          = wallTeam;
                    context->mAdmin.add(context->createBundle(effect));
                }
                break;
            }

            case WallKind::Fire:
            {
                bool fired = false;

                auto agentIds = GameStateDetail::findAgentInRange(context, wallCenter, wall->effectRange);

                for (const int targetId : agentIds)
                {
                    const auto* targetMask = pool.getMask(targetId);
                    if (!GameStateDetail::areOpposingTeams(pool, wallId, targetId))
                        continue;

                    if (!GameStateDetail::isEntityAlive(context, targetId) || !pool.hasComponent<CPosition>(targetMask))
                    {
                        continue;
                    }

                    {
                        fired                     = true;
                        const f32::Vec2 targetPos = pool.getComponent<CPosition>(targetId)->pos;
                        GameStateDetail::applyWallDamage(context,
                                                         targetId,
                                                         wall->damage,
                                                         wallCenter,
                                                         GameStateDetail::kImpactImpulse * f32::Fix(35) / 100);

                        auto* effect            = context->alloc<CVisualEffect>();
                        effect->kind            = EffectKind::FireBeam;
                        effect->startPos        = wallCenter;
                        effect->endPos          = targetPos;
                        effect->lifeTime        = 15;
                        effect->initialLifeTime = 15;
                        effect->teamId          = wallTeam;
                        context->mAdmin.add(context->createBundle(effect));
                    }
                }

                if (fired)
                {
                    auto& fireClock = context->fireClock;
                    if (fireClock > 5)
                    {
                        fireClock = 0;
                        context->playSound("SndWallFire", wallCenter, f32::Fix(65) / 100);
                    }
                }
                break;
            }

            case WallKind::Gun:
            {
                const int targetId = GameStateDetail::findClosestOpponentToPoint(context, wallCenter, wall->effectRange, wallId);
                if (targetId != -1 && pool.hasComponent<CPosition>(pool.getMask(targetId)))
                {
                    f32::Vec2 dir = pool.getComponent<CPosition>(targetId)->pos - wallCenter;
                    context->mAdmin.add(
                        GameStateDetail::createProjectileBundle(context, wallId, targetId, wallCenter, dir, wall->damage));
                }
                break;
            }

            case WallKind::Radar:
            {
                bool healedAny = false;

                auto agentIds = GameStateDetail::findAgentInRange(context, wallCenter, wall->effectRange);

                for (const int friendlyId : agentIds)
                {
                    const auto* friendlyMask = pool.getMask(friendlyId);
                    if (!GameStateDetail::areAlliedTeams(pool, wallId, friendlyId))
                        continue;

                    if (!GameStateDetail::isEntityAlive(context, friendlyId) || !pool.hasComponent<CPosition>(friendlyMask))
                    {
                        continue;
                    }

                    if (pool.hasComponent<CHealth>(friendlyMask))
                    {
                        auto health = pool.getComponent<CHealth>(friendlyId);
                        if (health->hp < health->maxHp)
                        {
                            f32::Fix healAmount = wall->damage > 0 ? f32::Fix(wall->damage) : f32::Fix(2);
                            health->hp          = std::min(health->maxHp, health->hp + healAmount);
                            healedAny           = true;

                            auto* healEffect            = context->alloc<CVisualEffect>();
                            healEffect->kind            = EffectKind::Heal;
                            healEffect->startPos        = pool.getComponent<CPosition>(friendlyId)->pos;
                            healEffect->endPos          = wallCenter;
                            healEffect->lifeTime        = 24;
                            healEffect->initialLifeTime = 24;
                            const int wallTeam          = GameStateDetail::getEntityTeam(pool, wallId);
                            healEffect->teamId          = wallTeam;
                            context->mAdmin.add(context->createBundle(healEffect));
                        }
                    }
                }

                if (healedAny)
                {
                    auto& healSoundClock = context->healSoundClock;
                    if (healSoundClock > 20)
                    {
                        healSoundClock = 0;
                        context->playSound("SndPickup", wallCenter, f32::Fix(75) / 100);
                    }

                    wall->cooldownTimer = wall->cooldownTick;
                }
                else
                {
                    wall->cooldownTimer = 6;
                }
                continue;
            }

            case WallKind::Defensive:
            default:
                break;
        }

        wall->cooldownTimer = wall->cooldownTick;
    }
}

void SystemProjectile::update()
{
    PROFILE_FUNCTION();
    auto&       projectiles   = context->mAdmin.mPool.viewComponent<CProjectile>();
    const auto& projectileIds = context->mAdmin.mPool.viewIndex<CProjectile>();

    for (size_t i = 0; i < projectiles.size(); i++)
    {
        const int   projectileId = projectileIds[i];
        const auto* projMask     = context->mAdmin.mPool.getMask(projectileId);
        if (!GameStateDetail::isEntityAlive(context, projectileId) ||
            !context->mAdmin.mPool.hasComponent<CPosition>(projMask) ||
            !context->mAdmin.mPool.hasComponent<CVelocity>(projMask))
        {
            continue;
        }

        auto& pool       = context->mAdmin.mPool;
        auto  projectile = pool.getComponent<CProjectile>(projectileId);
        auto  position   = pool.getComponent<CPosition>(projectileId);
        auto  velocity   = pool.getComponent<CVelocity>(projectileId);

        projectile->lifeTime--;

        if (projectile->lifeTime <= 0 || position->pos.x < f32::Fix(0) || position->pos.x > context->mapWidth() ||
            position->pos.y < f32::Fix(0) || position->pos.y > context->mapHeight())
        {
            context->mAdmin.remove(projectileId);
            continue;
        }

        const auto* targetMask = pool.getMask(projectile->targetId);
        if (!GameStateDetail::isEntityAlive(context, projectile->targetId) ||
            !pool.hasComponent<CPosition>(targetMask) || !pool.hasComponent<CHealth>(targetMask) ||
            (pool.hasComponent<CUnit>(targetMask) &&
             pool.getComponent<CUnit>(projectile->targetId)->state == UnitStateKind::Explode))
        {
            context->mAdmin.remove(projectileId);
            continue;
        }

        auto      targetPosition = pool.getComponent<CPosition>(projectile->targetId);
        auto      targetHealth   = pool.getComponent<CHealth>(projectile->targetId);
        f32::Vec2 targetPoint    = targetPosition->pos;

        if (pool.hasComponent<CBase>(targetMask) && pool.hasComponent<CRectDragDrop>(targetMask))
        {
            const auto     rect   = pool.getComponent<CRectDragDrop>(projectile->targetId);
            const f32::Fix clampX = std::clamp(position->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
            const f32::Fix clampY = std::clamp(position->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
            targetPoint           = f32::Vec2{clampX, clampY};
        }
        else
        {
            f32::Vec2 toTarget = targetPoint - position->pos;
            if (GameStateDetail::squaredLength(toTarget) > f32::Fix(1) / 10000)
            {
                velocity->vel = f32::normalized(toTarget) * GameConfig::AI::kProjectileSpeed;
            }
        }

        f32::Fix distance = f32::length(targetPoint - position->pos);

        if (distance > projectile->hitRadius)
        {
            continue;
        }

        auto impactDirection = targetPosition->pos - position->pos;
        if (GameStateDetail::squaredLength(impactDirection) > f32::Fix(1) / 10000)
        {
            impactDirection = f32::normalized(impactDirection);
        }

        auto dmg = projectile->damage;
        if (pool.hasComponent<CUnit>(targetMask))
        {
            auto targetUnit = pool.getComponent<CUnit>(projectile->targetId);
            if (targetUnit->buff.shieldHp > 0)
            {
                f32::Fix block = std::min(targetUnit->buff.shieldHp, dmg);
                targetUnit->buff.shieldHp -= block;
                dmg -= block;
            }
        }
        else if (pool.hasComponent<CPlayer>(targetMask))
        {
            auto targetPlayer = pool.getComponent<CPlayer>(projectile->targetId);
            if (targetPlayer->buff.shieldHp > 0)
            {
                f32::Fix block = std::min(targetPlayer->buff.shieldHp, dmg);
                targetPlayer->buff.shieldHp -= block;
                dmg -= block;
            }
        }
        if (targetHealth->armor > 0)
        {
            f32::Fix armorBlock = std::min(targetHealth->armor, dmg);
            targetHealth->armor -= armorBlock;
            dmg -= armorBlock;
        }
        targetHealth->hp -= dmg;
        if (pool.hasComponent<CVelocity>(targetMask))
        {
            pool.getComponent<CVelocity>(projectile->targetId)->pushDirection += impactDirection * GameStateDetail::kImpactImpulse;
        }

        GameStateDetail::resetIfDead(context, projectile->targetId);
        context->mAdmin.remove(projectileId);
    }
}
void SystemInit::init()
{
    context->campaign().maxLevel = GameConfig::kLevelCount;
    GameStateDetail::applyLevelConfig(context, context->campaign().currentLevel);

    const int  mapIdx      = std::clamp(context->campaign().currentMap, 1, 3);
    const int  playerCount = std::max(1, context->mPlayerCount);
    const bool isPvP       = (context->getGameMode() == GameMode::PvP && playerCount >= 2);

    auto spawnBase =
        [&](f32::Vec2 pos,
            f32::Vec2 size,
            f32::Fix  hp,
            f32::Fix  armor,
            f32::Fix  attackRange,
            f32::Fix  defenseRange,
            int       attackCooldown,
            f32::Fix  damage,
            int       teamId)
    {
        auto* baseRect   = context->alloc<CRectDragDrop>();
        auto* baseData   = context->alloc<CBase>();
        auto* basePos    = context->alloc<CPosition>();
        auto* baseHealth = context->alloc<CHealth>();
        auto* baseTeam   = context->alloc<CTeam>();

        baseTeam->teamId   = teamId;
        baseTeam->playerId = -1;

        baseRect->size = size;
        baseRect->pos  = pos;

        basePos->pos     = baseRect->pos + baseRect->size * f32::Fix(1) / 2;
        basePos->posInit = basePos->pos;
        basePos->prevPos = basePos->pos;

        baseHealth->hp    = hp;
        baseHealth->maxHp = hp;
        baseHealth->armor = armor;

        baseData->attackRange    = attackRange;
        baseData->defenseRange   = defenseRange;
        baseData->attackCooldown = attackCooldown;
        baseData->damage         = damage;

        auto* baseVision   = context->alloc<CVision>();
        baseVision->radius = f32::Fix(260);

        auto* baseObstacle          = context->alloc<CObstacle>();
        baseObstacle->isNavObstacle = true;
        baseObstacle->blocksVision  = false;

        auto* baseObjective = context->alloc<CMatchObjective>();

        context->mAdmin.add(
            context->createBundle(baseRect, baseData, basePos, baseHealth, baseTeam, baseVision, baseObstacle, baseObjective));
    };

    if (!isPvP || playerCount <= 1)
    {
        for (const auto& wall : GameConfig::getLevelWalls(mapIdx))
        {
            context->mAdmin.add(
                GameStateDetail::createWallBundle(context, wall.pos, wall.size, wall.kind, false, wall.teamId));
        }

        for (const auto& zone : GameConfig::getLevelZones(mapIdx))
        {
            context->mAdmin.add(
                GameStateDetail::createEnvironmentZoneBundle(context, zone.pos, zone.size, zone.kind, mapIdx));
        }

        std::vector<f32::Vec2> subBasePositions;
        f32::Vec2              mainBasePosition;

        if (mapIdx == 1)
        {
            subBasePositions = {{f32::Fix(1560), f32::Fix(340)},
                                {f32::Fix(2080), f32::Fix(1340)},
                                {f32::Fix(820), f32::Fix(1340)}};
            mainBasePosition = {f32::Fix(1280), f32::Fix(1080)};
        }
        else if (mapIdx == 2)
        {
            subBasePositions = {{f32::Fix(2050), f32::Fix(280)},
                                {f32::Fix(2550), f32::Fix(1180)},
                                {f32::Fix(2050), f32::Fix(1720)}};
            mainBasePosition = {f32::Fix(450), f32::Fix(1180)};
        }
        else if (mapIdx == 3)
        {
            subBasePositions = {{f32::Fix(1240), f32::Fix(260)},
                                {f32::Fix(2480), f32::Fix(1180)},
                                {f32::Fix(520), f32::Fix(1180)}};
            mainBasePosition = {f32::Fix(1220), f32::Fix(1120)};
        }
        else
        {
            const f32::Fix mapCenterX     = context->mapWidth() / f32::Fix(2);
            const f32::Fix mapCenterY     = context->mapHeight() / f32::Fix(2);
            const f32::Fix triangleRadius = f32::Fix(400);
            subBasePositions              = {{mapCenterX - f32::Fix(50), mapCenterY - triangleRadius - f32::Fix(50)},
                                             {mapCenterX - triangleRadius * f32::Fix(866) / 1000 - f32::Fix(50),
                                              mapCenterY + triangleRadius * f32::Fix(1) / 2 - f32::Fix(50)},
                                             {mapCenterX + triangleRadius * f32::Fix(866) / 1000 - f32::Fix(50),
                                              mapCenterY + triangleRadius * f32::Fix(1) / 2 - f32::Fix(50)}};
            mainBasePosition              = {mapCenterX - f32::Fix(75), mapCenterY - f32::Fix(75)};
        }

        for (const auto& pos : subBasePositions)
        {
            spawnBase(pos,
                      {f32::Fix(100), f32::Fix(100)},
                      f32::Fix(200),
                      f32::Fix(5),
                      f32::Fix(100),
                      f32::Fix(200),
                      20,
                      f32::Fix(3) / 10,
                      0);
        }
        spawnBase(mainBasePosition,
                  {f32::Fix(150), f32::Fix(150)},
                  f32::Fix(400),
                  f32::Fix(10),
                  f32::Fix(120),
                  f32::Fix(200),
                  20,
                  f32::Fix(1) / 2,
                  0);

        context->mAdmin.update();

        GameStateDetail::queueTeamPopulation(context, -1, 0);
        GameStateDetail::spawnPlayer(context);

        auto& c                = context->campaign();
        c.currentWave          = 0;
        const auto& lvlWaveCfg = GameConfig::getLevelWaveConfig(c.currentLevel);
        c.maxWaves             = GameConfig::getCoopMaxWaves(c.difficulty);
        c.allWavesCompleted    = false;
        c.waveTimerTicks       = lvlWaveCfg.waves[0].delaySeconds * 60;
        c.waveTotalTimerTicks  = c.waveTimerTicks;
    }
    else
    {
        const auto& layout = (playerCount == 2) ? GameConfig::getPvP2PMapLayout(mapIdx)
                                                : GameConfig::getPvP3PMapLayout(mapIdx);

        for (const auto& base : layout.bases)
        {
            spawnBase(base.pos,
                      base.size,
                      base.hp,
                      base.armor,
                      base.attackRange,
                      base.defenseRange,
                      base.attackCooldown,
                      base.damage,
                      base.teamId);
        }

        for (const auto& wall : layout.walls)
        {
            context->mAdmin.add(
                GameStateDetail::createWallBundle(context, wall.pos, wall.size, wall.kind, false, wall.teamId));
        }

        for (const auto& zone : layout.zones)
        {
            context->mAdmin.add(
                GameStateDetail::createEnvironmentZoneBundle(context, zone.pos, zone.size, zone.kind, mapIdx));
        }

        for (const auto& pickupPos : layout.pickups)
        {
            context->mAdmin.add(GameStateDetail::createPickupBundle(context, pickupPos));
        }

        context->mAdmin.update();

        const int pvpBaseUnitCount = GameConfig::getPvPBaseUnitCount(context->campaign().difficulty);
        for (int i = 0; i < playerCount; ++i)
        {
            GameStateDetail::queueTeamPopulation(context, pvpBaseUnitCount, i);
        }
        GameStateDetail::spawnPlayer(context);

        auto& c                = context->campaign();
        c.defensiveWallStock   = 8;
        c.electricWallStock    = 4;
        c.fireWallStock        = 4;
        c.gunWallStock         = 4;
        c.radarWallStock       = 2;
        c.designatedPointStock = 3;
        c.warriorUnitStock     = 10;
        c.archerUnitStock      = 10;
        c.tankUnitStock        = 5;
        c.gruntUnitStock       = 0;
        c.shooterUnitStock     = 0;
        c.berserkerUnitStock   = 0;
        for (auto& ts : c.teamStocks)
        {
            ts.defensiveWallStock   = 8;
            ts.electricWallStock    = 4;
            ts.fireWallStock        = 4;
            ts.gunWallStock         = 4;
            ts.radarWallStock       = 2;
            ts.designatedPointStock = 3;
            ts.warriorUnitStock     = 10;
            ts.archerUnitStock      = 10;
            ts.tankUnitStock        = 5;
            ts.gruntUnitStock       = 0;
            ts.shooterUnitStock     = 0;
            ts.berserkerUnitStock   = 0;
        }

        c.waveTimerTicks      = 15 * 60;
        c.waveTotalTimerTicks = c.waveTimerTicks;
        c.currentWave         = 0;
        c.maxWaves            = 999;
        c.allWavesCompleted   = false;
    }

    context->mAdmin.update();
}

void SystemMatchEnd::exit()
{
}

void SystemMatchEnd::update()
{
    PROFILE_FUNCTION();
    if (!context)
        return;

    if (context->campaign().pendingLevelReset > 0)
    {
        GameStateDetail::queueLevelReset(context, context->campaign().pendingLevelReset);
    }

    auto& pool = context->mAdmin.mPool;

    std::map<int, int> teamBaseCounts;
    for (const int objId : pool.viewIndex<CMatchObjective>())
    {
        if (!GameStateDetail::isEntityAlive(context, objId))
            continue;
        int tId = GameStateDetail::getEntityTeam(pool, objId);
        teamBaseCounts[tId]++;
    }

    auto& c = context->campaign();
    if (context->getGameMode() == GameMode::Coop && !c.gameOver && !c.allWavesCompleted)
    {
        const auto& lvlWaveCfg = GameConfig::getLevelWaveConfig(c.currentLevel);

        if (c.waveTimerTicks > 0)
        {
            c.waveTimerTicks--;
        }

        if (c.waveTimerTicks <= 0)
        {
            if (c.currentWave < c.maxWaves)
            {
                c.currentWave++;
                GameStateDetail::spawnWave(context, c.currentWave);

                if (c.currentWave < c.maxWaves)
                {
                    const int nextDelay   = lvlWaveCfg.waves[c.currentWave].delaySeconds;
                    c.waveTimerTicks      = nextDelay * 60;
                    c.waveTotalTimerTicks = c.waveTimerTicks;
                }
                else
                {
                    c.waveTimerTicks      = lvlWaveCfg.finalSurvivalSeconds * 60;
                    c.waveTotalTimerTicks = c.waveTimerTicks;
                }
            }
            else
            {
                c.allWavesCompleted   = true;
                c.victory             = true;
                c.gameOver            = true;
                context->mWinningTeam = 0;
                return;
            }
        }
    }
    else if (context->getGameMode() == GameMode::PvP && !c.gameOver)
    {
        if (c.waveTimerTicks > 0)
        {
            c.waveTimerTicks--;
        }
        else
        {
            c.waveTimerTicks      = 15 * 60;
            c.waveTotalTimerTicks = c.waveTimerTicks;
            c.currentWave++;

            const int pvpBaseUnitCount = GameConfig::getPvPBaseUnitCount(c.difficulty);
            const int waveUnitCount    = std::min(pvpBaseUnitCount + 5, pvpBaseUnitCount + (c.currentWave - 1) / 2);
            const int teamCount        = std::max(2, context->mPlayerCount);

            for (int tId = 0; tId < teamCount; ++tId)
            {
                auto it = teamBaseCounts.find(tId);
                if (it != teamBaseCounts.end() && it->second > 0)
                {
                    GameStateDetail::queueTeamPopulation(context, waveUnitCount, tId);
                }
            }
            GameStateDetail::replenishBaloSupplies(context, std::min(10, c.currentWave + 1));
        }
    }

    if (context->getGameMode() == GameMode::PvP)
    {
        int activeTeamCount   = 0;
        int lastSurvivingTeam = -1;
        for (const auto& [tId, count] : teamBaseCounts)
        {
            if (count > 0)
            {
                activeTeamCount++;
                lastSurvivingTeam = tId;
            }
        }

        if (activeTeamCount <= 1 && !context->campaign().gameOver)
        {
            context->mWinningTeam        = (activeTeamCount == 1) ? lastSurvivingTeam : -1;
            context->campaign().victory  = (context->mWinningTeam == 0);
            context->campaign().gameOver = true;
            return;
        }
    }
    else
    {
        auto it0 = teamBaseCounts.find(0);
        if ((it0 == teamBaseCounts.end() || it0->second == 0) && !context->campaign().gameOver)
        {
            context->mWinningTeam        = -1;
            context->campaign().victory  = false;
            context->campaign().gameOver = true;
            return;
        }
    }
}

void SystemNavigationField::init()
{
    auto& nav                  = context->mNav;
    nav.ticksUntilUpdate       = 0;
    nav.visionTicksUntilUpdate = 0;
    nav.delayFrames            = 0;

    update();
    updateVisionGrid();
}

void SystemNavigationField::computeGrid()
{
    PROFILE_FUNCTION();
    auto&     nav = context->mNav;
    const int w   = static_cast<int>(context->mapWidth() / nav.spacing);
    const int h   = static_cast<int>(context->mapHeight() / nav.spacing);
    nav.mGrid.assign(w, h, 0);

    auto&       dragDrop    = context->mAdmin.mPool.viewComponent<CRectDragDrop>();
    const auto& dragDropIds = context->mAdmin.mPool.viewIndex<CRectDragDrop>();

    auto dragDropCount = dragDrop.size();

    nav.mCachedWalls.clear();
    nav.mCachedWalls.reserve(dragDropCount);

    const f32::Fix radius = 12;

    int* const gridData = nav.mGrid.mData.data();

    for (size_t i = 0; i < dragDropCount; i++)
    {
        int   id = dragDropIds[i];
        auto& r  = dragDrop[i];

        const auto* dragMask     = context->mAdmin.mPool.getMask(id);
        bool        isDesignated = false;
        if (context->mAdmin.mPool.hasComponent<CObstacle>(dragMask))
        {
            const auto obstacle = context->mAdmin.mPool.getComponent<CObstacle>(id);
            if (!obstacle->isNavObstacle)
            {
                isDesignated = true;
            }
        }

        int teamId = GameStateDetail::getEntityTeam(context->mAdmin.mPool, id);
        nav.mCachedWalls.push_back({id, f32::Rect(r.pos, r.size), isDesignated, teamId});

        if (isDesignated)
        {
            continue;
        }

        const int topleftCellX = static_cast<int>((r.pos.x - radius) / nav.spacing);
        const int topleftCellY = static_cast<int>((r.pos.y - radius) / nav.spacing);

        const int bottomRightCellX = static_cast<int>((r.pos.x + r.size.x + radius) / nav.spacing);
        const int bottomRightCellY = static_cast<int>((r.pos.y + r.size.y + radius) / nav.spacing);

        const f32::Fix radiusSq   = radius * radius;
        const f32::Fix spacingFix = nav.spacing;

        const int startX = std::max(0, topleftCellX);
        const int endX   = std::min(w - 1, bottomRightCellX);
        const int startY = std::max(0, topleftCellY);
        const int endY   = std::min(h - 1, bottomRightCellY);

        for (int x = startX; x <= endX; x++)
        {
            const int      colOffset   = x * h;
            const f32::Fix cellCenterX = (f32::Fix(x) + f32::Fix(1) / 2) * spacingFix;
            const f32::Fix clampX      = std::clamp(cellCenterX, r.pos.x, r.pos.x + r.size.x);
            const f32::Fix dx          = cellCenterX - clampX;
            const f32::Fix dxSq        = dx * dx;

            for (int y = startY; y <= endY; y++)
            {
                const f32::Fix cellCenterY = (f32::Fix(y) + f32::Fix(1) / 2) * spacingFix;
                const f32::Fix clampY      = std::clamp(cellCenterY, r.pos.y, r.pos.y + r.size.y);
                const f32::Fix dy          = cellCenterY - clampY;

                if (dxSq + dy * dy <= radiusSq)
                {
                    gridData[colOffset + y] = id;
                }
            }
        }
    }
}

void SystemNavigationField::updateVisionGridForTeam(int teamId, Grid<char>& targetVisionGrid)
{
    PROFILE_FUNCTION();
    constexpr f32::Fix kVisionRadiusBase = 260;
    constexpr f32::Fix kVisionRadiusUnit = 200;

    if (targetVisionGrid.width() == 0 || targetVisionGrid.height() == 0)
        targetVisionGrid = Grid<char>(static_cast<int>(context->mapWidth() / context->mNav.visionSpacing),
                                      static_cast<int>(context->mapHeight() / context->mNav.visionSpacing),
                                      0);

    for (char& cell : targetVisionGrid.mData)
    {
        if (cell == 2)
            cell = 1;
    }

    constexpr int kFracShift = 8;
    constexpr int kFixedBits = 18 - kFracShift;

    auto toScaled = [&](f32::Fix v) -> int64_t { return v.raw_value() >> kFracShift; };

    using LightSource = NavData::LightSource;

    auto& pool   = context->mAdmin.mPool;
    auto& lights = context->mNav.mScratchLights;
    lights.clear();
    lights.reserve(pool.viewIndex<CVision>().size());

    auto addLight = [&](f32::Vec2 pos, f32::Fix radius, int ignoreWallId = -1)
    {
        const int64_t rScaled      = toScaled(radius);
        const int64_t innerRScaled = toScaled(radius - f32::Fix(context->mNav.visionSpacing));
        lights.push_back({.pos                 = pos,
                          .radius              = radius,
                          .xScaled             = toScaled(pos.x),
                          .yScaled             = toScaled(pos.y),
                          .innerRadiusSqScaled = innerRScaled * innerRScaled,
                          .radiusSqScaled      = rScaled * rScaled,
                          .ignoreWallId        = ignoreWallId});
    };

    for (int entityId : pool.viewIndex<CVision>())
    {
        if (!GameStateDetail::isEntityAlive(context, entityId))
            continue;
        if (GameStateDetail::getEntityTeam(pool, entityId) != teamId)
            continue;

        const auto  vision = pool.getComponent<CVision>(entityId);
        const auto* mask   = pool.getMask(entityId);

        f32::Vec2 center{};
        int       ignoreWallId = -1;

        if (pool.hasComponent<CPosition>(mask))
        {
            center = pool.getComponent<CPosition>(entityId)->pos;
            if (pool.hasComponent<CRectDragDrop>(mask))
            {
                ignoreWallId = entityId;
            }
        }
        else if (pool.hasComponent<CRectDragDrop>(mask))
        {
            const auto rect = pool.getComponent<CRectDragDrop>(entityId);
            center          = rect->pos + rect->size * f32::Fix(1) / 2;
            ignoreWallId    = entityId;
        }
        else
        {
            continue;
        }

        addLight(center, vision->radius, ignoreWallId);
    }

    if (lights.empty())
        return;

    constexpr int kChunkSpacing = 128;
    const int     chunkCols     = static_cast<int>(context->mapWidth()) / kChunkSpacing + 1;
    const int     chunkRows     = static_cast<int>(context->mapHeight()) / kChunkSpacing + 1;

    const int gridW = targetVisionGrid.width();
    const int gridH = targetVisionGrid.height();

    int globalMinChunkCol = chunkCols;
    int globalMaxChunkCol = -1;
    int globalMinChunkRow = chunkRows;
    int globalMaxChunkRow = -1;

    using WallBox        = NavData::WallBox;
    auto& wallBoxes      = context->mNav.mScratchWallBoxes;
    auto& lightChunkGrid = context->mNav.mLightChunkGrid;
    {
        PROFILE_SCOPE("VisionGrid - Build Light Chunks");
        if (lightChunkGrid.width() != chunkCols || lightChunkGrid.height() != chunkRows)
        {
            lightChunkGrid.create(chunkCols, chunkRows);
        }
        else
        {
            for (auto& chunk : lightChunkGrid.mGrid.mData)
            {
                chunk.vecIdEntity.clear();
            }
        }

        for (int lightIdx = 0; lightIdx < static_cast<int>(lights.size()); ++lightIdx)
        {
            const auto& light  = lights[lightIdx];
            const int   minCol = std::max(0, static_cast<int>((light.pos.x - light.radius) / kChunkSpacing));
            const int maxCol = std::min(chunkCols - 1, static_cast<int>((light.pos.x + light.radius) / kChunkSpacing));
            const int minRow = std::max(0, static_cast<int>((light.pos.y - light.radius) / kChunkSpacing));
            const int maxRow = std::min(chunkRows - 1, static_cast<int>((light.pos.y + light.radius) / kChunkSpacing));

            for (int r = minRow; r <= maxRow; ++r)
            {
                for (int c = minCol; c <= maxCol; ++c)
                {
                    lightChunkGrid.insert(r, c, lightIdx);
                }
            }

            globalMinChunkCol = std::min(globalMinChunkCol, minCol);
            globalMaxChunkCol = std::max(globalMaxChunkCol, maxCol);
            globalMinChunkRow = std::min(globalMinChunkRow, minRow);
            globalMaxChunkRow = std::max(globalMaxChunkRow, maxRow);
        }

        const auto& dragRects = pool.viewComponent<CRectDragDrop>();
        const auto& dragIds   = pool.viewIndex<CRectDragDrop>();
        int         maxWallId = 0;
        for (int id : dragIds)
            if (id > maxWallId)
                maxWallId = id;

        if (static_cast<int>(wallBoxes.size()) < maxWallId + 1)
        {
            wallBoxes.resize(maxWallId + 1);
        }
        std::fill(wallBoxes.begin(), wallBoxes.begin() + (maxWallId + 1), WallBox{});

        for (size_t i = 0; i < dragIds.size(); ++i)
        {
            const int id = dragIds[i];
            if (!GameStateDetail::isEntityAlive(context, id))
                continue;

            const auto* mask = pool.getMask(id);
            if (!pool.hasComponent<CObstacle>(mask))
                continue;

            const auto obstacle = pool.getComponent<CObstacle>(id);
            if (!obstacle->blocksVision)
                continue;

            const auto& rect = dragRects[i];
            wallBoxes[id]    = {.x0    = toScaled(rect.pos.x),
                                .y0    = toScaled(rect.pos.y),
                                .x1    = toScaled(rect.pos.x + rect.size.x),
                                .y1    = toScaled(rect.pos.y + rect.size.y),
                                .valid = true};
        }
    }

    {
        PROFILE_SCOPE("VisionGrid - Scan Cells & LoS");
        const int   size       = context->mNav.spacing;
        const int   obstacleW  = context->mNav.mGrid.width();
        const int   obstacleH  = context->mNav.mGrid.height();
        const int*  gridData   = context->mNav.mGrid.mData.data();
        char*       visionData = targetVisionGrid.mData.data();
        const auto* wallData   = wallBoxes.data();
        const int   wallDataSz = static_cast<int>(wallBoxes.size());

        auto segmentHitsWall = [](int64_t sx, int64_t sy, int64_t ex, int64_t ey, const WallBox& w) -> bool
        {
            if (ex >= w.x0 && ex <= w.x1 && ey >= w.y0 && ey <= w.y1)
                return false;

            const int64_t segMinX = (sx < ex) ? sx : ex;
            const int64_t segMaxX = (sx > ex) ? sx : ex;
            if (segMaxX < w.x0 || segMinX > w.x1)
                return false;

            const int64_t segMinY = (sy < ey) ? sy : ey;
            const int64_t segMaxY = (sy > ey) ? sy : ey;
            if (segMaxY < w.y0 || segMinY > w.y1)
                return false;

            const int64_t dx  = ex - sx;
            const int64_t dy  = ey - sy;
            const int64_t vx0 = (w.x0 - sx) * dy;
            const int64_t vx1 = (w.x1 - sx) * dy;
            const int64_t vy0 = (w.y0 - sy) * dx;
            const int64_t vy1 = (w.y1 - sy) * dx;

            const int64_t c00 = vx0 - vy0;
            const int64_t c10 = vx1 - vy0;
            const int64_t c01 = vx0 - vy1;
            const int64_t c11 = vx1 - vy1;

            if (c00 > 0 && c10 > 0 && c01 > 0 && c11 > 0)
                return false;
            if (c00 < 0 && c10 < 0 && c01 < 0 && c11 < 0)
                return false;

            return true;
        };

        auto isqrt64 = [](int64_t n) -> int64_t
        {
            if (n <= 0)
                return 0;
            const uint64_t un = static_cast<uint64_t>(n);
            uint64_t       x  = 1ULL << ((std::bit_width(un) + 1) >> 1);
            for (;;)
            {
                const uint64_t y = (x + un / x) >> 1;
                if (y >= x)
                    return static_cast<int64_t>(x);
                x = y;
            }
        };

        auto traceFromLight =
            [&](int64_t  sx,
                int64_t  sy,
                int64_t  ex,
                int64_t  ey,
                int      ignoreWallId,
                int&     outBlockingWallId,
                int64_t* outHitX = nullptr,
                int64_t* outHitY = nullptr) -> bool
        {
            if (outHitX)
                *outHitX = ex;
            if (outHitY)
                *outHitY = ey;

            const int startC = static_cast<int>(sx >> (kFixedBits + 4));
            const int startR = static_cast<int>(sy >> (kFixedBits + 4));
            const int endC   = static_cast<int>(ex >> (kFixedBits + 4));
            const int endR   = static_cast<int>(ey >> (kFixedBits + 4));

            int lastCheckedId       = -1;
            int firstBlockingWallId = -1;

            const int dc = (endC >= startC) ? 1 : -1;
            const int dr = (endR >= startR) ? 1 : -1;

            auto stepCell = [&](int c, int r) -> bool
            {
                if (static_cast<unsigned>(c) >= static_cast<unsigned>(obstacleW) ||
                    static_cast<unsigned>(r) >= static_cast<unsigned>(obstacleH))
                {
                    return true;
                }

                const int idx      = c * obstacleH + r;
                const int entityID = gridData[idx];
                if (entityID > 0 && entityID != ignoreWallId)
                {
                    if (entityID != lastCheckedId)
                    {
                        lastCheckedId = entityID;
                        if (entityID < wallDataSz && wallData[entityID].valid)
                        {
                            if (segmentHitsWall(sx, sy, ex, ey, wallData[entityID]))
                            {
                                if (firstBlockingWallId == -1)
                                {
                                    firstBlockingWallId = entityID;
                                    outBlockingWallId   = entityID;
                                }
                                else if (entityID != firstBlockingWallId)
                                {
                                    const int64_t cx = static_cast<int64_t>((c << 4) + 8) << kFixedBits;
                                    const int64_t cy = static_cast<int64_t>((r << 4) + 8) << kFixedBits;
                                    if (segmentHitsWall(sx, sy, cx, cy, wallData[entityID]))
                                    {
                                        if (outHitX)
                                            *outHitX = cx;
                                        if (outHitY)
                                            *outHitY = cy;
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                if (firstBlockingWallId != -1)
                {
                    const WallBox& w  = wallData[firstBlockingWallId];
                    const int64_t  cx = static_cast<int64_t>((c << 4) + 8) << kFixedBits;
                    const int64_t  cy = static_cast<int64_t>((r << 4) + 8) << kFixedBits;
                    if (segmentHitsWall(sx, sy, cx, cy, w))
                    {
                        const int64_t cellLeft   = static_cast<int64_t>(c << 4) << kFixedBits;
                        const int64_t cellRight  = static_cast<int64_t>((c + 1) << 4) << kFixedBits;
                        const int64_t cellTop    = static_cast<int64_t>(r << 4) << kFixedBits;
                        const int64_t cellBottom = static_cast<int64_t>((r + 1) << 4) << kFixedBits;

                        if ((dc > 0 && cellLeft > w.x1) || (dc < 0 && cellRight < w.x0) || (dr > 0 && cellTop > w.y1) ||
                            (dr < 0 && cellBottom < w.y0))
                        {
                            if (outHitX)
                                *outHitX = cx;
                            if (outHitY)
                                *outHitY = cy;
                            return false;
                        }
                        visionData[idx] = 2;
                        return true;
                    }
                }

                visionData[idx] = 2;
                return true;
            };

            if (startR == endR)
            {
                for (int c = startC;; c += dc)
                {
                    if (!stepCell(c, startR))
                        return false;
                    if (c == endC)
                        break;
                }
                return firstBlockingWallId == -1;
            }

            if (startC == endC)
            {
                for (int r = startR;; r += dr)
                {
                    if (!stepCell(startC, r))
                        return false;
                    if (r == endR)
                        break;
                }
                return firstBlockingWallId == -1;
            }

            const int64_t dyTotal = ey - sy;
            const int64_t dxTotal = ex - sx;
            const int64_t slope   = (dxTotal << kFixedBits) / dyTotal;

            int64_t xa = sx;
            for (int r = startR;; r += dr)
            {
                int64_t xb;
                if (r == endR)
                {
                    xb = ex;
                }
                else
                {
                    const int     nextBoundaryRow = (dr > 0) ? (r + 1) : r;
                    const int64_t yBoundary       = static_cast<int64_t>(nextBoundaryRow * size) << kFixedBits;
                    xb                            = sx + (((yBoundary - sy) * slope) >> kFixedBits);
                }

                const int64_t xMin   = (xa < xb) ? xa : xb;
                const int64_t xMax   = (xa > xb) ? xa : xb;
                const int     cLeft  = static_cast<int>(xMin >> (kFixedBits + 4));
                const int     cRight = static_cast<int>(xMax >> (kFixedBits + 4));
                xa                   = xb;

                if (dc > 0)
                {
                    for (int c = cLeft; c <= cRight; ++c)
                    {
                        if (!stepCell(c, r))
                            return false;
                    }
                }
                else
                {
                    for (int c = cRight; c >= cLeft; --c)
                    {
                        if (!stepCell(c, r))
                            return false;
                    }
                }

                if (r == endR)
                    break;
            }

            return firstBlockingWallId == -1;
        };

        constexpr int kCellsPerChunk = kChunkSpacing / 16;

        for (int chunkCol = globalMinChunkCol; chunkCol <= globalMaxChunkCol; ++chunkCol)
        {
            const int gxStart = chunkCol * kCellsPerChunk;
            const int gxEnd   = std::min(gridW, gxStart + kCellsPerChunk);

            for (int chunkRow = globalMinChunkRow; chunkRow <= globalMaxChunkRow; ++chunkRow)
            {
                auto& candidateLights = lightChunkGrid.get(chunkRow, chunkCol).vecIdEntity;
                if (candidateLights.empty())
                {
                    continue;
                }

                const int gyStart = chunkRow * kCellsPerChunk;
                const int gyEnd   = std::min(gridH, gyStart + kCellsPerChunk);

                const int64_t chunkCenterX = static_cast<int64_t>((gxStart + gxEnd) << 3) << kFixedBits;
                const int64_t chunkCenterY = static_cast<int64_t>((gyStart + gyEnd) << 3) << kFixedBits;

                size_t  nearestIdx = 0;
                int64_t minDistSq  = -1;
                for (size_t i = 0; i < candidateLights.size(); ++i)
                {
                    const auto&   l  = lights[candidateLights[i]];
                    const int64_t dx = chunkCenterX - l.xScaled;
                    const int64_t dy = chunkCenterY - l.yScaled;
                    const int64_t d2 = dx * dx + dy * dy;
                    if (minDistSq < 0 || d2 < minDistSq)
                    {
                        minDistSq  = d2;
                        nearestIdx = i;
                    }
                }
                if (nearestIdx != 0)
                {
                    std::swap(candidateLights[0], candidateLights[nearestIdx]);
                }

                const auto& nearestLight = lights[candidateLights[0]];
                const int   colStep      = (chunkCenterX < nearestLight.xScaled) ? 1 : -1;
                const int   rowStep      = (chunkCenterY < nearestLight.yScaled) ? 1 : -1;

                const int firstGx = (colStep > 0) ? gxStart : (gxEnd - 1);
                const int lastGx  = (colStep > 0) ? gxEnd : (gxStart - 1);
                const int firstGy = (rowStep > 0) ? gyStart : (gyEnd - 1);
                const int lastGy  = (rowStep > 0) ? gyEnd : (gyStart - 1);

                for (int gx = firstGx; gx != lastGx; gx += colStep)
                {
                    const int64_t pxScaled           = static_cast<int64_t>((gx << 4) + 8) << kFixedBits;
                    const int     colBase            = gx * gridH;
                    int           lastBlockingWallId = -1;

                    for (int gy = firstGy; gy != lastGy; gy += rowStep)
                    {
                        const int cellIdx = colBase + gy;
                        if (visionData[cellIdx] == 2)
                        {
                            continue;
                        }

                        const int64_t pyScaled         = static_cast<int64_t>((gy << 4) + 8) << kFixedBits;
                        const bool    targetIsObstacle = (gridData[cellIdx] > 0);

                        int blockedWallIds[4] = {-1, -1, -1, -1};
                        int numBlockedWalls   = 0;
                        if (lastBlockingWallId > 0)
                        {
                            blockedWallIds[numBlockedWalls++] = lastBlockingWallId;
                        }

                        for (const int lightIdx : candidateLights)
                        {
                            const auto&   light  = lights[lightIdx];
                            const int64_t dx     = pxScaled - light.xScaled;
                            const int64_t dy     = pyScaled - light.yScaled;
                            const int64_t distSq = dx * dx + dy * dy;

                            if (distSq > light.radiusSqScaled)
                                continue;

                            bool isBlocked = false;
                            for (int bw = 0; bw < numBlockedWalls; ++bw)
                            {
                                const int wId = blockedWallIds[bw];
                                if (wId > 0 && wId != light.ignoreWallId &&
                                    segmentHitsWall(light.xScaled, light.yScaled, pxScaled, pyScaled, wallData[wId]))
                                {
                                    isBlocked = true;
                                    break;
                                }
                            }
                            if (isBlocked)
                                continue;

                            int64_t targetX = pxScaled;
                            int64_t targetY = pyScaled;
                            if (!targetIsObstacle && distSq <= light.innerRadiusSqScaled)
                            {
                                const int64_t dist    = isqrt64(distSq);
                                const int64_t rScaled = toScaled(light.radius);
                                if (dist > 0)
                                {
                                    targetX = light.xScaled + (dx * rScaled) / (dist + 1);
                                    targetY = light.yScaled + (dy * rScaled) / (dist + 1);
                                }
                            }

                            int hitWallId = -1;
                            (void)traceFromLight(light.xScaled, light.yScaled, targetX, targetY, light.ignoreWallId, hitWallId);

                            if (hitWallId > 0)
                            {
                                lastBlockingWallId = hitWallId;
                                bool alreadyIn     = false;
                                for (int bw = 0; bw < numBlockedWalls; ++bw)
                                {
                                    if (blockedWallIds[bw] == hitWallId)
                                    {
                                        alreadyIn = true;
                                        break;
                                    }
                                }
                                if (!alreadyIn && numBlockedWalls < 4)
                                {
                                    blockedWallIds[numBlockedWalls++] = hitWallId;
                                }
                            }

                            if (visionData[cellIdx] == 2)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void SystemNavigationField::ensureVisionGridAllocated(int maxTeamId)
{
    auto& nav = context->mNav;
    if (nav.mGrid.width() == 0 || nav.mGrid.height() == 0)
    {
        computeGrid();
    }

    if (nav.mVisionGrid.width() == 0 || nav.mVisionGrid.height() == 0)
    {
        nav.mVisionGrid = Grid<char>(static_cast<int>(context->mapWidth() / nav.visionSpacing),
                                     static_cast<int>(context->mapHeight() / nav.visionSpacing),
                                     0);
    }

    if (static_cast<int>(nav.mTeamVisionGrid.size()) <= maxTeamId)
    {
        nav.mTeamVisionGrid.resize(maxTeamId + 1);
    }

    for (size_t t = 0; t < nav.mTeamVisionGrid.size(); ++t)
    {
        if (nav.mTeamVisionGrid[t].width() == 0 || nav.mTeamVisionGrid[t].height() == 0)
        {
            nav.mTeamVisionGrid[t] = Grid<char>(static_cast<int>(context->mapWidth() / nav.visionSpacing),
                                                static_cast<int>(context->mapHeight() / nav.visionSpacing),
                                                0);
        }
    }
}

void SystemNavigationField::updateVisionGrid()
{
    PROFILE_FUNCTION();
    int   maxTeamId = 1;
    auto& pool      = context->mAdmin.mPool;
    for (int e : pool.viewIndex<CTeam>())
    {
        maxTeamId = std::max(maxTeamId, pool.getComponent<CTeam>(e)->teamId);
    }

    ensureVisionGridAllocated(maxTeamId);

    auto& nav = context->mNav;
    for (size_t t = 0; t < nav.mTeamVisionGrid.size(); ++t)
    {
        updateVisionGridForTeam(static_cast<int>(t), nav.mTeamVisionGrid[t]);
    }

    if (nav.mVisionGrid.width() != nav.mTeamVisionGrid[0].width() ||
        nav.mVisionGrid.height() != nav.mTeamVisionGrid[0].height())
    {
        nav.mVisionGrid = nav.mTeamVisionGrid[0];
    }
    else
    {
        std::memcpy(nav.mVisionGrid.mData.data(), nav.mTeamVisionGrid[0].mData.data(), nav.mTeamVisionGrid[0].mData.size());
    }
    nav.visionDirty = true;
    nav.visionVersion++;
}

void SystemNavigationField::updateTeamField(int teamId)
{
    PROFILE_SCOPE("Team FlowFields Compute");
    auto& pool = context->mAdmin.mPool;

    auto& targets = context->mNav.mScratchTargets;
    targets.clear();

    for (int objId : pool.viewIndex<CMatchObjective>())
    {
        if (GameStateDetail::getEntityTeam(pool, objId) != teamId && GameStateDetail::isEntityAlive(context, objId))
        {
            targets.push_back(objId);
        }
    }

    if (context->getGameMode() == GameState::GameMode::PvP || teamId == 0)
    {
        const auto& visionGrid = getVisionGrid(teamId);
        for (int opponentId : pool.viewIndex<CNavTarget>())
        {
            if (GameStateDetail::getEntityTeam(pool, opponentId) == teamId ||
                !GameStateDetail::isEntityAlive(context, opponentId))
            {
                continue;
            }

            const auto* opponentMask = pool.getMask(opponentId);
            if (!pool.hasComponent<CPosition>(opponentMask))
            {
                continue;
            }

            const auto position   = pool.getComponent<CPosition>(opponentId);
            const int  safeVision = context->mNav.visionSpacing > 0 ? context->mNav.visionSpacing : 16;
            const int  cellX      = static_cast<int>(position->pos.x / safeVision);
            const int  cellY      = static_cast<int>(position->pos.y / safeVision);

            if (visionGrid.isValidCell(cellX, cellY) && visionGrid[cellX][cellY] != 0)
            {
                targets.push_back(opponentId);
            }
        }
    }

    getTeamField(teamId).compute(context->mNav.spacing, context->mapWidth(), context->mapHeight(), pool, targets);
}

void SystemNavigationField::updateDesignatedFields()
{
    int   maxTeamId = 1;
    auto& pool      = context->mAdmin.mPool;
    for (int e : pool.viewIndex<CTeam>())
    {
        maxTeamId = std::max(maxTeamId, pool.getComponent<CTeam>(e)->teamId);
    }

    auto& dpTargets = context->mNav.mScratchDpTargets;
    for (int t = 0; t <= maxTeamId; ++t)
    {
        dpTargets.clear();
        for (int dpId : pool.viewIndex<CNavBeacon>())
        {
            if (!GameStateDetail::isEntityAlive(context, dpId))
                continue;
            const int dpTeam = GameStateDetail::getEntityTeam(pool, dpId);
            if (dpTeam == t || (dpTeam < 0 && t == 0))
            {
                dpTargets.push_back(dpId);
            }
        }
        getDesignatedField(t).compute(context->mNav.spacing, context->mapWidth(), context->mapHeight(), pool, dpTargets);
    }
}

void SystemNavigationField::updateTeamFields()
{
    int   maxTeamId = 1;
    auto& pool      = context->mAdmin.mPool;
    for (int e : pool.viewIndex<CTeam>())
    {
        maxTeamId = std::max(maxTeamId, pool.getComponent<CTeam>(e)->teamId);
    }

    for (int t = 0; t <= maxTeamId; ++t)
    {
        updateTeamField(t);
    }
}

void SystemNavigationField::update()
{
    PROFILE_FUNCTION();
    auto& nav = context->mNav;

    nav.visionTicksUntilUpdate--;
    if (nav.visionTicksUntilUpdate <= 0)
    {
        nav.visionTicksUntilUpdate = nav.visionUpdateIntervalTick;
        updateVisionGrid();
    }

    nav.ticksUntilUpdate--;

    if (nav.ticksUntilUpdate <= 0)
    {
        nav.ticksUntilUpdate = nav.updateIntervalTick;
    }

    const int baseTick = nav.updateIntervalTick / 2;
    if (nav.ticksUntilUpdate <= baseTick && nav.ticksUntilUpdate >= baseTick - 16)
    {
        int   maxTeamId = 1;
        auto& pool      = context->mAdmin.mPool;
        for (int e : pool.viewIndex<CTeam>())
        {
            maxTeamId = std::max(maxTeamId, pool.getComponent<CTeam>(e)->teamId);
        }

        const int teamOffset = baseTick - nav.ticksUntilUpdate;
        if (teamOffset <= maxTeamId)
        {
            updateTeamField(teamOffset);
        }
    }

    if (nav.delayFrames > 0)
    {
        nav.delayFrames--;
    }

    if (nav.delayFrames == 0)
    {
        nav.delayFrames = -1;

        computeGrid();
        updateVisionGrid();
        updateDesignatedFields();
        updateTeamFields();
    }
}

bool SystemNavigationField::hasLineOfSight(f32::Vec2 start, f32::Vec2 end, int ignoreWallId, bool allowEndInsideWall) const
{
    if (!context)
    {
        return true;
    }

    const auto& nav = context->mNav;
    if (nav.spacing <= 0 || nav.mGrid.width() == 0 || nav.mGrid.height() == 0)
    {
        return true;
    }

    const auto& pool = context->mAdmin.mPool;

    f32::Vec2 p1 = start;
    f32::Vec2 p2 = end;
    if (p1.y > p2.y)
    {
        std::swap(p1, p2);
    }

    const int size = nav.spacing;
    const int cbR  = static_cast<int>(p1.y) / size;
    const int cbC  = static_cast<int>(p1.x) / size;
    const int ceR  = static_cast<int>(p2.y) / size;
    const int ceC  = static_cast<int>(p2.x) / size;

    int lastCheckedId = -1;

    auto checkCell = [&](int c, int r) -> bool
    {
        if (nav.mGrid.isValidCell(c, r))
        {
            int entityID = nav.mGrid[c][r];
            if (entityID > 0 && entityID != lastCheckedId && entityID != ignoreWallId)
            {
                lastCheckedId    = entityID;
                const auto* mask = pool.getMask(entityID);
                if (pool.hasComponent<CObstacle>(mask))
                {
                    const auto obstacle = pool.getComponent<CObstacle>(entityID);
                    if (!obstacle->blocksLineOfSight)
                    {
                        return true;
                    }
                }

                if (pool.hasComponent<CRectDragDrop>(mask))
                {
                    const auto      wallRect = pool.getComponent<CRectDragDrop>(entityID);
                    const f32::Rect rect(wallRect->pos, wallRect->size);
                    if (allowEndInsideWall && GameStateDetail::isPointInsideRect(end, rect))
                    {
                        return true;
                    }
                    if (GameStateDetail::lineIntersectsRect(start, end, rect))
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    if (cbR == ceR)
    {
        int dcC = (ceC > cbC) ? 1 : ((ceC < cbC) ? -1 : 0);
        for (int j = cbC; j != ceC + dcC; j += (dcC == 0 ? 1 : dcC))
        {
            if (!checkCell(j, cbR))
            {
                return false;
            }
            if (dcC == 0)
            {
                break;
            }
        }
        return true;
    }

    if (cbC == ceC)
    {
        for (int i = cbR; i <= ceR; i++)
        {
            if (!checkCell(cbC, i))
            {
                return false;
            }
        }
        return true;
    }

    const f32::Fix dyTotal = p2.y - p1.y;
    const f32::Fix dxTotal = p2.x - p1.x;
    const f32::Fix slope   = dxTotal / dyTotal;

    f32::Fix xa = p1.x;
    for (int r = cbR; r <= ceR; r++)
    {
        const f32::Fix xb = (r == ceR) ? p2.x : (p1.x + (f32::Fix((r + 1) * size) - p1.y) * slope);

        const int cLeft  = static_cast<int>(std::min(xa, xb)) / size;
        const int cRight = static_cast<int>(std::max(xa, xb)) / size;
        xa               = xb;

        for (int c = cLeft; c <= cRight; c++)
        {
            if (!checkCell(c, r))
            {
                return false;
            }
        }
    }

    return true;
}

std::set<int> SystemNavigationField::getObstacleInSight(f32::Vec2 start, f32::Vec2 end) const
{
    if (start.y > end.y)
        std::swap(start, end);

    const auto& nav  = context->mNav;
    const int   size = nav.spacing;
    const int   cbR  = static_cast<int>(start.y) / size;
    const int   cbC  = static_cast<int>(start.x) / size;
    const int   ceR  = static_cast<int>(end.y) / size;
    const int   ceC  = static_cast<int>(end.x) / size;

    std::set<int> obstacleIds;

    auto collectCell = [&](int c, int r)
    {
        if (nav.mGrid.isValidCell(c, r) && nav.mGrid[c][r] > 0)
        {
            int         entityID = nav.mGrid[c][r];
            const auto* mask     = context->mAdmin.mPool.getMask(entityID);
            if (context->mAdmin.mPool.hasComponent<CRectDragDrop>(mask))
            {
                obstacleIds.insert(entityID);
            }
        }
    };

    if (cbR == ceR)
    {
        int dcC = (ceC > cbC) ? 1 : ((ceC < cbC) ? -1 : 0);
        for (int j = cbC; j != ceC + dcC; j += (dcC == 0 ? 1 : dcC))
        {
            collectCell(j, cbR);
            if (dcC == 0)
                break;
        }
        return obstacleIds;
    }

    if (cbC == ceC)
    {
        for (int i = cbR; i <= ceR; i++)
        {
            collectCell(cbC, i);
        }
        return obstacleIds;
    }

    f32::Fix dyTotal = end.y - start.y;
    f32::Fix dxTotal = end.x - start.x;

    for (int r = cbR; r <= ceR; r++)
    {
        f32::Fix yTop    = r * size;
        f32::Fix yBottom = (r + 1) * size;

        f32::Fix ya = std::max(yTop, start.y);
        f32::Fix yb = std::min(yBottom, end.y);

        f32::Fix xa = start.x + (ya - start.y) / dyTotal * dxTotal;
        f32::Fix xb = start.x + (yb - start.y) / dyTotal * dxTotal;

        int cLeft  = static_cast<int>(std::min(xa, xb) / size);
        int cRight = static_cast<int>(std::max(xa, xb) / size);

        for (int c = cLeft; c <= cRight; c++)
        {
            collectCell(c, r);
        }
    }

    return obstacleIds;
}

const Grid<char>& SystemNavigationField::getVisionGrid(int teamId) const
{
    return context->mNav.getVisionGrid(teamId);
}

VectorField& SystemNavigationField::getDesignatedField(int teamId)
{
    return context->mNav.getDesignatedField(teamId);
}

const VectorField& SystemNavigationField::getDesignatedField(int teamId) const
{
    return context->mNav.getDesignatedField(teamId);
}

VectorField& SystemNavigationField::getTeamField(int teamId)
{
    return context->mNav.getTeamField(teamId);
}

const VectorField& SystemNavigationField::getTeamField(int teamId) const
{
    return context->mNav.getTeamField(teamId);
}

void SystemNavigationField::requestRecompute()
{
    if (context)
    {
        context->mNav.requestRecompute();
    }
}

void SystemCollision::init()
{
    auto&     col  = context->mCollision;
    const int cols = static_cast<int>(context->mapWidth() / col.spacing) + 1;
    const int rows = static_cast<int>(context->mapHeight() / col.spacing) + 1;
    col.map.create(cols, rows);

    auto& pool = context->mAdmin.mPool;
    for (int entityId : pool.viewIndex<CBoundCircle>())
    {
        const auto* mask = pool.getMask(entityId);
        if (pool.hasComponent<CPosition>(mask))
        {
            auto position = pool.getComponent<CPosition>(entityId);
            int  chunkX   = static_cast<int>(position->pos.x / col.spacing);
            int  chunkY   = static_cast<int>(position->pos.y / col.spacing);
            col.map.insert(chunkY, chunkX, entityId);
        }
    }
}

void SystemCollision::update()
{
    auto&       col        = context->mCollision;
    auto&       pool       = context->mAdmin.mPool;
    auto&       bounds     = pool.viewComponent<CBoundCircle>();
    const auto& boundIds   = pool.viewIndex<CBoundCircle>();
    const int   boundCount = bounds.size();

    int maxId = 0;
    for (int id : boundIds)
    {
        maxId = std::max(maxId, id);
    }

    auto& cachedBodies = col.mCachedBodies;
    if (cachedBodies.size() <= static_cast<size_t>(maxId))
    {
        cachedBodies.resize(maxId + 1);
    }
    std::fill(cachedBodies.begin(), cachedBodies.end(), CollisionData::CachedBody{});

    for (int i = 0; i < boundCount; i++)
    {
        const int entityId = boundIds[i];
        if (!GameStateDetail::isEntityAlive(context, entityId) || bounds[i].radius <= 0)
        {
            cachedBodies[entityId].entityId = -1;
            continue;
        }

        auto velocity             = pool.getComponent<CVelocity>(entityId);
        auto position             = pool.getComponent<CPosition>(entityId);
        velocity->separationForce = {};

        cachedBodies[entityId] = {entityId, position.get(), velocity.get(), &bounds[i]};
    }

    for (int i = 0; i < boundCount; i++)
    {
        const int   entityId = boundIds[i];
        const auto& body     = cachedBodies[entityId];
        if (body.entityId != entityId)
        {
            continue;
        }

        auto* velocity = body.vel;
        auto* position = body.pos;
        auto& bound    = *body.bound;

        const int currentChunkRow = static_cast<int>(position->pos.y / col.spacing);
        const int currentChunkCol = static_cast<int>(position->pos.x / col.spacing);

        if (!col.map.isValid(currentChunkRow, currentChunkCol))
        {
            continue;
        }

        int minRow = static_cast<int>((position->pos.y - bound.radius) / col.spacing);
        int maxRow = static_cast<int>((position->pos.y + bound.radius) / col.spacing);
        int minCol = static_cast<int>((position->pos.x - bound.radius) / col.spacing);
        int maxCol = static_cast<int>((position->pos.x + bound.radius) / col.spacing);

        for (int neighborRow = minRow; neighborRow <= maxRow; neighborRow++)
        {
            for (int neighborCol = minCol; neighborCol <= maxCol; neighborCol++)
            {
                if (!col.map.isValid(neighborRow, neighborCol))
                {
                    continue;
                }

                const auto& neighborIds = col.map.get(neighborRow, neighborCol).getEntityInCell();

                for (const int otherEntityId : neighborIds)
                {
                    if (entityId == otherEntityId || otherEntityId < 0 ||
                        otherEntityId >= static_cast<int>(cachedBodies.size()))
                    {
                        continue;
                    }

                    const auto& otherBody = cachedBodies[otherEntityId];
                    if (otherBody.entityId != otherEntityId || !otherBody.pos || !otherBody.vel || !otherBody.bound)
                    {
                        continue;
                    }

                    const f32::Vec2 offsetBetweenEntities = position->pos - otherBody.pos->pos;
                    const f32::Fix  collisionRange        = bound.radius + otherBody.bound->radius;
                    const f32::Fix  distSq                = offsetBetweenEntities.x * offsetBetweenEntities.x +
                                                            offsetBetweenEntities.y * offsetBetweenEntities.y;
                    const f32::Fix  rangeSq               = collisionRange * collisionRange;

                    if (distSq < rangeSq)
                    {
                        f32::Fix  distanceBetweenBodies = f32::sqrt(distSq);
                        f32::Vec2 separationDirection   = {};

                        if (distanceBetweenBodies == f32::Fix(0))
                        {
                            const std::array<f32::Vec2, 8> randomDirections = {{
                                {f32::Fix(1), f32::Fix(0)},
                                {f32::Fix(0), f32::Fix(1)},
                                {f32::Fix(0), f32::Fix(-1)},
                                {f32::Fix(-1), f32::Fix(0)},
                                {f32::Fix(-1), f32::Fix(1)},
                                {f32::Fix(-1), f32::Fix(-1)},
                                {f32::Fix(1), f32::Fix(1)},
                                {f32::Fix(1), f32::Fix(-1)},
                            }};

                            distanceBetweenBodies = f32::Fix(1) / 1000;
                            separationDirection   = randomDirections[context->mUpdateRandom.randInt(8)];
                        }
                        else
                        {
                            separationDirection = offsetBetweenEntities / distanceBetweenBodies;
                        }

                        const f32::Vec2 relVelocity    = velocity->vel - otherBody.vel->vel;
                        const f32::Fix  velAlongNormal = relVelocity.x * separationDirection.x +
                                                         relVelocity.y * separationDirection.y;

                        if (velAlongNormal < f32::Fix(0))
                        {
                            const f32::Fix restitution  = f32::Fix(2) / 10;
                            const f32::Fix impulseScale = f32::Fix(1) / 2;
                            const f32::Fix massA        = (bound.radius > f32::Fix(1)) ? bound.radius : f32::Fix(1);
                            const f32::Fix massB    = (otherBody.bound->radius > f32::Fix(1)) ? otherBody.bound->radius
                                                                                              : f32::Fix(1);
                            const f32::Fix impulseA = -(f32::Fix(1) + restitution) * velAlongNormal *
                                                      (massB / (massA + massB)) * impulseScale;

                            velocity->pushDirection += separationDirection * impulseA;
                        }

                        const f32::Fix separationStrength = (collisionRange - distanceBetweenBodies) / collisionRange;
                        velocity->separationForce += separationDirection * separationStrength;
                    }
                }
            }
        }
    }

    for (int i = 0; i < boundCount; i++)
    {
        const int entityId = boundIds[i];
        if (entityId >= 0 && entityId < static_cast<int>(cachedBodies.size()))
        {
            cachedBodies[entityId].entityId = -1;
        }
    }
}

void SystemPlayer::update()
{
    PROFILE_FUNCTION();
    if (!context)
        return;

    auto&            pool = context->mAdmin.mPool;
    std::vector<int> playerEntities;
    for (const int entityId : pool.viewIndex<CPlayer>())
    {
        playerEntities.push_back(entityId);
    }

    for (const int entityId : playerEntities)
    {
        const auto* mask = pool.getMask(entityId);
        if (!pool.hasComponent<CPlayer>(mask))
        {
            continue;
        }

        if (GameStateDetail::resetIfDead(context, entityId))
        {
            continue;
        }

        auto player = pool.getComponent<CPlayer>(entityId);
        if (player->respawnTimer > 0)
        {
            player->respawnTimer--;
            if (pool.hasComponent<CVelocity>(mask))
            {
                auto velocity              = pool.getComponent<CVelocity>(entityId);
                velocity->vel              = {};
                velocity->desiredDirection = {};
                velocity->pushDirection    = {};
                velocity->separationForce  = {};
            }
            continue;
        }

        if (player->skill1Timer > 0)
            player->skill1Timer--;
        if (player->skill2Timer > 0)
            player->skill2Timer--;
        if (player->skill3Timer > 0)
            player->skill3Timer--;
        if (player->attackTimer > 0)
            player->attackTimer--;
        if (player->showRangeTimer > 0)
            player->showRangeTimer--;

        if (pool.hasComponent<CVelocity>(mask))
        {
            auto      velocity = pool.getComponent<CVelocity>(entityId);
            f32::Vec2 moveDir  = velocity->desiredDirection;
            if (f32::length(moveDir) > f32::Fix(5) / 100)
            {
                moveDir = f32::normalized(moveDir);
            }
            f32::Fix maxSpeed = player->moveSpeed;
            GameStateDetail::applySteering(*velocity, moveDir, maxSpeed, f32::Fix(3) / 10);
        }
    }
}

void SystemUnit::update()
{
    PROFILE_FUNCTION();
    if (!context)
        return;

    auto& pool = context->mAdmin.mPool;

    for (int entityId : pool.viewIndex<CUnit>())
    {
        const auto* mask = pool.getMask(entityId);
        if (!GameStateDetail::isEntityAlive(context, entityId) || !pool.hasComponent<CHealth>(mask))
        {
            continue;
        }

        if (GameStateDetail::resetIfDead(context, entityId))
        {
            continue;
        }

        auto unit   = pool.getComponent<CUnit>(entityId);
        auto health = pool.getComponent<CHealth>(entityId);

        if (unit->state == UnitStateKind::Explode)
        {
            unit->explodeTimer--;

            if (unit->explodeTimer <= 0)
            {
                if (pool.hasComponent<CPosition>(mask))
                {
                    const auto     pos             = pool.getComponent<CPosition>(entityId);
                    const f32::Fix explosionRadius = 90;
                    const auto&    collision       = context->collision();

                    auto* effect            = context->alloc<CVisualEffect>();
                    effect->kind            = EffectKind::DeathExplosion;
                    effect->startPos        = pos->pos;
                    effect->endPos          = {};
                    effect->lifeTime        = 25;
                    effect->initialLifeTime = 25;
                    effect->teamId          = GameStateDetail::getEntityTeam(pool, entityId);
                    context->mAdmin.add(context->createBundle(effect));
                    context->playSound("SndExplosion", pos->pos, f32::Fix(1));

                    const int chunkRadius  = static_cast<int>(explosionRadius / collision.spacing) + 1;
                    const int centerChunkX = static_cast<int>(pos->pos.x / collision.spacing);
                    const int centerChunkY = static_cast<int>(pos->pos.y / collision.spacing);

                    for (int dy = -chunkRadius; dy <= chunkRadius; dy++)
                    {
                        for (int dx = -chunkRadius; dx <= chunkRadius; dx++)
                        {
                            const int chunkX = centerChunkX + dx;
                            const int chunkY = centerChunkY + dy;

                            if (chunkX < 0 || chunkY < 0 || chunkX >= collision.map.width() ||
                                chunkY >= collision.map.height())
                            {
                                continue;
                            }

                            const auto& agentIdInChunks = collision.map.get(chunkY, chunkX).getEntityInCell();

                            for (const int targetId : agentIdInChunks)
                            {
                                const auto* targetMask = pool.getMask(targetId);
                                if (targetId == entityId || !GameStateDetail::isEntityAlive(context, targetId) ||
                                    !pool.hasComponent<CHealth>(targetMask) || !pool.hasComponent<CPosition>(targetMask))
                                {
                                    continue;
                                }

                                if (!GameStateDetail::areOpposingTeams(pool, entityId, targetId))
                                {
                                    continue;
                                }

                                const auto     targetPos = pool.getComponent<CPosition>(targetId);
                                const f32::Fix distX     = targetPos->pos.x - pos->pos.x;
                                const f32::Fix distY     = targetPos->pos.y - pos->pos.y;
                                const f32::Fix dist      = f32::hypot(distX, distY);

                                if (dist <= explosionRadius)
                                {
                                    auto targetHealth = pool.getComponent<CHealth>(targetId);
                                    targetHealth->hp  = 0;
                                    GameStateDetail::resetIfDead(context, targetId);
                                }
                            }
                        }
                    }
                }
                GameStateDetail::resetIfDead(context, entityId);
            }

            continue;
        }

        if (unit->state == UnitStateKind::Retreat)
        {
            if (unit->retreatTimer > 0)
            {
                unit->retreatTimer--;
            }

            if (health->hp < health->maxHp)
            {
                unit->retreatHealTimer--;
                if (unit->retreatHealTimer <= 0)
                {
                    health->hp             = std::min(health->maxHp, health->hp + f32::Fix(5) / 2);
                    unit->retreatHealTimer = 40;
                }
            }
            else
            {
                unit->retreatHealTimer = 40;
            }

            if (health->hp >= health->maxHp)
            {
                handleUnitTransition(context, entityId, UnitEventKind::Safe);
                continue;
            }

            const int threatId = GameStateDetail::findClosestOpponent(context, entityId, unit->retreatSafeDistance);
            if (unit->retreatTimer <= 0 && threatId == -1)
            {
                handleUnitTransition(context, entityId, UnitEventKind::Safe);
                continue;
            }

            if (pool.hasComponent<CPosition>(mask) && pool.hasComponent<CVelocity>(mask))
            {
                auto        position   = pool.getComponent<CPosition>(entityId);
                auto        velocity   = pool.getComponent<CVelocity>(entityId);
                auto        desired    = position->posInit - position->pos;
                const auto* threatMask = pool.getMask(threatId);
                if (GameStateDetail::isEntityAlive(context, threatId) && pool.hasComponent<CPosition>(threatMask))
                {
                    const auto threatPosition = pool.getComponent<CPosition>(threatId);
                    desired += (position->pos - threatPosition->pos) * f32::Fix(3) / 2;
                }
                GameStateDetail::applySteering(*velocity, desired, unit->retreatMoveSpeed, GameStateDetail::kRetreatSteerWeight);
            }

            continue;
        }

        if (unit->state == UnitStateKind::Combat)
        {
            unit->combatTimer--;

            bool shouldRetreat = false;
            if (unit->unitClass == UnitClass::Berserker)
            {
                if (GameStateDetail::isLowHealth(*health))
                {
                    unit->moveSpeed      = f32::Fix(155) / 100;
                    unit->attackCooldown = 6;
                    unit->damage         = 3;
                }
            }
            else if (unit->unitClass == UnitClass::Tank)
            {
                if (health->hp <= health->maxHp * f32::Fix(15) / 100)
                {
                    shouldRetreat = true;
                }
            }
            else
            {
                if (GameStateDetail::isLowHealth(*health))
                {
                    shouldRetreat = true;
                }
            }

            if (shouldRetreat)
            {
                handleUnitTransition(context, entityId, UnitEventKind::HealthLow);
                continue;
            }

            const auto* targetMask = pool.getMask(unit->targetId);
            if (!GameStateDetail::isEntityAlive(context, unit->targetId) || !pool.hasComponent<CHealth>(targetMask) ||
                !GameStateDetail::areOpposingTeams(pool, entityId, unit->targetId) ||
                (pool.hasComponent<CUnit>(targetMask) &&
                 pool.getComponent<CUnit>(unit->targetId)->state == UnitStateKind::Explode))
            {
                handleUnitTransition(context, entityId, UnitEventKind::TargetLost);
                if (pool.hasComponent<CVelocity>(mask))
                {
                    auto velocity = pool.getComponent<CVelocity>(entityId);
                    GameStateDetail::applySteering(*velocity, {}, f32::Fix(0), f32::Fix(0));
                }
                continue;
            }

            const auto selfPosition   = pool.getComponent<CPosition>(entityId);
            const auto targetPosition = pool.getComponent<CPosition>(unit->targetId);
            f32::Vec2  targetPoint    = targetPosition->pos;
            if (pool.hasComponent<CBase>(targetMask) && pool.hasComponent<CRectDragDrop>(targetMask))
            {
                const auto     rect   = pool.getComponent<CRectDragDrop>(unit->targetId);
                const f32::Fix clampX = std::clamp(selfPosition->pos.x, rect->pos.x, rect->pos.x + rect->size.x);
                const f32::Fix clampY = std::clamp(selfPosition->pos.y, rect->pos.y, rect->pos.y + rect->size.y);
                targetPoint           = f32::Vec2{clampX, clampY};
            }

            const f32::Fix distanceToTarget = f32::length(targetPoint - selfPosition->pos);
            const f32::Fix rangeCombat = unit->disengageRange > 0 ? unit->disengageRange : GameStateDetail::kAcquireRange;

            if (distanceToTarget > rangeCombat)
            {
                if (unit->combatTimer < 0)
                {
                    handleUnitTransition(context, entityId, UnitEventKind::TargetLost);
                    unit->combatTimer = CUnit::kCombatInterval;
                    if (pool.hasComponent<CVelocity>(mask))
                    {
                        auto velocity = pool.getComponent<CVelocity>(entityId);
                        GameStateDetail::applySteering(*velocity, {}, f32::Fix(0), f32::Fix(0));
                    }
                    continue;
                }
            }

            if (pool.hasComponent<CVelocity>(mask))
            {
                auto       velocity = pool.getComponent<CVelocity>(entityId);
                const bool isRanged = unit->attackRange > f32::Fix(50);
                const auto toTarget = targetPoint - selfPosition->pos;

                if (isRanged && distanceToTarget < unit->attackRange * f32::Fix(7) / 10)
                {
                    const auto awayFromTarget = selfPosition->pos - targetPoint;
                    GameStateDetail::applySteering(*velocity,
                                                   awayFromTarget,
                                                   unit->moveSpeed * f32::Fix(12) / 10,
                                                   GameStateDetail::kCombatSteerWeight);
                }
                else if (distanceToTarget > unit->attackRange * f32::Fix(8) / 10)
                {
                    GameStateDetail::applySteering(*velocity, toTarget, unit->moveSpeed, GameStateDetail::kCombatSteerWeight);
                }
                else
                {
                    GameStateDetail::applySteering(*velocity, {}, f32::Fix(0), f32::Fix(0));
                }
            }

            if (unit->attackTimer > 0)
            {
                unit->attackTimer--;
            }

            if (distanceToTarget > unit->attackRange || unit->attackTimer > 0 ||
                !GameStateDetail::hasLineOfSight(context, entityId, unit->targetId))
            {
                continue;
            }

            unit->attackTimer = unit->attackCooldown;

            if (unit->unitClass == UnitClass::Warrior || unit->unitClass == UnitClass::Grunt ||
                unit->unitClass == UnitClass::Berserker)
            {
                if (pool.hasComponent<CHealth>(targetMask))
                {
                    auto targetHealth = pool.getComponent<CHealth>(unit->targetId);
                    auto dmg          = unit->damage + unit->buff.damageBonus;
                    if (pool.hasComponent<CUnit>(targetMask))
                    {
                        auto targetUnit = pool.getComponent<CUnit>(unit->targetId);
                        if (targetUnit->buff.shieldHp > 0)
                        {
                            f32::Fix shieldBlock = std::min(targetUnit->buff.shieldHp, dmg);
                            targetUnit->buff.shieldHp -= shieldBlock;
                            dmg -= shieldBlock;
                        }
                    }
                    else if (pool.hasComponent<CPlayer>(targetMask))
                    {
                        auto targetPlayer = pool.getComponent<CPlayer>(unit->targetId);
                        if (targetPlayer->buff.shieldHp > 0)
                        {
                            f32::Fix shieldBlock = std::min(targetPlayer->buff.shieldHp, dmg);
                            targetPlayer->buff.shieldHp -= shieldBlock;
                            dmg -= shieldBlock;
                        }
                    }
                    if (targetHealth->armor > 0 && dmg > 0)
                    {
                        f32::Fix armorBlock = std::min(targetHealth->armor, dmg);
                        targetHealth->armor -= armorBlock;
                        dmg -= armorBlock;
                    }
                    targetHealth->hp -= dmg;
                    if (GameStateDetail::resetIfDead(context, unit->targetId) ||
                        !GameStateDetail::isEntityAlive(context, unit->targetId) ||
                        (pool.hasComponent<CUnit>(targetMask) &&
                         pool.getComponent<CUnit>(unit->targetId)->state == UnitStateKind::Explode))
                    {
                        handleUnitTransition(context, entityId, UnitEventKind::TargetLost);
                    }
                }
                continue;
            }

            if (unit->unitClass == UnitClass::Archer || unit->unitClass == UnitClass::Shooter)
            {
                GameStateDetail::fireProjectile(context, entityId, unit->targetId, *unit);
                continue;
            }

            auto selfVelocity    = pool.getComponent<CVelocity>(entityId);
            auto targetHealth    = pool.getComponent<CHealth>(unit->targetId);
            auto impactDirection = targetPosition->pos - selfPosition->pos;

            if (GameStateDetail::squaredLength(impactDirection) > f32::Fix(1) / 10000)
            {
                impactDirection = f32::normalized(impactDirection);
            }

            auto dmg = unit->damage + unit->buff.damageBonus;
            if (pool.hasComponent<CUnit>(targetMask))
            {
                auto targetUnit = pool.getComponent<CUnit>(unit->targetId);
                if (targetUnit->buff.shieldHp > 0)
                {
                    f32::Fix shieldBlock = std::min(targetUnit->buff.shieldHp, dmg);
                    targetUnit->buff.shieldHp -= shieldBlock;
                    dmg -= shieldBlock;
                }
            }
            else if (pool.hasComponent<CPlayer>(targetMask))
            {
                auto targetPlayer = pool.getComponent<CPlayer>(unit->targetId);
                if (targetPlayer->buff.shieldHp > 0)
                {
                    f32::Fix shieldBlock = std::min(targetPlayer->buff.shieldHp, dmg);
                    targetPlayer->buff.shieldHp -= shieldBlock;
                    dmg -= shieldBlock;
                }
            }
            if (targetHealth->armor > 0 && dmg > 0)
            {
                f32::Fix armorBlock = std::min(targetHealth->armor, dmg);
                targetHealth->armor -= armorBlock;
                dmg -= armorBlock;
            }
            targetHealth->hp -= dmg;
            selfVelocity->pushDirection -= impactDirection * (GameStateDetail::kImpactImpulse * f32::Fix(1) / 2);

            if (pool.hasComponent<CVelocity>(targetMask))
            {
                auto targetVelocity = pool.getComponent<CVelocity>(unit->targetId);
                targetVelocity->pushDirection += impactDirection * GameStateDetail::kImpactImpulse;
            }

            if (GameStateDetail::resetIfDead(context, unit->targetId) ||
                !GameStateDetail::isEntityAlive(context, unit->targetId) ||
                (pool.hasComponent<CUnit>(targetMask) &&
                 pool.getComponent<CUnit>(unit->targetId)->state == UnitStateKind::Explode))
            {
                handleUnitTransition(context, entityId, UnitEventKind::TargetLost);
            }

            continue;
        }

        if (unit->state == UnitStateKind::Search)
        {
            unit->searchTimer--;

            if (unit->searchTimer <= 0)
            {
                unit->searchTimer = CUnit::kSearchInterval;

                const int myTeam   = GameStateDetail::getEntityTeam(pool, entityId);
                int       targetId = -1;

                if (context->getGameMode() == GameMode::Coop && myTeam != 0)
                {
                    targetId = GameStateDetail::findClosestBase(context, entityId, GameStateDetail::kAcquireRange);
                    if (targetId == -1)
                    {
                        targetId = GameStateDetail::findClosestOpponent(context, entityId, GameStateDetail::kAcquireRange);
                    }
                }
                else
                {
                    targetId = GameStateDetail::findClosestOpponent(context, entityId, GameStateDetail::kAcquireRange);
                    if (targetId == -1)
                    {
                        targetId = GameStateDetail::findClosestBase(context, entityId, GameStateDetail::kAcquireRange);
                    }
                }

                if (targetId != -1)
                {
                    const bool canSeeTarget = GameStateDetail::hasLineOfSight(context, entityId, targetId);
                    if (canSeeTarget)
                    {
                        handleUnitTransition(context, entityId, UnitEventKind::OpponentSpotted, targetId);
                        continue;
                    }
                }
            }

            if (pool.hasComponent<CPosition>(mask) && pool.hasComponent<CVelocity>(mask))
            {
                auto      position       = pool.getComponent<CPosition>(entityId);
                auto      velocity       = pool.getComponent<CVelocity>(entityId);
                const int teamId         = GameStateDetail::getEntityTeam(pool, entityId);
                auto&     directionField = (unit->stance == CombatStance::Tactical)
                                               ? context->mNav.getDesignatedField(teamId)
                                               : context->mNav.getTeamField(teamId);

                const int cellX = static_cast<int>(position->pos.x / context->mNav.spacing);
                const int cellY = static_cast<int>(position->pos.y / context->mNav.spacing);

                auto direction = directionField.getDirections(cellX, cellY);
                if (direction.x == 0 && direction.y == 0)
                {
                    direction = directionField.getValidDirectionNear(cellX, cellY);
                }

                f32::Fix maxSpeed = GameStateDetail::kSearchMaxSpeed;
                GameStateDetail::applySteering(*velocity,
                                               f32::Vec2(direction.x, direction.y),
                                               maxSpeed,
                                               GameStateDetail::kSearchSteerWeight);
            }
        }
    }
}

void SystemUnit::handleUnitTransition(GameState* context, int entityId, UnitEventKind event, int targetId)
{
    if (!context || !GameStateDetail::isEntityAlive(context, entityId))
    {
        return;
    }

    auto&       pool    = context->mAdmin.mPool;
    const auto* msgMask = pool.getMask(entityId);
    if (!pool.hasComponent<CUnit>(msgMask))
    {
        return;
    }

    auto unit = pool.getComponent<CUnit>(entityId);

    switch (unit->state)
    {
        case UnitStateKind::Search:
        {
            if (event == UnitEventKind::OpponentSpotted && targetId != -1)
            {
                unit->state       = UnitStateKind::Combat;
                unit->targetId    = targetId;
                unit->combatTimer = 10;
                unit->attackTimer = 0;
            }
            break;
        }
        case UnitStateKind::Combat:
        {
            if (event == UnitEventKind::TargetLost)
            {
                unit->state       = UnitStateKind::Search;
                unit->targetId    = -1;
                unit->searchTimer = 0;
            }
            else if (event == UnitEventKind::HealthLow)
            {
                unit->state               = UnitStateKind::Retreat;
                unit->retreatTimer        = GameConfig::AI::kRetreatDurationTick;
                unit->retreatHealTimer    = 40;
                unit->retreatMoveSpeed    = unit->moveSpeed * f32::Fix(3) / 2 + unit->buff.speedBonus * f32::Fix(3) / 2;
                unit->retreatSafeDistance = (unit->attackRange > 50) ? f32::Fix(180) : f32::Fix(140);
            }
            break;
        }
        case UnitStateKind::Retreat:
        {
            if (event == UnitEventKind::Safe)
            {
                unit->state       = UnitStateKind::Search;
                unit->targetId    = -1;
                unit->searchTimer = 0;
            }
            break;
        }
        default:
            break;
    }
}

void SystemPickup::update()
{
    PROFILE_FUNCTION();
    if (!context)
        return;
    GameStateDetail::updatePickupsAndProgress(context);
}

void SystemVisualEffect::update()
{
    PROFILE_FUNCTION();

    auto& mAdmin = context->mAdmin;

    for (const int effectId : context->mAdmin.mPool.viewIndex<CVisualEffect>())
    {
        auto effect = context->mAdmin.mPool.getComponent<CVisualEffect>(effectId);
        effect->lifeTime--;
        if (effect->lifeTime <= 0)
        {
            mAdmin.remove(effectId);
        }
    }
}


} // namespace SystemGameState
