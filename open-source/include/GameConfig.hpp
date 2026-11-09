#ifndef GAMECONFIG_HPP
#define GAMECONFIG_HPP

#include "Components.hpp"

#include <SFML/Graphics.hpp>

#include <SFML/System/Vector2.hpp>

#include <array>
#include <vector>

namespace GameConfig
{
namespace AI
{
constexpr f32::Fix kSearchMaxSpeed      = 1;
constexpr f32::Fix kRetreatMaxSpeed     = 2;
constexpr f32::Fix kSearchSteerWeight   = f32::Fix(0.1f);
constexpr f32::Fix kCombatSteerWeight   = f32::Fix(0.18f);
constexpr f32::Fix kRetreatSteerWeight  = f32::Fix(0.22f);
constexpr f32::Fix kPushDecay           = f32::Fix(0.35f);
constexpr f32::Fix kImpactImpulse       = f32::Fix(0.45f);
constexpr f32::Fix kSeparationWeight    = f32::Fix(2.2f);
constexpr f32::Fix kLowHealthRatio      = f32::Fix(0.34f);
constexpr f32::Fix kAcquireRange        = 200;
constexpr f32::Fix kProjectileSpeed     = 6;
constexpr int      kRetreatDurationTick = 110;
constexpr int      kPickupValue         = 1;
} // namespace AI

namespace Game
{
constexpr uint64_t kTickUs = 16667;

constexpr uint64_t kCommandDelayTicks = 10;
constexpr size_t   kTargetBufferDepth = 4;

constexpr uint64_t kNetSyncIntervalUs = kTickUs * 2;

} // namespace Game

namespace Network
{
constexpr const char* kDefaultHost     = "geometrylegion.ddns.net";
constexpr uint16_t    kDefaultPort     = 7777;
constexpr uint64_t    kSimulatedPingMs = 0;
} // namespace Network

namespace UI
{
constexpr float kWidth  = 2532.f;
constexpr float kHeight = 1170.f;
} // namespace UI


struct DifficultyConfig
{
    int team0UnitCount;
    int team1UnitCount;
    int requiredSupplies;
    int unitDeathLimit;

    std::array<UnitTypeConfig, 6> unitConfigs;

