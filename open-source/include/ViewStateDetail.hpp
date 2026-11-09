#ifndef VIEWSTATEDETAIL_HPP
#define VIEWSTATEDETAIL_HPP

#include "Components.hpp"
#include "GameConfig.hpp"

#include <SFML/Graphics.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include <cmath>

class GameState;
struct Game;
struct MemoryPool;
struct DeterministicRandom;
class ViewState;

namespace ViewStateDetail
{
using namespace GameConfig::Visual;

struct Camera3D
{
    sf::Vector2f center{0.f, 0.f};
    sf::Vector2f size{1280.f, 768.f};
    float        cosPitch    = 0.68f;
    float        sinPitch    = 0.46f;
    float        heightScale = 1.28f;
    float        heightPersp = 0.50f;
    float        focalDist   = 1800.f;

    void setup(const sf::View& worldView);

    [[nodiscard]] float getScale(float worldX, float worldY, float worldZ = 0.f) const;

    [[nodiscard]] sf::Vector2f project(float worldX, float worldY, float worldZ = 0.f) const;
    [[nodiscard]] sf::Vector2f project(sf::Vector2f groundPos, float worldZ = 0.f) const;
    [[nodiscard]] sf::Vector2f project(sf::Vector3f pos3d) const;

    [[nodiscard]] sf::Vector2f unprojectToGround(sf::Vector2f viewPos) const;

    [[nodiscard]] sf::FloatRect getGroundFrustumBounds(float margin = 180.f) const;
};

sf::Color shadeColor(sf::Color c, float brightness, float highlight = 0.f);

sf::Color computeLitColor(sf::Color baseColor, float nx, float ny, float nz);

struct PlayerPalette
{
    sf::Color fill;
    sf::Color innerOutline;
    sf::Color outerPulse;
    sf::Color projOuter;
    sf::Color projInner;
};

PlayerPalette getPlayerPalette(int clientId);

sf::Color getUnitClassColor(UnitClass uc);

sf::Color getWallColor(const CWall* wallComponent, bool isSelected);

sf::Color getStateColor(UnitStateKind state);

void drawArrow(ViewState*       viewState,
               sf::Vector2f     start,
               sf::Vector2f     end,
               sf::Color        color,
               float            thickness,
               sf::VertexArray& lines,
               sf::VertexArray& triangles);

std::string buildUnitLabel(MemoryPool& pool, int entityId);

void drawWorldDebug(ViewState* viewState, const GameState* gameState);

void drawStructureRange3D(Game*            game,
                          const Camera3D&  cam,
                          sf::Vector2f     center,
                          float            range,
                          sf::Color        baseColor,
                          bool             isSelected,
                          int              currentFrame,
                          sf::VertexArray& groundTriBatch);

void drawBase(Game*                game,
              const Camera3D&      cam,
              const CRectDragDrop& baseRect,
              const CBase*         baseComponent,
              const CHealth*       healthComponent,
              bool                 isSelected,
              int                  currentFrame,
              sf::VertexArray&     triBatch,
              sf::VertexArray&     lineBatch,
              sf::VertexArray*     hudBatch   = nullptr,
              bool                 isFriendly = true);

void drawWall(Game*                game,
              const Camera3D&      cam,
              const CRectDragDrop& wallRect,
              const CWall*         wallComponent,
              bool                 isSelected,
              int                  currentFrame,
              DeterministicRandom& random,
              sf::VertexArray&     triBatch,
              sf::VertexArray&     lineBatch,
              int                  level = 1);

void drawRock3D(sf::VertexArray& triBatch, const Camera3D& cam, const CRectDragDrop& wallRect, int level, int frame);

void drawBush3D(sf::VertexArray& triBatch, const Camera3D& cam, sf::Vector2f pos, sf::Vector2f size, int level, int frame);

void drawHazardPool3D(sf::VertexArray& triBatch,
                      const Camera3D&  cam,
                      sf::Vector2f     pos,
                      sf::Vector2f     size,
                      ZoneKind         kind,
                      int              level,
                      int              frame);

void drawTree3D(sf::VertexArray& triBatch, const Camera3D& cam, sf::Vector2f pos, sf::Vector2f size, int level, int frame);

void drawRoads3D(sf::VertexArray& triBatch, const Camera3D& cam, const std::vector<GameConfig::RoadSegment>& roads, int level);

struct UtilityPole
{
    sf::Vector2f pos;
    sf::Vector2f dir;
    sf::Vector2f perp;
    sf::Vector2f armDir;
    int          roadIndex;
    int          indexInRoad;
    int          totalInRoad;
    bool         hasTransformer;
};

const std::vector<UtilityPole>& getLevelUtilityPoles(int level);

void drawStreetlightGlow3D(sf::VertexArray& triBatch, const Camera3D& cam, const std::vector<UtilityPole>& poles);

void drawSinglePole3D(sf::VertexArray& triBatch, const Camera3D& cam, const UtilityPole& pole, int level, int frame);

void drawPowerLines3D(sf::VertexArray&                triBatch,
                      const Camera3D&                 cam,
                      const std::vector<UtilityPole>& poles,
                      int                             level,
                      int                             currentFrame = 0);

} // namespace ViewStateDetail

#endif
