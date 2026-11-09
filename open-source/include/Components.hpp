#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "FloatFixed.hpp"
#include "StateMachine.hpp"

#include <SFML/Window/Keyboard.hpp>

#include <SFML/System/Vector2.hpp>

#include <array>
#include <concepts>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>
#include <vector>

struct Component
{
};

struct ComponentParent : Component
{
};

struct CPosition : ComponentParent
{
    f32::Vec2 pos;
    f32::Vec2 posInit;
    f32::Vec2 prevPos;
};

struct CVelocity : ComponentParent
{
    f32::Vec2 vel;
    f32::Vec2 pushDirection;
    f32::Vec2 desiredDirection;
    f32::Vec2 separationForce;
};

struct CBoundCircle : ComponentParent
{
    f32::Fix radius = 0;
};

struct CHealth : ComponentParent
{
    f32::Fix hp    = 5;
    f32::Fix maxHp = 5;
    f32::Fix armor = 0;
};

struct CTeam : ComponentParent
{
    int teamId   = 0;
    int playerId = -1;
};

struct CRectDragDrop : ComponentParent
{
    f32::Vec2 pos{0, 0};
    f32::Vec2 size{0, 0};
};

struct ComponentChild : Component
{
};

enum class UnitClass
{
    Warrior,
    Archer,
    Tank,
    Grunt,
    Shooter,
    Berserker
};

enum class CombatStance
{
    Attack,
    Tactical,
};

struct UnitTypeConfig
{
    int maxHp       = 20;
    int armor       = 0;
    int damageBonus = 0;
    int hpBonus     = 0;
};

struct CUnitBuff
{
    f32::Fix damageBonus   = 0;
    f32::Fix speedBonus    = 0;
    int      cooldownBonus = 0;
    f32::Fix shieldHp      = 0;
};

struct CUnit : ComponentChild
{
    UnitClass unitClass      = UnitClass::Warrior;
    f32::Fix  attackRange    = 26;
    f32::Fix  engageRange    = 36;
    f32::Fix  disengageRange = 64;
    f32::Fix  moveSpeed      = f32::Fix(65) / 100;
    int       attackCooldown = 18;
    f32::Fix  damage         = 1;

    CUnitBuff buff{};

    UnitStateKind state = UnitStateKind::Search;

    static constexpr int kSearchInterval = 10;
    int                  searchTimer     = 0;

    static constexpr int kCombatInterval = 10;
    int                  combatTimer     = kCombatInterval;
    int                  targetId        = -1;
    int                  attackTimer     = 0;

    int      retreatTimer        = 110;
    int      retreatHealTimer    = 40;
    f32::Fix retreatSafeDistance = 140;
    f32::Fix retreatMoveSpeed    = f32::Fix(115) / 100;

    int explodeTimer = 10;

    CombatStance stance = CombatStance::Attack;

    bool isSelected = false;
};

struct CPlayer : ComponentChild
{
    int skill1Cooldown = 12;
    int skill1Timer    = 0;
    int skill2Cooldown = 60;
    int skill2Timer    = 0;
    int skill3Cooldown = 18;
    int skill3Timer    = 0;
    int attackCooldown = 15;
    int attackTimer    = 0;
    int showRangeTimer = 0;
    int respawnTimer   = 0;

    f32::Fix attackRange = 200;
    f32::Fix moveSpeed   = f32::Fix(3) / 2;
    f32::Fix damage      = 2;

    CUnitBuff buff{};
};

enum class WallKind
{
    Defensive,
    Electric,
    Fire,
    Gun,
    Radar,
    DesignatedPoint,
    Rock,
};

struct CWall : ComponentChild
{
    WallKind kind          = WallKind::Defensive;
    bool     playerPlaced  = false;
    f32::Fix effectRange   = 0;
    int      cooldownTick  = 0;
    int      cooldownTimer = 0;
    int      damage        = 0;
    f32::Fix gunAngle      = 0;
    f32::Fix spinAngle     = 0;
};

struct CBase : ComponentChild
{
    f32::Fix attackRange    = 250;
    f32::Fix defenseRange   = 300;
    int      attackCooldown = 30;
    int      attackTimer    = 0;
    f32::Fix damage         = 1;
};