    int defensiveWallStock;
    int electricWallStock;
    int fireWallStock;
    int gunWallStock;
    int radarWallStock;
    int designatedPointStock;
    int warriorUnitStock;
    int archerUnitStock;
    int tankUnitStock;
    int gruntUnitStock;
    int shooterUnitStock;
    int berserkerUnitStock;
};

constexpr int                                               kDifficultyCount   = 3;
inline const std::array<DifficultyConfig, kDifficultyCount> kDifficultyConfigs = {{
    {.team0UnitCount       = 3,
     .team1UnitCount       = 12,
     .requiredSupplies     = 3,
     .unitDeathLimit       = 3,
     .unitConfigs          = {{
         {.maxHp = 18, .armor = 0, .damageBonus = 0, .hpBonus = 2},
         {.maxHp = 9, .armor = 0, .damageBonus = 0, .hpBonus = 1},
         {.maxHp = 18, .armor = 5, .damageBonus = 0, .hpBonus = 2},
         {.maxHp = 26, .armor = 6, .damageBonus = 1, .hpBonus = -2},
         {.maxHp = 13, .armor = 6, .damageBonus = 1, .hpBonus = -1},
         {.maxHp = 19, .armor = 6, .damageBonus = 1, .hpBonus = -2},
     }},
     .defensiveWallStock   = 1,
     .electricWallStock    = 1,
     .fireWallStock        = 1,
     .gunWallStock         = 1,
     .radarWallStock       = 1,
     .designatedPointStock = 1,
     .warriorUnitStock     = 2,
     .archerUnitStock      = 2,
     .tankUnitStock        = 1,
     .gruntUnitStock       = 1,
     .shooterUnitStock     = 1,
     .berserkerUnitStock   = 1},
    {.team0UnitCount       = 4,
     .team1UnitCount       = 16,
     .requiredSupplies     = 4,
     .unitDeathLimit       = 3,
     .unitConfigs          = {{
         {.maxHp = 20, .armor = 0, .damageBonus = 0, .hpBonus = 0},
         {.maxHp = 10, .armor = 0, .damageBonus = 0, .hpBonus = 0},
         {.maxHp = 20, .armor = 5, .damageBonus = 0, .hpBonus = 0},
         {.maxHp = 30, .armor = 8, .damageBonus = 2, .hpBonus = 0},
         {.maxHp = 15, .armor = 8, .damageBonus = 2, .hpBonus = 0},
         {.maxHp = 22, .armor = 8, .damageBonus = 2, .hpBonus = 0},
     }},
     .defensiveWallStock   = 2,
     .electricWallStock    = 2,
     .fireWallStock        = 2,
     .gunWallStock         = 2,
     .radarWallStock       = 2,
     .designatedPointStock = 2,
     .warriorUnitStock     = 3,
     .archerUnitStock      = 3,
     .tankUnitStock        = 2,
     .gruntUnitStock       = 2,
     .shooterUnitStock     = 2,
     .berserkerUnitStock   = 2},
    {.team0UnitCount       = 5,
     .team1UnitCount       = 20,
     .requiredSupplies     = 5,
     .unitDeathLimit       = 3,
     .unitConfigs          = {{
         {.maxHp = 22, .armor = 0, .damageBonus = 0, .hpBonus = -2},
         {.maxHp = 11, .armor = 0, .damageBonus = 0, .hpBonus = -1},
         {.maxHp = 22, .armor = 5, .damageBonus = 0, .hpBonus = -2},
         {.maxHp = 34, .armor = 10, .damageBonus = 3, .hpBonus = 4},
         {.maxHp = 17, .armor = 10, .damageBonus = 3, .hpBonus = 2},
         {.maxHp = 25, .armor = 10, .damageBonus = 3, .hpBonus = 3},
     }},
     .defensiveWallStock   = 3,
     .electricWallStock    = 3,
     .fireWallStock        = 3,
     .gunWallStock         = 3,
     .radarWallStock       = 3,
     .designatedPointStock = 3,
     .warriorUnitStock     = 4,
     .archerUnitStock      = 4,
     .tankUnitStock        = 3,
     .gruntUnitStock       = 3,
     .shooterUnitStock     = 3,
     .berserkerUnitStock   = 3},
}};

struct WallLayout
{
    f32::Vec2 pos;
    f32::Vec2 size;
    WallKind  kind;
    int       teamId = 0;
};

constexpr int                                                 kLevelCount = 3;
inline const std::array<std::vector<WallLayout>, kLevelCount> kLevelWalls = {{
    {
        {.pos = {380, 380}, .size = {110, 65}, .kind = WallKind::Defensive},
        {.pos = {580, 375}, .size = {90, 75}, .kind = WallKind::Electric},
        {.pos = {1050, 370}, .size = {120, 80}, .kind = WallKind::Radar},
        {.pos = {1820, 375}, .size = {115, 75}, .kind = WallKind::Fire},
        {.pos = {1980, 240}, .size = {80, 80}, .kind = WallKind::DesignatedPoint},
        {.pos = {2450, 380}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {420, 880}, .size = {170, 60}, .kind = WallKind::Defensive},
        {.pos = {1050, 870}, .size = {130, 80}, .kind = WallKind::Radar},
        {.pos = {1330, 880}, .size = {85, 70}, .kind = WallKind::Electric},
        {.pos = {1820, 880}, .size = {130, 65}, .kind = WallKind::Defensive},
        {.pos = {2480, 875}, .size = {80, 75}, .kind = WallKind::Gun},
        {.pos = {450, 1260}, .size = {60, 150}, .kind = WallKind::Defensive},
        {.pos = {1050, 1375}, .size = {115, 75}, .kind = WallKind::Fire},
        {.pos = {1780, 1370}, .size = {110, 80}, .kind = WallKind::Radar},
        {.pos = {2480, 1375}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {1120, 1580}, .size = {120, 60}, .kind = WallKind::Defensive},
    },
    {
        {.pos = {300, 750}, .size = {80, 80}, .kind = WallKind::Rock},
        {.pos = {1450, 320}, .size = {85, 85}, .kind = WallKind::Rock},
        {.pos = {1450, 1650}, .size = {85, 85}, .kind = WallKind::Rock},
        {.pos = {2150, 750}, .size = {85, 85}, .kind = WallKind::Rock},
        {.pos = {750, 1250}, .size = {110, 65}, .kind = WallKind::Defensive},
        {.pos = {280, 1180}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {1100, 820}, .size = {120, 80}, .kind = WallKind::Radar},
        {.pos = {1950, 620}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {1950, 1380}, .size = {115, 75}, .kind = WallKind::Fire},
        {.pos = {1480, 1150}, .size = {85, 70}, .kind = WallKind::Electric},
        {.pos = {1100, 1250}, .size = {80, 80}, .kind = WallKind::DesignatedPoint},
        {.pos = {2550, 820}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {2350, 1380}, .size = {110, 65}, .kind = WallKind::Defensive},
    },
    {
        {.pos = {380, 260}, .size = {120, 70}, .kind = WallKind::Defensive},
        {.pos = {570, 260}, .size = {110, 75}, .kind = WallKind::Radar},
        {.pos = {2420, 260}, .size = {120, 70}, .kind = WallKind::Defensive},
        {.pos = {2600, 260}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {950, 680}, .size = {90, 75}, .kind = WallKind::Electric},
        {.pos = {1250, 680}, .size = {120, 80}, .kind = WallKind::Radar},
        {.pos = {1750, 680}, .size = {80, 80}, .kind = WallKind::DesignatedPoint},
        {.pos = {1980, 680}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {950, 1260}, .size = {115, 75}, .kind = WallKind::Fire},
        {.pos = {1750, 1260}, .size = {90, 75}, .kind = WallKind::Electric},
        {.pos = {2000, 1260}, .size = {120, 70}, .kind = WallKind::Defensive},
        {.pos = {450, 1680}, .size = {85, 75}, .kind = WallKind::Gun},
        {.pos = {1100, 1680}, .size = {120, 70}, .kind = WallKind::Defensive},
        {.pos = {1800, 1680}, .size = {115, 75}, .kind = WallKind::Fire},
        {.pos = {2450, 1680}, .size = {85, 75}, .kind = WallKind::Gun},
    },
}};

inline const std::vector<f32::Vec2> kTeam0SpawnPositions = {
    {1400, 950},
    {1450, 950},
    {1500, 950},
    {1400, 1000},
    {1450, 1000},
    {1500, 1000},
    {1400, 1050},
    {1450, 1050},
    {1500, 1050},
    {1600, 950},
    {1650, 950},
    {1700, 950},
    {1600, 1000},
    {1650, 1000},
    {1700, 1000},
    {1600, 1050},
    {1650, 1050},
    {1700, 1050},
};

inline const std::vector<f32::Vec2> kTeam1SpawnPositions = {
    {200, 200},  {250, 200},  {300, 200},   {200, 250},   {250, 250},   {300, 250},   {2700, 200},  {2750, 200},
    {2800, 200}, {2700, 250}, {2750, 250},  {2800, 250},  {200, 1700},  {250, 1700},  {300, 1700},  {200, 1750},
    {250, 1750}, {300, 1750}, {2700, 1700}, {2750, 1700}, {2800, 1700}, {2700, 1750}, {2750, 1750}, {2800, 1750},
    {100, 900},  {100, 950},  {100, 1000},  {100, 1050},  {150, 900},   {150, 950},   {150, 1000},  {150, 1050},
    {2850, 900}, {2850, 950}, {2850, 1000}, {2850, 1050}, {2900, 900},  {2900, 950},  {2900, 1000}, {2900, 1050},
};

struct WallData
{
    f32::Fix effectRange;
    int      cooldownTick;
    int      damage;
};

inline const WallData kWallData[] = {
    {.effectRange = 0, .cooldownTick = 0, .damage = 0},
    {.effectRange = 110, .cooldownTick = 22, .damage = 1},
    {.effectRange = 95, .cooldownTick = 10, .damage = 1},
    {.effectRange = 230, .cooldownTick = 18, .damage = 1},
    {.effectRange = 250, .cooldownTick = 60, .damage = 2},
    {.effectRange = 0, .cooldownTick = 0, .damage = 0},
    {.effectRange = 0, .cooldownTick = 0, .damage = 0},
};

struct WallBuildData
{
    f32::Vec2 size;
};

inline const WallBuildData kWallBuildData[] = {
    {{75, 75}},
    {{70, 70}},
    {{80, 80}},
    {{90, 90}},
    {{60, 60}},
    {{85, 85}},
    {{80, 80}},
};

namespace Visual
{
constexpr float kDebugArrowScale = 18.f;

inline const sf::Color kFriendlyColor        = sf::Color(50, 160, 255);
inline const sf::Color kOpponentColor        = sf::Color(255, 60, 90);
inline const sf::Color kWallColor            = sf::Color(45, 125, 175);
inline const sf::Color kSelectedWallColor    = sf::Color(70, 180, 240, 200);
inline const sf::Color kElectricWallColor    = sf::Color(0, 200, 255);
inline const sf::Color kFireWallColor        = sf::Color(255, 90, 40);
inline const sf::Color kGunWallColor         = sf::Color(130, 140, 155);
inline const sf::Color kRadarWallColor       = sf::Color(40, 240, 120);
inline const sf::Color kDesignatedPointColor = sf::Color(255, 215, 0);

inline const sf::Color kUnitClassColors[] = {
    {50, 140, 255},
    {0, 230, 210},
    {160, 90, 255},
    {255, 60, 90},
    {255, 140, 30},
    {240, 40, 150},
};
} // namespace Visual

const DifficultyConfig&        getDifficultyConfig(int difficulty);
const std::vector<WallLayout>& getLevelWalls(int level);

struct EnvironmentZoneLayout
{
    f32::Vec2 pos;
    f32::Vec2 size;
    ZoneKind  kind;
};

inline const std::array<std::vector<EnvironmentZoneLayout>, kLevelCount> kLevelZones = {{
    {
        {.pos = {1180, 1120}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1340, 1260}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1200, 1020}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1380, 340}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1680, 340}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1990, 1360}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2290, 1360}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {640, 1360}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {940, 1360}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1580, 910}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {660, 580}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {830, 1590}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2340, 580}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2170, 1590}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {300, 370}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {600, 310}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {330, 870}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1970, 870}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2360, 370}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {360, 1300}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1260, 1570}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1690, 1370}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1240, 1025}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1310, 1025}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1400, 280}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {2010, 1310}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {935, 1310}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1525, 915}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {320, 420}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1750, 920}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1050, 1610}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {2410, 915}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1900, 280}, .size = {46, 40}, .kind = ZoneKind::Bush},
    },
    {
        {.pos = {340, 720}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {580, 720}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {650, 1180}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1100, 450}, .size = {130, 110}, .kind = ZoneKind::Quicksand},
        {.pos = {2200, 240}, .size = {140, 120}, .kind = ZoneKind::Quicksand},
        {.pos = {1100, 1550}, .size = {130, 110}, .kind = ZoneKind::Quicksand},
        {.pos = {2200, 1650}, .size = {140, 120}, .kind = ZoneKind::Quicksand},
        {.pos = {1800, 320}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2450, 1080}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1800, 1680}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {580, 890}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1550, 880}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1850, 1150}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1350, 1420}, .size = {46, 40}, .kind = ZoneKind::Bush},
    },
    {
        {.pos = {1850, 760}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1850, 810}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {400, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {600, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {950, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1150, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1750, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1950, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2400, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2600, 860}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {400, 560}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {950, 560}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1750, 560}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2400, 560}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {400, 1380}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {950, 1380}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {1750, 1380}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {2400, 1380}, .size = {75, 75}, .kind = ZoneKind::Tree},
        {.pos = {500, 875}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1050, 875}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {1850, 875}, .size = {46, 40}, .kind = ZoneKind::Bush},
        {.pos = {2500, 875}, .size = {46, 40}, .kind = ZoneKind::Bush},
    },
}};

