#include "GameState.hpp"

#include "Command.hpp"
#include "GameConfig.hpp"
#include "GameStateDetail.hpp"
#include "Profiler.hpp"

GameState::GameState() : mSystems(this)
{
    mAdmin.gameState                           = this;
    mProfilerTimes["1. Commands & Admin"]      = 0;
    mProfilerTimes["2. VectorField & Vision"]  = 0;
    mProfilerTimes["3. Collision Resolution"]  = 0;
    mProfilerTimes["4. AI & Combat State"]     = 0;
    mProfilerTimes["5. Pickups & Items"]       = 0;
    mProfilerTimes["6. Waves & Objectives"]    = 0;
    mProfilerTimes["7. Movement & Kinematics"] = 0;
    mProfilerTimes["8. Base Defenses"]         = 0;
    mProfilerTimes["9. Wall & Turret Systems"] = 0;
    mProfilerTimes["10. Projectiles"]          = 0;
    mProfilerTimes["11. Visual Effects"]       = 0;
}

int GameState::getPlayerTeam(int playerId) const
{
    return GameStateDetail::getEntityTeam(mAdmin.mPool, playerId);
}

void GameState::execute(const Command& command)
{
    switch (command.type())
    {
        case CommandType::NoOp:
            break;

        case CommandType::MovePlayer:
        {
            const auto& data = std::get<CmdMovePlayer>(command.data());
            int         pid  = data.playerId;
            const auto* mask = mAdmin.mPool.getMask(pid);
            if (pid >= 0 && mAdmin.mPool.hasComponent<CPlayer>(mask) && !mAdmin.isPendingRemove(pid) &&
                mAdmin.mPool.hasComponent<CVelocity>(mask))
            {
                auto player = mAdmin.mPool.getComponent<CPlayer>(pid);
                if (player->respawnTimer == 0)
                {
                    auto vel              = mAdmin.mPool.getComponent<CVelocity>(pid);
                    vel->desiredDirection = f32::Vec2{data.dirX, data.dirY};
                }
            }
            break;
        }
        case CommandType::PlaceWall:
        {
            const auto& data = std::get<CmdPlaceWall>(command.data());
            f32::Vec2   center{data.worldX, data.worldY};
            GameStateDetail::placePlayerWall(this, center, data.kind, data.playerId);
            break;
        }
        case CommandType::PlaceUnit:
        {
            const auto& data = std::get<CmdPlaceUnit>(command.data());
            GameStateDetail::placePlayerUnit(this, f32::Vec2{data.worldX, data.worldY}, data.unitClass, data.playerId);
            break;
        }
        case CommandType::MoveWall:
        {
            const auto& data     = std::get<CmdMoveWall>(command.data());
            const auto* wallMask = mAdmin.mPool.getMask(data.wallId);
            if (data.wallId >= 0 && (data.wallGen == 0 || GameStateDetail::isEntityAlive(this, data.wallId, data.wallGen)) &&
                GameStateDetail::isEntityAlive(this, data.wallId) && mAdmin.mPool.hasComponent<CRectDragDrop>(wallMask))
            {
                if (data.playerId != -1 && !GameStateDetail::isPlayerWallOwner(mAdmin.mPool, data.wallId, data.playerId))
                {
                    break;
                }
                if (data.playerId == -1 && GameStateDetail::getEntityPlayerId(mAdmin.mPool, data.wallId) != -1)
                {
                    break;
                }

                auto      wallRect  = mAdmin.mPool.getComponent<CRectDragDrop>(data.wallId);
                f32::Vec2 oldCenter = wallRect->pos + wallRect->size * f32::Fix(1) / 2;

                f32::Vec2 newPos{data.worldX - wallRect->size.x * f32::Fix(1) / 2,
                                 data.worldY - wallRect->size.y * f32::Fix(1) / 2};
                newPos.x      = std::clamp(newPos.x, f32::Fix(0), mapWidth() - wallRect->size.x);
                newPos.y      = std::clamp(newPos.y, f32::Fix(0), mapHeight() - wallRect->size.y);
                wallRect->pos = newPos;

                f32::Vec2 newCenter = wallRect->pos + wallRect->size * f32::Fix(1) / 2;
                f32::Fix  distMoved = f32::hypot(newCenter.x - oldCenter.x, newCenter.y - oldCenter.y);

                const bool isDesignatedPoint = mAdmin.mPool.hasComponent<CWall>(wallMask) &&
                                               (mAdmin.mPool.getComponent<CWall>(data.wallId)->kind ==
                                                WallKind::DesignatedPoint);
                if ((isDesignatedPoint || mAdmin.mPool.hasComponent<CBase>(wallMask)) &&
                    mAdmin.mPool.hasComponent<CPosition>(wallMask))
                {
                    auto pos     = mAdmin.mPool.getComponent<CPosition>(data.wallId);
                    pos->pos     = wallRect->pos + wallRect->size * f32::Fix(1) / 2;
                    pos->prevPos = pos->pos;
                }

                this->nav().requestRecompute();

                if (distMoved > f32::Fix(8))
                {
                    const int wallTeam = GameStateDetail::getEntityTeam(mAdmin.mPool, data.wallId);

                    auto* warpBeam            = alloc<CVisualEffect>();
                    warpBeam->kind            = EffectKind::ElectricBeam;
                    warpBeam->startPos        = oldCenter;
                    warpBeam->endPos          = newCenter;
                    warpBeam->lifeTime        = 14;
                    warpBeam->initialLifeTime = 14;
                    warpBeam->teamId          = wallTeam;
                    mAdmin.add(createBundle(warpBeam));

                    playSound("SndBuild", newCenter, f32::Fix(95) / 100);
                }
            }
            break;
        }
        case CommandType::PlayerAttack:
        {
            const auto& data = std::get<CmdPlayerAttack>(command.data());
            int         pid  = data.playerId;
            const auto* mask = mAdmin.mPool.getMask(pid);
            if (pid >= 0 && mAdmin.mPool.hasComponent<CPlayer>(mask) && !mAdmin.isPendingRemove(pid) &&
                mAdmin.mPool.hasComponent<CPosition>(mask))
            {
                auto playerComp = mAdmin.mPool.getComponent<CPlayer>(pid);
                if (playerComp->respawnTimer == 0 && playerComp->attackTimer == 0)
                {
                    auto     position    = mAdmin.mPool.getComponent<CPosition>(pid);
                    f32::Fix attackRange = playerComp->attackRange;
                    int targetId = GameStateDetail::findClosestOpponentToPoint(this, position->pos, attackRange, pid);
                    const auto* targetMask = mAdmin.mPool.getMask(targetId);
                    if (targetId != -1 && GameStateDetail::isEntityAlive(this, targetId) &&
                        mAdmin.mPool.hasComponent<CPosition>(targetMask) &&
                        GameStateDetail::areOpposingTeams(mAdmin.mPool, pid, targetId))
                    {
                        auto      targetPos = mAdmin.mPool.getComponent<CPosition>(targetId);
                        f32::Vec2 dir       = targetPos->pos - position->pos;
                        f32::Fix  dmg       = playerComp->damage;

                        mAdmin.add(GameStateDetail::createProjectileBundle(this, pid, targetId, position->pos, dir, dmg));
                        playSound("SndShoot", position->pos, f32::Fix(9) / 10);
                        playerComp->attackTimer    = playerComp->attackCooldown;
                        playerComp->showRangeTimer = 30;
                    }
                }
            }
            break;
        }
        case CommandType::PlayerSkill:
        {
            const auto& data = std::get<CmdPlayerSkill>(command.data());
            int         pid  = data.playerId;
            const auto* mask = mAdmin.mPool.getMask(pid);
            if (pid >= 0 && mAdmin.mPool.hasComponent<CPlayer>(mask) && !mAdmin.isPendingRemove(pid) &&
                mAdmin.mPool.hasComponent<CPosition>(mask))
            {
                auto playerComp = mAdmin.mPool.getComponent<CPlayer>(pid);
                if (playerComp->respawnTimer == 0)
                {
                    auto position = mAdmin.mPool.getComponent<CPosition>(pid);

                    if (data.skillId == 1 && playerComp->skill1Timer == 0)
                    {
                        f32::Fix sniperRange = f32::Fix(300);
                        int targetId = GameStateDetail::findClosestOpponentToPoint(this, position->pos, sniperRange, pid);
                        const auto* targetMask = mAdmin.mPool.getMask(targetId);
                        if (targetId != -1 && GameStateDetail::isEntityAlive(this, targetId) &&
                            mAdmin.mPool.hasComponent<CPosition>(targetMask) &&
                            GameStateDetail::areOpposingTeams(mAdmin.mPool, pid, targetId))
                        {
                            auto      targetPos = mAdmin.mPool.getComponent<CPosition>(targetId);
                            f32::Vec2 dir       = targetPos->pos - position->pos;
                            f32::Fix  dmg       = f32::Fix(5);

                            auto* bundle = GameStateDetail::createProjectileBundle(this, pid, targetId, position->pos, dir, dmg);
                            auto* bundleCast = static_cast<ComponentBundle<CPosition, CVelocity, CProjectile>*>(bundle);
                            if (bundleCast)
                            {
                                std::get<2>(bundleCast->components)->hitRadius = f32::Fix(20);
                                std::get<2>(bundleCast->components)->lifeTime  = 90;
                            }

                            mAdmin.add(bundle);
                            playSound("SndShoot", position->pos, f32::Fix(9) / 10);
                            playerComp->skill1Timer    = playerComp->skill1Cooldown;
                            playerComp->showRangeTimer = 30;
                        }
                    }
                    else if (data.skillId == 2 && playerComp->skill2Timer == 0)
                    {
                        f32::Fix  fireRange  = f32::Fix(150);
                        auto      targetIds  = GameStateDetail::findAgentInRange(this, position->pos, fireRange);
                        bool      fired      = false;
                        const int playerTeam = getPlayerTeam(pid);
                        for (int targetId : targetIds)
                        {
                            const auto* targetMask = mAdmin.mPool.getMask(targetId);
                            if (targetId >= 0 && GameStateDetail::areOpposingTeams(mAdmin.mPool, pid, targetId) &&
                                GameStateDetail::isEntityAlive(this, targetId) &&
                                mAdmin.mPool.hasComponent<CPosition>(targetMask))
                            {
                                fired                     = true;
                                const f32::Vec2 targetPos = mAdmin.mPool.getComponent<CPosition>(targetId)->pos;
                                f32::Fix        dmg       = f32::Fix(4);
                                GameStateDetail::applyWallDamage(this,
                                                                 targetId,
                                                                 dmg,
                                                                 position->pos,
                                                                 GameStateDetail::kImpactImpulse * f32::Fix(3) / 2);

                                auto* effect            = alloc<CVisualEffect>();
                                effect->kind            = EffectKind::FireBeam;
                                effect->startPos        = position->pos;
                                effect->endPos          = targetPos;
                                effect->lifeTime        = 18;
                                effect->initialLifeTime = 18;
                                effect->teamId          = playerTeam;
                                mAdmin.add(createBundle(effect));
                            }
                        }
                        if (fired)
                        {
                            playSound("SndExplode", position->pos, f32::Fix(1));
                            playerComp->skill2Timer    = playerComp->skill2Cooldown;
                            playerComp->showRangeTimer = 30;
                        }
                    }
                    else if (data.skillId == 3 && playerComp->skill3Timer == 0)
                    {
                        const f32::Fix slashRange = f32::Fix(120);
                        const auto*    velMask    = mAdmin.mPool.getMask(pid);
                        f32::Vec2      slashDir(f32::Fix(0), f32::Fix(0));

                        if (mAdmin.mPool.hasComponent<CVelocity>(velMask))
                        {
                            slashDir = mAdmin.mPool.getComponent<CVelocity>(pid)->desiredDirection;
                        }

                        if (f32::length(slashDir) < f32::Fix(1) / 10)
                        {
                            const int nearest = GameStateDetail::findClosestOpponentToPoint(this, position->pos, slashRange * 2, pid);
                            if (nearest != -1 && mAdmin.mPool.hasComponent<CPosition>(mAdmin.mPool.getMask(nearest)))
                            {
                                slashDir = mAdmin.mPool.getComponent<CPosition>(nearest)->pos - position->pos;
                            }
                            else
                            {
                                slashDir = f32::Vec2(f32::Fix(1), f32::Fix(0));
                            }
                        }
                        slashDir = f32::normalized(slashDir);

                        const auto targetIds  = GameStateDetail::findAgentInRange(this, position->pos, slashRange);
                        bool       hitAny     = false;
                        const int  playerTeam = getPlayerTeam(pid);

                        for (int targetId : targetIds)
                        {
                            const auto* targetMask = mAdmin.mPool.getMask(targetId);
                            if (targetId >= 0 && GameStateDetail::areOpposingTeams(mAdmin.mPool, pid, targetId) &&
                                GameStateDetail::isEntityAlive(this, targetId) &&
                                mAdmin.mPool.hasComponent<CPosition>(targetMask))
                            {
                                const f32::Vec2 targetPos = mAdmin.mPool.getComponent<CPosition>(targetId)->pos;
                                const f32::Vec2 toEnemy   = targetPos - position->pos;
                                const f32::Fix  dist      = f32::length(toEnemy);

                                if (dist > f32::Fix(0))
                                {
                                    const f32::Vec2 toEnemyNorm = toEnemy / dist;
                                    const f32::Fix  dot = slashDir.x * toEnemyNorm.x + slashDir.y * toEnemyNorm.y;

                                    if (dot >= -f32::Fix(1) / 10 || dist <= f32::Fix(65))
                                    {
                                        hitAny                  = true;
                                        const f32::Fix slashDmg = f32::Fix(7);
                                        GameStateDetail::applyWallDamage(this, targetId, slashDmg, position->pos, f32::Fix(0));

                                        if (mAdmin.mPool.hasComponent<CVelocity>(targetMask))
                                        {
                                            auto      velComp      = mAdmin.mPool.getComponent<CVelocity>(targetId);
                                            f32::Vec2 blendedDir   = toEnemyNorm * f32::Fix(6) / 10 +
                                                                     slashDir * f32::Fix(4) / 10;
                                            f32::Vec2 knockbackDir = (f32::length(blendedDir) > f32::Fix(1) / 100)
                                                                         ? f32::normalized(blendedDir)
                                                                         : toEnemyNorm;
                                            velComp->vel           = knockbackDir * f32::Fix(3) / 2;
                                            velComp->pushDirection += knockbackDir * f32::Fix(4);
                                        }
                                    }
                                }
                            }
                        }

                        auto* effect            = alloc<CVisualEffect>();
                        effect->kind            = EffectKind::SlashArc;
                        effect->startPos        = position->pos;
                        effect->endPos          = position->pos + slashDir * slashRange;
                        effect->lifeTime        = 14;
                        effect->initialLifeTime = 14;
                        effect->teamId          = playerTeam;
                        mAdmin.add(createBundle(effect));

                        if (hitAny)
                        {
                            playSound("SndHit", position->pos, f32::Fix(1));
                        }
                        else
                        {
                            playSound("SndShoot", position->pos, f32::Fix(7) / 10);
                        }

                        playerComp->skill3Timer    = playerComp->skill3Cooldown;
                        playerComp->showRangeTimer = 25;
                    }
                }
            }
            break;
        }
        case CommandType::CombatMode:
        {
            const auto& data       = std::get<CmdCombatMode>(command.data());
            int         style      = (data.state == CombatStance::Attack) ? 3 : 4;
            int         playerTeam = (data.playerId != -1) ? getPlayerTeam(data.playerId) : 0;
            for (int id : mAdmin.mPool.viewIndex<CUnit>())
            {
                if (id < 0 || !GameStateDetail::isEntityAlive(this, id))
                    continue;

                auto unit = mAdmin.mPool.getComponent<CUnit>(id);
                if (!unit->isSelected)
                    continue;

                if (GameStateDetail::getEntityTeam(mAdmin.mPool, id) != playerTeam)
                    continue;

                const auto* mask = mAdmin.mPool.getMask(id);
                if (mAdmin.mPool.hasComponent<CPosition>(mask))
                {
                    GameStateDetail::spawnClickIndicator(this, mAdmin.mPool.getComponent<CPosition>(id)->pos, style, 20, playerTeam);
                }

                unit->stance     = data.state;
                unit->isSelected = false;
            }
            break;
        }
        case CommandType::SetGameSpeed:
        {
            const auto& data = std::get<CmdSetGameSpeed>(command.data());
            mTimeScale       = data.timeScale;
            break;
        }
        case CommandType::SelectUnits:
        {
            const auto&   data = std::get<CmdSelectUnits>(command.data());
            std::set<int> selectedSet(data.unitIds.begin(), data.unitIds.end());
            int           playerTeam = (data.playerId != -1) ? getPlayerTeam(data.playerId) : 0;
            for (int id : mAdmin.mPool.viewIndex<CUnit>())
            {
                if (id < 0 || !GameStateDetail::isEntityAlive(this, id))
                    continue;

                if (GameStateDetail::getEntityTeam(mAdmin.mPool, id) != playerTeam)
                    continue;

                auto unit        = mAdmin.mPool.getComponent<CUnit>(id);
                unit->isSelected = selectedSet.contains(id);
            }
            break;
        }
        case CommandType::OrderUnits:
        {
            const auto& data       = std::get<CmdOrderUnits>(command.data());
            int         playerTeam = (data.playerId != -1) ? getPlayerTeam(data.playerId) : 0;
            for (int id : data.unitIds)
            {
                const auto* mask = mAdmin.mPool.getMask(id);
                if (id < 0 || !GameStateDetail::isEntityAlive(this, id) || !mAdmin.mPool.hasComponent<CUnit>(mask))
                    continue;

                if (GameStateDetail::getEntityTeam(mAdmin.mPool, id) != playerTeam)
                    continue;

                if (data.targetEntityId != -1)
                {
                    if (data.targetEntityId >= 0 &&
                        (data.targetEntityGen == 0 ||
                         GameStateDetail::isEntityAlive(this, data.targetEntityId, data.targetEntityGen)) &&
                        GameStateDetail::isEntityAlive(this, data.targetEntityId) &&
                        GameStateDetail::areOpposingTeams(mAdmin.mPool, id, data.targetEntityId))
                    {
                        GameStateDetail::emitUnitEvent(this, id, UnitEventKind::OpponentSpotted, data.targetEntityId);
                    }
                }
            }
            break;
        }
    }
}

