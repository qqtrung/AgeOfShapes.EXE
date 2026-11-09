#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include "Components.hpp"
#include "DeterministicRandom.hpp"
#include "EntityManager.hpp"
#include "FloatFixed.hpp"
#include "GameEvent.hpp"
#include "Packet.hpp"
#include "SystemGameState.hpp"
#include "SystemManager.hpp"

#include <map>
#include <queue>
#include <vector>

enum class GameMode
{
    Coop,
    PvP
};

class GameState
{
public:
    using GameMode = ::GameMode;

    using SimulationPack = SystemManager<SystemGameState::SystemInit,
                                         SystemGameState::SystemCollision,
                                         SystemGameState::SystemNavigationField,
                                         SystemGameState::SystemMatchEnd,
                                         SystemGameState::SystemMovement,
                                         SystemGameState::SystemUnit,
                                         SystemGameState::SystemPlayer,
                                         SystemGameState::SystemWall,
                                         SystemGameState::SystemBase,
                                         SystemGameState::SystemProjectile,
                                         SystemGameState::SystemPickup,
                                         SystemGameState::SystemVisualEffect>;

    size_t shootClock{};
    size_t hitClock{};
    size_t elecClock{};
    size_t fireClock{};
    size_t healSoundClock{};

    GameMode mGameMode    = GameMode::Coop;
    int      mWinningTeam = -1;

    bool isReplay        = false;
    bool mDesyncDetected = false;
    int  mId             = -1;
    int  mPlayerCount    = 1;

    std::vector<GameEvent>         mEvents;
    std::map<std::string, int64_t> mProfilerTimes;

    uint64_t            mTick = 0;
    DeterministicRandom mUpdateRandom{12345};
    f32::Fix            mTimeScale = 1;

    EntityManager  mAdmin;
    SimulationPack mSystems;

    CampaignState mCampaign;
    f32::Fix      mMapWidth     = 2992;
    f32::Fix      mMapHeight    = 2000;
    int           maxTeam0Units = 12;
    int           maxTeam1Units = 12;

    NavData       mNav;
    CollisionData mCollision;

    template <typename T, typename... Args>
    T* alloc(Args&&... args)
    {
        return mAdmin.alloc<T>(std::forward<Args>(args)...);
    }

    template <typename... Comps>
    ComponentBundle<Comps...>* createBundle(Comps*... comps)
    {
        return mAdmin.createBundle(comps...);
    }

    GameState();

    [[nodiscard]] const std::vector<GameEvent>& events() const
    {
        return mEvents;
    }
    void clearEvents()
    {
        mEvents.clear();
    }

    [[nodiscard]] const std::map<std::string, int64_t>& profilerTimes() const
    {
        return mProfilerTimes;
    }

    bool update(const CommandOnTick& commandOnTick);
    void execute(const Command& command);

    [[nodiscard]] int      getPlayerTeam(int playerId) const;
    [[nodiscard]] GameMode getGameMode() const
    {
        return mGameMode;
    }
    [[nodiscard]] GameMode gameMode() const
    {
        return mGameMode;
    }
    void setGameMode(GameMode mode)
    {
        mGameMode = mode;
    }
    [[nodiscard]] int winningTeam() const
    {
        return mWinningTeam;
    }
    void setWinningTeam(int teamId)
    {
        mWinningTeam = teamId;
    }

    [[nodiscard]] std::string dumpState() const;

    [[nodiscard]] f32::Fix timeScale() const
    {
        return mTimeScale;
    }
    [[nodiscard]] f32::Fix& timeScale()
    {
        return mTimeScale;
    }
    [[nodiscard]] bool isGameOver() const
    {
        return campaign().gameOver;
    }

    [[nodiscard]] EntityManager& admin()
    {
        return mAdmin;
    }
    [[nodiscard]] const EntityManager& admin() const
    {
        return mAdmin;
    }

    [[nodiscard]] CampaignState& campaign()
    {
        return mCampaign;
    }
    [[nodiscard]] const CampaignState& campaign() const
    {
        return mCampaign;
    }
    [[nodiscard]] f32::Fix& mapWidth()
    {
        return mMapWidth;
    }
    [[nodiscard]] const f32::Fix& mapWidth() const
    {
        return mMapWidth;
    }
    [[nodiscard]] f32::Fix& mapHeight()
    {
        return mMapHeight;
    }
    [[nodiscard]] const f32::Fix& mapHeight() const
    {
        return mMapHeight;
    }
    [[nodiscard]] NavData& nav()
    {
        return mNav;
    }
    [[nodiscard]] const NavData& nav() const
    {
        return mNav;
    }
    [[nodiscard]] CollisionData& collision()
    {
        return mCollision;
    }
    [[nodiscard]] const CollisionData& collision() const
    {
        return mCollision;
    }

    void playSound(const std::string&              soundName,
                   const std::optional<f32::Vec2>& worldPos         = std::nullopt,
                   std::optional<f32::Fix>         customImportance = std::nullopt);
};

#endif