const std::vector<EnvironmentZoneLayout>& getLevelZones(int level);

struct RoadSegment
{
    f32::Vec2 start;
    f32::Vec2 end;
    float     width = 56.f;
};

inline const std::array<std::vector<RoadSegment>, kLevelCount> kLevelRoads = {{
    {
        {{200, 1000}, {750, 1000}, 60.0f},   {{750, 1000}, {1500, 1000}, 60.0f},  {{1500, 1000}, {2250, 1000}, 60.0f},
        {{2250, 1000}, {2800, 1000}, 60.0f}, {{200, 500}, {750, 500}, 50.0f},     {{750, 500}, {1500, 500}, 50.0f},
        {{1500, 500}, {2250, 500}, 50.0f},   {{2250, 500}, {2800, 500}, 50.0f},   {{200, 1500}, {750, 1500}, 50.0f},
        {{750, 1500}, {1500, 1500}, 50.0f},  {{1500, 1500}, {2250, 1500}, 50.0f}, {{2250, 1500}, {2800, 1500}, 50.0f},
        {{1500, 150}, {1500, 500}, 60.0f},   {{1500, 500}, {1500, 1000}, 60.0f},  {{1500, 1000}, {1500, 1500}, 60.0f},
        {{1500, 1500}, {1500, 1850}, 60.0f}, {{750, 150}, {750, 500}, 50.0f},     {{750, 500}, {750, 1000}, 50.0f},
        {{750, 1000}, {750, 1500}, 50.0f},   {{750, 1500}, {750, 1850}, 50.0f},   {{2250, 150}, {2250, 500}, 50.0f},
        {{2250, 500}, {2250, 1000}, 50.0f},  {{2250, 1000}, {2250, 1500}, 50.0f}, {{2250, 1500}, {2250, 1850}, 50.0f},
    },
    {
        {{200, 1000}, {900, 1000}, 62.0f},
        {{900, 1000}, {1700, 1000}, 62.0f},
        {{1700, 1000}, {2400, 1000}, 62.0f},
        {{2400, 1000}, {2800, 1000}, 62.0f},
        {{1700, 1000}, {1700, 480}, 48.0f},
        {{1700, 480}, {2400, 480}, 48.0f},
        {{1700, 1000}, {1700, 1520}, 48.0f},
        {{1700, 1520}, {2400, 1520}, 48.0f},
    },
    {
        {{200, 480}, {750, 480}, 55.0f},     {{750, 480}, {1500, 480}, 55.0f},    {{1500, 480}, {2250, 480}, 55.0f},
        {{2250, 480}, {2800, 480}, 55.0f},   {{200, 1000}, {750, 1000}, 68.0f},   {{750, 1000}, {1500, 1000}, 68.0f},
        {{1500, 1000}, {2250, 1000}, 68.0f}, {{2250, 1000}, {2800, 1000}, 68.0f}, {{200, 1520}, {750, 1520}, 55.0f},
        {{750, 1520}, {1500, 1520}, 55.0f},  {{1500, 1520}, {2250, 1520}, 55.0f}, {{2250, 1520}, {2800, 1520}, 55.0f},
        {{750, 150}, {750, 480}, 55.0f},     {{750, 480}, {750, 1000}, 55.0f},    {{750, 1000}, {750, 1520}, 55.0f},
        {{750, 1520}, {750, 1850}, 55.0f},   {{1500, 150}, {1500, 480}, 68.0f},   {{1500, 480}, {1500, 1000}, 68.0f},
        {{1500, 1000}, {1500, 1520}, 68.0f}, {{1500, 1520}, {1500, 1850}, 68.0f}, {{2250, 150}, {2250, 480}, 55.0f},
        {{2250, 480}, {2250, 1000}, 55.0f},  {{2250, 1000}, {2250, 1520}, 55.0f}, {{2250, 1520}, {2250, 1850}, 55.0f},
    },
}};