bool GameState::update(const CommandOnTick& commandOnTick)
{
    PROFILE_FUNCTION();
    if (mDesyncDetected)
    {
        return false;
    }

    if (isGameOver())
    {
        return false;
    }

    shootClock++;
    hitClock++;
    elecClock++;
    fireClock++;
    healSoundClock++;

    auto t0 = std::chrono::high_resolution_clock::now();
    {
        PROFILE_SCOPE("Commands and Admin");
        mAdmin.update();
        for (const auto& command : commandOnTick.commands)
        {
            execute(command);
            mAdmin.update();
        }
        mAdmin.update();
    }
    auto t1                               = std::chrono::high_resolution_clock::now();
    mProfilerTimes["1. Commands & Admin"] = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    auto tStart = t1;
    {
        PROFILE_SCOPE("1. NavigationField (VectorField & Vision)");
        mSystems.get<SystemGameState::SystemNavigationField>().update();
    }
    auto tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["2. VectorField & Vision"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("2. SystemCollision (Spatial Chunk Partitioning)");
        mSystems.get<SystemGameState::SystemCollision>().update();
    }
    tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["3. Collision Resolution"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("3. SystemUnit & SystemPlayer");
        mSystems.get<SystemGameState::SystemUnit>().update();
        mSystems.get<SystemGameState::SystemPlayer>().update();
    }
    tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["4. AI & Combat State"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("4. SystemPickup (Item Drops & Buffs)");
        mSystems.get<SystemGameState::SystemPickup>().update();
    }
    tEnd                                 = std::chrono::high_resolution_clock::now();
    mProfilerTimes["5. Pickups & Items"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("5. SystemMatchEnd (Waves, Objectives & Match End)");
        mSystems.get<SystemGameState::SystemMatchEnd>().update();
    }
    tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["6. Waves & Objectives"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("6. SystemMovement (Kinematics Integration)");
        mSystems.get<SystemGameState::SystemMovement>().update();
    }
    tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["7. Movement & Kinematics"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("7. SystemBase (Base Defenses)");
        mSystems.get<SystemGameState::SystemBase>().update();
    }
    tEnd                               = std::chrono::high_resolution_clock::now();
    mProfilerTimes["8. Base Defenses"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("8. SystemWall (Turrets & Wall Effects)");
        mSystems.get<SystemGameState::SystemWall>().update();
    }
    tEnd = std::chrono::high_resolution_clock::now();
    mProfilerTimes["9. Wall & Turret Systems"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("9. SystemProjectile (Projectiles & Damage)");
        mSystems.get<SystemGameState::SystemProjectile>().update();
    }
    tEnd                              = std::chrono::high_resolution_clock::now();
    mProfilerTimes["10. Projectiles"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    tStart = tEnd;
    {
        PROFILE_SCOPE("10. SystemVisualEffect (Visual Effects)");
        mSystems.get<SystemGameState::SystemVisualEffect>().update();
    }
    tEnd                                 = std::chrono::high_resolution_clock::now();
    mProfilerTimes["11. Visual Effects"] = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();

    mTick++;

    return true;
}

std::string GameState::dumpState() const
{
    std::string s;
    s.reserve(16384);

    s += "Tick:" + std::to_string(mTick) + "|";
    s += "RNG:" + std::to_string(mUpdateRandom.mState) + "|";
    s += "TimeScale:" + std::to_string(mTimeScale) + "|";

    s += "Admin:max=" + std::to_string(mAdmin.maxId()) + ",free=" + std::to_string(mAdmin.freeIds().size()) + "[";
    for (int fid : mAdmin.freeIds().st)
    {
        s += std::to_string(fid) + ",";
    }
    s += "]|";

    s += "Map:" + std::to_string(mapWidth()) + "x" + std::to_string(mapHeight()) + "|";

    const auto& c = campaign();
    s += "Camp:map=" + std::to_string(c.currentMap) + ",lvl=" + std::to_string(c.currentLevel) +
         ",maxLvl=" + std::to_string(c.maxLevel) + ",wave=" + std::to_string(c.currentWave) +
         ",diff=" + std::to_string(c.difficulty) + ",sup=" + std::to_string(c.collectedSupplies) + "/" +
         std::to_string(c.requiredSupplies) + "/" + std::to_string(c.totalSupplies) +
         ",t0Units=" + std::to_string(c.team0UnitCount) + ",t1Units=" + std::to_string(c.team1UnitCount) +
         ",deaths=" + std::to_string(c.unitDeathsThisLevel) + "/" + std::to_string(c.unitDeathLimit) +
         ",pendingLvl=" + (c.pendingLevelReset ? "1" : "0") + ",walls=" + std::to_string(c.defensiveWallStock) + "," +
         std::to_string(c.electricWallStock) + "," + std::to_string(c.fireWallStock) + "," +
         std::to_string(c.gunWallStock) + "," + std::to_string(c.radarWallStock) +
         ",desPt=" + std::to_string(c.designatedPointStock) + ",units=" + std::to_string(c.warriorUnitStock) + "," +
         std::to_string(c.archerUnitStock) + "," + std::to_string(c.tankUnitStock) + "," +
         std::to_string(c.gruntUnitStock) + "," + std::to_string(c.shooterUnitStock) + "," +
         std::to_string(c.berserkerUnitStock) + ",mode=" + std::to_string(static_cast<int>(mGameMode)) + ",win=" +
         std::to_string(mWinningTeam) + ",vic=" + (c.victory ? "1" : "0") + ",over=" + (c.gameOver ? "1" : "0") + "|";
    for (size_t t = 1; t < c.teamStocks.size(); ++t)
    {
        const auto& ts = c.teamStocks[t];
        s += "TS" + std::to_string(t) + ":w=" + std::to_string(ts.defensiveWallStock) + "," +
             std::to_string(ts.electricWallStock) + "," + std::to_string(ts.fireWallStock) + "," +
             std::to_string(ts.gunWallStock) + "," + std::to_string(ts.radarWallStock) + "," +
             std::to_string(ts.designatedPointStock) + ",u=" + std::to_string(ts.warriorUnitStock) + "," +
             std::to_string(ts.archerUnitStock) + "," + std::to_string(ts.tankUnitStock) + "," +
             std::to_string(ts.gruntUnitStock) + "," + std::to_string(ts.shooterUnitStock) + "," +
             std::to_string(ts.berserkerUnitStock) + "|";
    }

    s += "Init:maxT0=" + std::to_string(maxTeam0Units) + ",maxT1=" + std::to_string(maxTeam1Units) + "|";

    const auto& f = mNav;
    s += "Field:sp=" + std::to_string(f.spacing) + ",vis=" + std::to_string(f.visionSpacing) +
         ",int=" + std::to_string(f.updateIntervalTick) + ",rem=" + std::to_string(f.ticksUntilUpdate) +
         ",vint=" + std::to_string(f.visionUpdateIntervalTick) + ",vrem=" + std::to_string(f.visionTicksUntilUpdate) +
         ",del=" + std::to_string(f.delayFrames) + ",dirty=" + (f.visionDirty ? "1" : "0") +
         ",ver=" + std::to_string(f.visionVersion) + "|";

    auto computeFastHash = [](const auto& vec) -> uint64_t
    {
        uint64_t h = 14695981039346656037ULL;
        for (const auto& val : vec)
        {
            h ^= static_cast<uint64_t>(val);
            h *= 1099511628211ULL;
        }
        return h;
    };
    uint64_t gridHash   = computeFastHash(f.mGrid.mData);
    uint64_t visionHash = computeFastHash(f.mVisionGrid.mData);
    for (const auto& tv : f.mTeamVisionGrid)
    {
        visionHash ^= computeFastHash(tv.mData);
        visionHash *= 1099511628211ULL;
    }
    s += "FieldGrids:grid=0x" + std::to_string(gridHash) + ",vis=0x" + std::to_string(visionHash) + "|";

    const auto& col = mCollision;
    s += "Col:sp=" + std::to_string(col.spacing) + ",w=" + std::to_string(col.map.width()) +
         ",h=" + std::to_string(col.map.height()) + "|";

    uint64_t colHash        = 14695981039346656037ULL;
    int      colEntityCount = 0;
    for (const auto& chunk : col.map.mGrid.mData)
    {
        colEntityCount += chunk.vecIdEntity.size();
        for (int id : chunk.vecIdEntity)
        {
            colHash ^= static_cast<uint64_t>(id);
            colHash *= 1099511628211ULL;
        }
    }
    s += "ColChunks:ent=" + std::to_string(colEntityCount) + ",hash=0x" + std::to_string(colHash) + "|";

    const auto& pool = mAdmin.mPool.mPool;

    auto appendSparseSet = [&]<typename T>(const char* tag, auto dumpFunc)
    {
        const auto& set = std::get<SparseSet<T>>(pool);
        s += tag;
        s += "(" + std::to_string(set.dense.size()) + "):";
        for (size_t i = 0; i < set.dense.size(); i++)
        {
            s += "[" + std::to_string(set.denseIDs[i]) + ":";
            dumpFunc(set.dense[i]);
            s += "]";
        }
        s += "|";
    };

    appendSparseSet.operator()<CPosition>("CPos",
                                          [&](const CPosition& p)
                                          {
                                              s += std::to_string(p.pos.x) + "," + std::to_string(p.pos.y) + "," +
                                                   std::to_string(p.posInit.x) + "," + std::to_string(p.posInit.y) +
                                                   "," + std::to_string(p.prevPos.x) + "," + std::to_string(p.prevPos.y);
                                          });

    appendSparseSet.operator()<CVelocity>("CVel",
                                          [&](const CVelocity& v)
                                          {
                                              s += std::to_string(v.vel.x) + "," + std::to_string(v.vel.y) + "," +
                                                   std::to_string(v.pushDirection.x) + "," +
                                                   std::to_string(v.pushDirection.y) + "," +
                                                   std::to_string(v.desiredDirection.x) + "," +
                                                   std::to_string(v.desiredDirection.y) + "," +
                                                   std::to_string(v.separationForce.x) + "," +
                                                   std::to_string(v.separationForce.y);
                                          });

    appendSparseSet
        .operator()<CUnit>("CUnit",
                           [&](const CUnit& u)
                           {
                               s += std::to_string(static_cast<int>(u.unitClass)) + "," +
                                    std::to_string(u.attackRange) + "," + std::to_string(u.engageRange) + "," +
                                    std::to_string(u.disengageRange) + "," + std::to_string(u.moveSpeed) + "," +
                                    std::to_string(u.attackCooldown) + "," + std::to_string(u.damage) + "," +
                                    std::to_string(static_cast<int>(u.state)) + "," + std::to_string(u.targetId) + "," +
                                    std::to_string(u.searchTimer) + "," + std::to_string(u.combatTimer) + "," +
                                    std::to_string(u.attackTimer) + "," + std::to_string(u.retreatTimer) + "," +
                                    std::to_string(u.retreatHealTimer) + "," + std::to_string(u.retreatSafeDistance) +
                                    "," + std::to_string(u.retreatMoveSpeed) + "," + std::to_string(u.explodeTimer) +
                                    "," + std::to_string(static_cast<int>(u.stance)) + "," + (u.isSelected ? "1" : "0") +
                                    "," + std::to_string(u.buff.damageBonus) + "," + std::to_string(u.buff.speedBonus) +
                                    "," + std::to_string(u.buff.cooldownBonus) + "," + std::to_string(u.buff.shieldHp);
                           });

    appendSparseSet.operator()<CPlayer>("CPlayer",
                                        [&](const CPlayer& p)
                                        {
                                            s += std::to_string(p.skill1Timer) + "," + std::to_string(p.skill2Timer) +
                                                 "," + std::to_string(p.skill3Timer) + "," +
                                                 std::to_string(p.attackTimer) + "," + std::to_string(p.respawnTimer);
                                        });

    appendSparseSet.operator()<CProjectile>("CProj",
                                            [&](const CProjectile& pr)
                                            {
                                                s += std::to_string(pr.ownerId) + "," + std::to_string(pr.targetId) +
                                                     "," + std::to_string(pr.lifeTime) + "," +
                                                     std::to_string(pr.damage) + "," + std::to_string(pr.hitRadius) +
                                                     "," + std::to_string(pr.renderSize);
                                            });

    appendSparseSet.operator()<CPickup>("CPickup",
                                        [&](const CPickup& pk)
                                        {
                                            s += std::to_string(static_cast<int>(pk.kind)) + "," +
                                                 std::to_string(pk.value) + "," + std::to_string(pk.lifeTime) + "," +
                                                 std::to_string(pk.radius);
                                        });

    appendSparseSet.operator()<CHealth>("CHp",
                                        [&](const CHealth& h)
                                        {
                                            s += std::to_string(h.hp) + "," + std::to_string(h.maxHp) + "," +
                                                 std::to_string(h.armor);
                                        });

    appendSparseSet.operator()<CRectDragDrop>("CDrag",
                                              [&](const CRectDragDrop& dd)
                                              {
                                                  s += std::to_string(dd.pos.x) + "," + std::to_string(dd.pos.y) + "," +
                                                       std::to_string(dd.size.x) + "," + std::to_string(dd.size.y);
                                              });

    appendSparseSet.operator()<CWall>("CWall",
                                      [&](const CWall& w)
                                      {
                                          s += std::to_string(static_cast<int>(w.kind)) + "," +
                                               (w.playerPlaced ? "1" : "0") + "," + std::to_string(w.effectRange) +
                                               "," + std::to_string(w.cooldownTick) + "," +
                                               std::to_string(w.cooldownTimer) + "," + std::to_string(w.damage) + "," +
                                               std::to_string(w.gunAngle) + "," + std::to_string(w.spinAngle);
                                      });

    appendSparseSet.operator()<CBoundCircle>("CCircle", [&](const CBoundCircle& bc) { s += std::to_string(bc.radius); });

    appendSparseSet.operator()<CVisualEffect>("CEffect",
                                              [&](const CVisualEffect& ve)
                                              {
                                                  s += std::to_string(static_cast<int>(ve.kind)) + "," +
                                                       std::to_string(ve.startPos.x) + "," + std::to_string(ve.startPos.y) +
                                                       "," + std::to_string(ve.endPos.x) + "," +
                                                       std::to_string(ve.endPos.y) + "," + std::to_string(ve.lifeTime) +
                                                       "," + std::to_string(ve.initialLifeTime) + "," +
                                                       std::to_string(ve.teamId);
                                              });

    appendSparseSet.operator()<CBase>("CBase",
                                      [&](const CBase& b)
                                      {
                                          s += std::to_string(b.attackRange) + "," + std::to_string(b.defenseRange) +
                                               "," + std::to_string(b.attackCooldown) + "," +
                                               std::to_string(b.attackTimer) + "," + std::to_string(b.damage);
                                      });

    appendSparseSet.operator()<CTeam>("CTeam",
                                      [&](const CTeam& t)
                                      { s += std::to_string(t.teamId) + "," + std::to_string(t.playerId); });

    auto appendTagSet = [&]<typename T>(const char* tag)
    {
        const auto& set = std::get<SparseSet<T>>(pool);
        s += tag;
        s += "(" + std::to_string(set.denseIDs.size()) + "):[";
        for (size_t i = 0; i < set.denseIDs.size(); i++)
        {
            s += std::to_string(set.denseIDs[i]) + ",";
        }
        s += "]|";
    };


    const auto& memPool         = mAdmin.mPool;
    size_t      activeMaskCount = 0;
    for (int id = 0; id < static_cast<int>(memPool.mMasks.size()); id++)
    {
        if (memPool.has(id))
        {
            activeMaskCount++;
        }
    }
    s += "Masks(" + std::to_string(activeMaskCount) + "):[";
    for (int id = 0; id < static_cast<int>(memPool.mMasks.size()); id++)
    {
        if (memPool.has(id))
        {
            s += std::to_string(id) + "@";
            for (size_t w : memPool.mMasks[id])
            {
                s += std::to_string(w) + ",";
            }
        }
    }
    s += "]|";

    return s;
}

void GameState::playSound(const std::string&              soundName,
                          const std::optional<f32::Vec2>& worldPos,
                          std::optional<f32::Fix>         customImportance)
{
    mEvents.emplace_back(EventSound{.soundName = soundName, .worldPos = worldPos, .customImportance = customImportance});
}