struct CProjectile : ComponentChild
{
    int ownerId  = -1;
    int targetId = -1;
    int lifeTime = 75;

    f32::Fix damage     = 1;
    f32::Fix hitRadius  = 12;
    f32::Fix renderSize = 12;
};

enum class PowerupKind
{
    HealthBoost,
    DamageBoost,
    SpeedBoost,
    AttackRateBoost,
    ShieldBoost,
};

struct CPickup : ComponentChild
{
    PowerupKind kind     = PowerupKind::HealthBoost;
    int         value    = 1;
    int         lifeTime = 900;
    f32::Fix    radius   = 32;
};

enum class ZoneKind
{
    Bush,
    Tree,
    Water,
    Quicksand,
    Lava,
};

struct CEnvironmentZone : ComponentChild
{
    ZoneKind  kind = ZoneKind::Bush;
    f32::Vec2 pos{0, 0};
    f32::Vec2 size{0, 0};
    int       level = 1;
};

enum class EffectKind
{
    ElectricBeam,
    FireBeam,
    LaserBeam,
    DeathExplosion,
    CollectPulse,
    Shockwave,
    RadarScan,
    Heal,
    ClickIndicator,
    SlashArc,
};

struct CVisualEffect : ComponentChild
{
    EffectKind kind{};
    f32::Vec2  startPos;
    f32::Vec2  endPos;
    int        lifeTime        = 0;
    int        initialLifeTime = 0;
    int        teamId          = 0;
};

template <typename T>
concept IsComponentParent = std::is_base_of_v<ComponentParent, T>;

template <typename T>
concept IsComponentChild = std::is_base_of_v<ComponentChild, T>;

enum class InputMode
{
    Select,
    DragDrop
};

enum class PlacementPalette
{
    Wall,
    Unit,
};

struct CInput : Component
{
    InputMode inputMode = InputMode::DragDrop;

    struct BuildOptions
    {
        bool             placementMode     = false;
        PlacementPalette palette           = PlacementPalette::Wall;
        WallKind         selectedKind      = WallKind::Defensive;
        UnitClass        selectedUnitClass = UnitClass::Warrior;
    } build;

    std::set<sf::Keyboard::Key> keysPressed;

    sf::Vector2f mouseWorldPos{0.f, 0.f};
    sf::Vector2i mousePixelPos{0, 0};
    sf::Vector2i prevMousePixelPos{0, 0};
    bool         leftMousePressed       = false;
    bool         leftMouseJustPressed   = false;
    bool         leftMouseJustReleased  = false;
    bool         rightMousePressed      = false;
    bool         rightMouseJustPressed  = false;
    bool         rightMouseJustReleased = false;

    bool         isCameraDragging  = false;
    bool         isMinimapDragging = false;
    sf::Vector2f cameraDragDelta   = {0.f, 0.f};

    float zoomDelta      = 0.f;
    float pinchZoomDelta = 0.f;

    int   dragSelectedId = -1;
    float dragOffsetX    = 0.f;
    float dragOffsetY    = 0.f;

    bool         isSelecting = false;
    sf::Vector2f selectionStart{0.f, 0.f};
    sf::Vector2f selectionEnd{0.f, 0.f};

    CombatStance combatModeState = CombatStance::Attack;

    bool requestMenuScene = false;

    sf::Vector2f joystickDir = {0.f, 0.f};

    std::optional<float>        lastPinchDist;
    std::optional<sf::Vector2i> lastPanTouch;
    std::optional<sf::Vector2i> touch0Pos;
    std::optional<sf::Vector2i> touch1Pos;
    std::optional<sf::Vector2i> touchStartPos;
    int                         activeWorldFingerId = -1;

    [[nodiscard]] bool isKeyPressed(sf::Keyboard::Key key) const
    {
        return keysPressed.contains(key);
    }

    void clearJustPressedReleased()
    {
        leftMouseJustPressed   = false;
        leftMouseJustReleased  = false;
        rightMouseJustPressed  = false;
        rightMouseJustReleased = false;
    }
};

#endif