const std::vector<RoadSegment>& getLevelRoads(int level);

constexpr int                             kMapCount = 3;
const std::vector<WallLayout>&            getMapWalls(int map);
const std::vector<EnvironmentZoneLayout>& getMapZones(int map);
const std::vector<RoadSegment>&           getMapRoads(int map);

struct BaseSpawnConfig
{
    f32::Vec2 pos;
    f32::Vec2 size;
    f32::Fix  hp;
    f32::Fix  armor;
    f32::Fix  attackRange;
    f32::Fix  defenseRange;
    int       attackCooldown;
    f32::Fix  damage;
    int       teamId;
};

struct MapLayoutConfig
{
    std::vector<BaseSpawnConfig>       bases;
    std::vector<WallLayout>            walls;
    std::vector<EnvironmentZoneLayout> zones;
    std::vector<f32::Vec2>             pickups;
};

namespace Detail
{
MapLayoutConfig createPvP2PMapLayout(int mapIdx);
MapLayoutConfig createPvP3PMapLayout(int mapIdx);
} // namespace Detail

const MapLayoutConfig& getPvP2PMapLayout(int mapIdx);
const MapLayoutConfig& getPvP3PMapLayout(int mapIdx);

constexpr int kWaveCount = 20;

int getCoopMaxWaves(int difficulty);
int getPvPBaseUnitCount(int difficulty);

struct WaveConfig
{
    int delaySeconds   = 10;
    int gruntCount     = 3;
    int shooterCount   = 0;
    int tankCount      = 0;
    int berserkerCount = 0;
};

struct LevelWaveConfig
{
    std::array<WaveConfig, kWaveCount> waves;
    int                                finalSurvivalSeconds = 35;
};

const LevelWaveConfig& getLevelWaveConfig(int level);

} // namespace GameConfig

#endif
