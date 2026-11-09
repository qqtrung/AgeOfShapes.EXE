#include "ViewStateDetail.hpp"

#include "ClientGame.hpp"
#include "Components.hpp"
#include "DeterministicRandom.hpp"
#include "FloatFixed.hpp"
#include "Game.hpp"
#include "GameConfig.hpp"
#include "GameState.hpp"
#include "GameStateDetail.hpp"
#include "Profiler.hpp"
#include "SystemGameState.hpp"
#include "SystemViewState.hpp"
#include "ViewState.hpp"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <numbers>
#include <string>
#include <unordered_map>

#include <cmath>
#include <cstdio>

namespace ViewStateDetail
{

void Camera3D::setup(const sf::View& worldView)
{
    center    = worldView.getCenter();
    size      = worldView.getSize();
    focalDist = std::max(500.f, size.y * 1.90f);
}

float Camera3D::getScale(float worldX, float worldY, float worldZ) const
{
    (void)worldX;
    const float dy    = worldY - center.y;
    const float denom = std::max(focalDist * 0.25f, focalDist - dy * sinPitch - worldZ * heightPersp);
    return focalDist / denom;
}

sf::Vector2f Camera3D::project(float worldX, float worldY, float worldZ) const
{
    const float dx    = worldX - center.x;
    const float dy    = worldY - center.y;
    const float denom = std::max(focalDist * 0.25f, focalDist - dy * sinPitch - worldZ * heightPersp);
    const float s     = focalDist / denom;
    return {center.x + dx * s, center.y + (dy * cosPitch - worldZ * heightScale) * s};
}

sf::Vector2f Camera3D::project(sf::Vector2f groundPos, float worldZ) const
{
    return project(groundPos.x, groundPos.y, worldZ);
}

sf::Vector2f Camera3D::project(sf::Vector3f pos3d) const
{
    return project(pos3d.x, pos3d.y, pos3d.z);
}

sf::Vector2f Camera3D::unprojectToGround(sf::Vector2f viewPos) const
{
    const float dvx   = viewPos.x - center.x;
    const float dvy   = viewPos.y - center.y;
    const float denom = cosPitch * focalDist + dvy * sinPitch;
    if (std::abs(denom) < 1e-4f)
    {
        return viewPos;
    }
    const float dy = (dvy * focalDist) / denom;
    const float dx = (dvx * cosPitch * focalDist) / denom;
    return {center.x + dx, center.y + dy};
}

sf::FloatRect Camera3D::getGroundFrustumBounds(float margin) const
{
    const sf::Vector2f half = size * 0.5f;
    const sf::Vector2f tl   = unprojectToGround({center.x - half.x, center.y - half.y});
    const sf::Vector2f tr   = unprojectToGround({center.x + half.x, center.y - half.y});
    const sf::Vector2f bl   = unprojectToGround({center.x - half.x, center.y + half.y});
    const sf::Vector2f br   = unprojectToGround({center.x + half.x, center.y + half.y});
    const float        minX = std::min({tl.x, tr.x, bl.x, br.x}) - margin;
    const float        maxX = std::max({tl.x, tr.x, bl.x, br.x}) + margin;
    const float        minY = std::min({tl.y, tr.y, bl.y, br.y}) - margin;
    const float        maxY = std::max({tl.y, tr.y, bl.y, br.y}) + margin;
    return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
}

sf::Color shadeColor(sf::Color c, float brightness, float highlight)
{
    const int r = static_cast<int>(c.r * brightness + highlight * 255.f);
    const int g = static_cast<int>(c.g * brightness + highlight * 255.f);
    const int b = static_cast<int>(c.b * brightness + highlight * 255.f);
    return {static_cast<uint8_t>(std::clamp(r, 0, 255)),
            static_cast<uint8_t>(std::clamp(g, 0, 255)),
            static_cast<uint8_t>(std::clamp(b, 0, 255)),
            c.a};
}

sf::Color computeLitColor(sf::Color baseColor, float nx, float ny, float nz)
{
    constexpr float lx  = -0.36f;
    constexpr float ly  = 0.32f;
    constexpr float lz  = 0.876f;
    const float     dot = nx * lx + ny * ly + nz * lz;
    const float     b   = std::clamp(0.68f + 0.38f * dot, 0.44f, 1.15f);
    const float     hl  = (dot > 0.82f) ? (dot - 0.82f) * 0.45f : 0.f;
    return shadeColor(baseColor, b, hl);
}

PlayerPalette getPlayerPalette(int clientId)
{
    switch (clientId % 3)
    {
        case 0:
            return {.fill         = sf::Color(0, 180, 255),
                    .innerOutline = sf::Color(0, 255, 255),
                    .outerPulse   = sf::Color(255, 215, 0),
                    .projOuter    = sf::Color(0, 255, 255, 180),
                    .projInner    = sf::Color(255, 215, 0, 220)};
        case 1:
            return {.fill         = sf::Color(230, 45, 45),
                    .innerOutline = sf::Color(255, 90, 90),
                    .outerPulse   = sf::Color(255, 200, 50),
                    .projOuter    = sf::Color(255, 70, 70, 180),
                    .projInner    = sf::Color(255, 215, 0, 220)};
        case 2:
            return {.fill         = sf::Color(170, 50, 230),
                    .innerOutline = sf::Color(215, 100, 255),
                    .outerPulse   = sf::Color(255, 215, 0),
                    .projOuter    = sf::Color(215, 90, 255, 180),
                    .projInner    = sf::Color(255, 215, 0, 220)};
        default:
            return {.fill         = sf::Color(0, 180, 255),
                    .innerOutline = sf::Color(0, 255, 255),
                    .outerPulse   = sf::Color(255, 215, 0),
                    .projOuter    = sf::Color(0, 255, 255, 180),
                    .projInner    = sf::Color(255, 215, 0, 220)};
    }
}

sf::Color getUnitClassColor(UnitClass uc)
{
    return GameConfig::Visual::kUnitClassColors[static_cast<int>(uc)];
}

sf::Color getWallColor(const CWall* wallComponent, bool isSelected)
{
    if (!wallComponent)
    {
        return isSelected ? kSelectedWallColor : kWallColor;
    }

    sf::Color color = kWallColor;
    switch (wallComponent->kind)
    {
        case WallKind::Defensive:
            color = isSelected ? sf::Color(245, 220, 165) : sf::Color(218, 198, 175);
            break;
        case WallKind::Electric:
            color = kElectricWallColor;
            break;
        case WallKind::Fire:
            color = kFireWallColor;
            break;
        case WallKind::Gun:
            color = kGunWallColor;
            break;
        case WallKind::Radar:
            color = kRadarWallColor;
            break;
        case WallKind::DesignatedPoint:
            color = kDesignatedPointColor;
            break;
        case WallKind::Rock:
            color = sf::Color(105, 98, 92);
            break;
    }

    if (!isSelected)
    {
        return color;
    }

    return {static_cast<uint8_t>(std::min(255, color.r + 45)),
            static_cast<uint8_t>(std::min(255, color.g + 45)),
            static_cast<uint8_t>(std::min(255, color.b + 45)),
            235};
}

sf::Color getStateColor(UnitStateKind state)
{
    switch (state)
    {
        case UnitStateKind::Search:
            return {255, 215, 90};
        case UnitStateKind::Combat:
            return {255, 150, 70};
        case UnitStateKind::Retreat:
            return {90, 240, 255};
        case UnitStateKind::Explode:
            return {255, 80, 80};
        default:
            return {40, 40, 40, 150};
    }
}

static const auto kUnitCircle30 = []()
{
    std::array<sf::Vector2f, 30> table;
    const float                  rotRad = -std::numbers::pi_v<float> * 0.5f;
    for (int p = 0; p < 30; ++p)
    {
        float a  = rotRad + p * 2.f * std::numbers::pi_v<float> / 30.f;
        table[p] = sf::Vector2f(std::cos(a), std::sin(a));
    }
    return table;
}();

static const auto kUnitCircle12 = []()
{
    std::array<sf::Vector2f, 12> table;
    const float                  rotRad = -std::numbers::pi_v<float> * 0.5f;
    for (int p = 0; p < 12; ++p)
    {
        float a  = rotRad + p * 2.f * std::numbers::pi_v<float> / 12.f;
        table[p] = sf::Vector2f(std::cos(a), std::sin(a));
    }
    return table;
}();

static const auto kUnitHexagon = []()
{
    std::array<sf::Vector2f, 6> table;
    const float                 rotRad = -std::numbers::pi_v<float> * 0.5f + (30.f * std::numbers::pi_v<float> / 180.f);
    for (int p = 0; p < 6; ++p)
    {
        float a  = rotRad + p * 2.f * std::numbers::pi_v<float> / 6.f;
        table[p] = sf::Vector2f(std::cos(a), std::sin(a));
    }
    return table;
}();

static const auto kUnitHexLines = []()
{
    std::array<sf::Vector2f, 6> table;
    for (int i = 0; i < 6; ++i)
    {
        float a  = (i * 60.f + 30.f) * std::numbers::pi_v<float> / 180.f;
        table[i] = sf::Vector2f(std::cos(a), std::sin(a));
    }
    return table;
}();

static const auto kUnitOctagon = []()
{
    std::array<sf::Vector2f, 8> table;
    const float                 rotRad = -std::numbers::pi_v<float> * 0.5f;
    for (int p = 0; p < 8; ++p)
    {
        float a  = rotRad + p * 2.f * std::numbers::pi_v<float> / 8.f;
        table[p] = sf::Vector2f(std::cos(a), std::sin(a));
    }
    return table;
}();

namespace
{

inline void appendQuad2D(sf::VertexArray& va,
                         sf::Vector2f     p0,
                         sf::Vector2f     p1,
                         sf::Vector2f     p2,
                         sf::Vector2f     p3,
                         sf::Color        cBottom,
                         sf::Color        cTop,
                         bool             cullBackface = false)
{
    if (cullBackface)
    {
        const float cross = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
        if (cross >= 0.f)
        {
            return;
        }
    }
    va.append(sf::Vertex(p0, cBottom));
    va.append(sf::Vertex(p1, cBottom));
    va.append(sf::Vertex(p2, cTop));
    va.append(sf::Vertex(p2, cTop));
    va.append(sf::Vertex(p3, cTop));
    va.append(sf::Vertex(p0, cBottom));
}

inline void addWireSegment3D(sf::VertexArray& va,
                             const Camera3D&  cam,
                             sf::Vector3f     p1_3d,
                             sf::Vector3f     p2_3d,
                             float            thickness,
                             sf::Color        c1,
                             sf::Color        c2)
{
    const sf::Vector2f v1   = cam.project(p1_3d);
    const sf::Vector2f v2   = cam.project(p2_3d);
    const sf::Vector2f diff = v2 - v1;
    const float        len  = std::hypot(diff.x, diff.y);
    if (len < 0.2f)
    {
        return;
    }
    const sf::Vector2f normal(-diff.y / len * (thickness * 0.5f), diff.x / len * (thickness * 0.5f));
    appendQuad2D(va, v1 - normal, v2 - normal, v2 + normal, v1 + normal, c1, c2, false);
}

void addBox3D(sf::VertexArray& va,
              const Camera3D&  cam,
              sf::Vector2f     pos,
              sf::Vector2f     size,
              float            z0,
              float            z1,
              sf::Color        baseColor,
              bool             drawRoof = true)
{
    const float x0 = pos.x;
    const float y0 = pos.y;
    const float x1 = pos.x + size.x;
    const float y1 = pos.y + size.y;

    const sf::Vector2f b00 = cam.project(x0, y0, z0);
    const sf::Vector2f b10 = cam.project(x1, y0, z0);
    const sf::Vector2f b11 = cam.project(x1, y1, z0);
    const sf::Vector2f b01 = cam.project(x0, y1, z0);

    const sf::Vector2f t00 = cam.project(x0, y0, z1);
    const sf::Vector2f t10 = cam.project(x1, y0, z1);
    const sf::Vector2f t11 = cam.project(x1, y1, z1);
    const sf::Vector2f t01 = cam.project(x0, y1, z1);

    {
        sf::Color cTop = computeLitColor(baseColor, 0.f, -1.f, 0.f);
        sf::Color cBot = shadeColor(cTop, 0.72f);
        appendQuad2D(va, b10, b00, t00, t10, cBot, cTop, true);
    }
    {
        sf::Color cTop = computeLitColor(baseColor, -1.f, 0.f, 0.f);
        sf::Color cBot = shadeColor(cTop, 0.74f);
        appendQuad2D(va, b00, b01, t01, t00, cBot, cTop, true);
    }
    {
        sf::Color cTop = computeLitColor(baseColor, 1.f, 0.f, 0.f);
        sf::Color cBot = shadeColor(cTop, 0.72f);
        appendQuad2D(va, b11, b10, t10, t11, cBot, cTop, true);
    }
    {
        sf::Color cTop = computeLitColor(baseColor, 0.f, 1.f, 0.f);
        sf::Color cBot = shadeColor(cTop, 0.75f);
        appendQuad2D(va, b01, b11, t11, t01, cBot, cTop, false);
    }

    if (drawRoof)
    {
        sf::Color cRoofNorth = shadeColor(baseColor, 0.96f, 0.03f);
        sf::Color cRoofSouth = shadeColor(baseColor, 1.12f, 0.08f);
        appendQuad2D(va, t01, t11, t10, t00, cRoofSouth, cRoofNorth, false);
    }
}

void addOrientedBox3D(sf::VertexArray& va,
                      const Camera3D&  cam,
                      sf::Vector2f     center,
                      sf::Vector2f     size,
                      sf::Vector2f     origin,
                      float            z0,
                      float            z1,
                      float            angleDeg,
                      sf::Color        baseColor)
{
    const float rad  = angleDeg * std::numbers::pi_v<float> / 180.f;
    const float cosA = std::cos(rad);
    const float sinA = std::sin(rad);

    auto toWorld = [&](float lx, float ly) -> sf::Vector2f
    {
        const float dx = lx - origin.x;
        const float dy = ly - origin.y;
        return {center.x + dx * cosA - dy * sinA, center.y + dx * sinA + dy * cosA};
    };

    const sf::Vector2f w0 = toWorld(0.f, 0.f);
    const sf::Vector2f w1 = toWorld(size.x, 0.f);
    const sf::Vector2f w2 = toWorld(size.x, size.y);
    const sf::Vector2f w3 = toWorld(0.f, size.y);

    const sf::Vector2f b[4] = {cam.project(w0, z0), cam.project(w1, z0), cam.project(w2, z0), cam.project(w3, z0)};
    const sf::Vector2f t[4] = {cam.project(w0, z1), cam.project(w1, z1), cam.project(w2, z1), cam.project(w3, z1)};

    const sf::Vector2f normals[4] = {{sinA, -cosA}, {cosA, sinA}, {-sinA, cosA}, {-cosA, -sinA}};

    for (int i = 0; i < 4; ++i)
    {
        const int       next = (i + 1) & 3;
        const sf::Color cTop = computeLitColor(baseColor, normals[i].x, normals[i].y, 0.1f);
        const sf::Color cBot = shadeColor(cTop, 0.74f);
        appendQuad2D(va, b[next], b[i], t[i], t[next], cBot, cTop, true);
    }

    const sf::Color cRoof = computeLitColor(baseColor, 0.f, 0.f, 1.f);
    appendQuad2D(va, t[0], t[1], t[2], t[3], cRoof, shadeColor(cRoof, 1.06f), false);
}

template <size_t N>
void addPrismLUT3D(sf::VertexArray&                   va,
                   const Camera3D&                    cam,
                   sf::Vector2f                       center,
                   float                              rBot,
                   float                              rTop,
                   float                              z0,
                   float                              z1,
                   const std::array<sf::Vector2f, N>& lut,
                   sf::Color                          baseColor,
                   bool                               drawCap = true)
{
    std::array<sf::Vector2f, N> bProj;
    std::array<sf::Vector2f, N> tProj;
    for (size_t i = 0; i < N; ++i)
    {
        bProj[i] = cam.project(center + lut[i] * rBot, z0);
        tProj[i] = cam.project(center + lut[i] * rTop, z1);
    }

    const float nz = (rBot > rTop) ? 0.25f : 0.f;
    for (size_t i = 0; i < N; ++i)
    {
        const size_t       next = (i + 1) % N;
        const sf::Vector2f n    = (lut[i] + lut[next]) * 0.5f;
        const sf::Color    cTop = computeLitColor(baseColor, n.x, n.y, nz);
        const sf::Color    cBot = shadeColor(cTop, 0.72f);
        appendQuad2D(va, bProj[next], bProj[i], tProj[i], tProj[next], cBot, cTop, true);
    }

    if (drawCap && rTop > 0.1f)
    {
        const sf::Vector2f cTopProj = cam.project(center, z1);
        const sf::Color    cCenter  = shadeColor(baseColor, 1.16f, 0.10f);
        const sf::Color    cEdge    = shadeColor(baseColor, 0.98f, 0.02f);
        for (size_t i = 0; i < N; ++i)
        {
            const size_t next = (i + 1) % N;
            va.append(sf::Vertex(cTopProj, cCenter));
            va.append(sf::Vertex(tProj[i], cEdge));
            va.append(sf::Vertex(tProj[next], cEdge));
        }
    }
}

void addOctahedron3D(sf::VertexArray& va,
                     const Camera3D&  cam,
                     sf::Vector2f     center,
                     float            radius,
                     float            zCenter,
                     float            halfHeight,
                     float            rotDeg,
                     sf::Color        color)
{
    const float        rad   = rotDeg * std::numbers::pi_v<float> / 180.f;
    const sf::Vector2f topPt = cam.project(center, zCenter + halfHeight);
    const sf::Vector2f botPt = cam.project(center, zCenter - halfHeight);
    sf::Vector2f       mid[4];
    sf::Vector2f       dirs[4];

    for (int i = 0; i < 4; ++i)
    {
        const float a = rad + i * (std::numbers::pi_v<float> * 0.5f);
        dirs[i]       = {std::cos(a), std::sin(a)};
        mid[i]        = cam.project(center + dirs[i] * radius, zCenter);
    }

    for (int i = 0; i < 4; ++i)
    {
        const int          next = (i + 1) & 3;
        const sf::Vector2f n    = (dirs[i] + dirs[next]) * 0.5f;

        const sf::Color cLow = computeLitColor(color, n.x, n.y, -0.45f);
        va.append(sf::Vertex(botPt, shadeColor(cLow, 0.75f)));
        va.append(sf::Vertex(mid[next], cLow));
        va.append(sf::Vertex(mid[i], cLow));

        const sf::Color cHigh = computeLitColor(color, n.x, n.y, 0.65f);
        va.append(sf::Vertex(topPt, shadeColor(cHigh, 1.2f, 0.18f)));
        va.append(sf::Vertex(mid[i], cHigh));
        va.append(sf::Vertex(mid[next], cHigh));
    }
}

void addFlatRing3D(sf::VertexArray& va, const Camera3D& cam, sf::Vector2f center, float rIn, float rOut, float z, sf::Color col)
{
    for (int p = 0; p < 30; ++p)
    {
        const int          next = (p + 1) % 30;
        const sf::Vector2f in1  = cam.project(center + kUnitCircle30[p] * rIn, z);
        const sf::Vector2f out1 = cam.project(center + kUnitCircle30[p] * rOut, z);
        const sf::Vector2f in2  = cam.project(center + kUnitCircle30[next] * rIn, z);
        const sf::Vector2f out2 = cam.project(center + kUnitCircle30[next] * rOut, z);

        va.append(sf::Vertex(in1, col));
        va.append(sf::Vertex(out1, col));
        va.append(sf::Vertex(out2, col));
        va.append(sf::Vertex(out2, col));
        va.append(sf::Vertex(in2, col));
        va.append(sf::Vertex(in1, col));
    }
}

void addFlatDisc3D(sf::VertexArray& va, const Camera3D& cam, sf::Vector2f center, float r, float z, sf::Color col)
{
    const sf::Vector2f cProj = cam.project(center, z);
    for (int p = 0; p < 30; ++p)
    {
        const int          next = (p + 1) % 30;
        const sf::Vector2f pt1  = cam.project(center + kUnitCircle30[p] * r, z);
        const sf::Vector2f pt2  = cam.project(center + kUnitCircle30[next] * r, z);
        va.append(sf::Vertex(cProj, col));
        va.append(sf::Vertex(pt1, col));
        va.append(sf::Vertex(pt2, col));
    }
}

void addSunkenBasin3D(sf::VertexArray& va,
                      const Camera3D&  cam,
                      sf::Vector2f     center,
                      float            rRim,
                      float            rBed,
                      float            zRim,
                      float            zBed,
                      sf::Color        bankColor)
{
    for (int p = 0; p < 30; ++p)
    {
        const int          next = (p + 1) % 30;
        const sf::Vector2f rim1 = cam.project(center + kUnitCircle30[p] * rRim, zRim);
        const sf::Vector2f rim2 = cam.project(center + kUnitCircle30[next] * rRim, zRim);
        const sf::Vector2f bed1 = cam.project(center + kUnitCircle30[p] * rBed, zBed);
        const sf::Vector2f bed2 = cam.project(center + kUnitCircle30[next] * rBed, zBed);

        const sf::Vector2f n    = (kUnitCircle30[p] + kUnitCircle30[next]) * 0.5f;
        const sf::Color    cRim = computeLitColor(bankColor, -n.x, -n.y, 0.5f);
        const sf::Color    cBed = shadeColor(cRim, 0.70f);

        appendQuad2D(va, rim1, rim2, bed2, bed1, cRim, cBed, true);
    }
}

void addFoliageLobe3D(sf::VertexArray& va,
                      const Camera3D&  cam,
                      sf::Vector2f     center,
                      float            rMid,
                      float            zBot,
                      float            zMid,
                      float            zTop,
                      sf::Color        color,
                      float            sway = 0.f)
{
    const sf::Vector2f c = center + sf::Vector2f(sway, 0.f);
    addPrismLUT3D(va, cam, c, rMid * 0.70f, rMid, zBot, zMid, kUnitOctagon, shadeColor(color, 0.72f), true);
    addPrismLUT3D(va, cam, c, rMid, rMid * 0.15f, zMid, zTop, kUnitOctagon, shadeColor(color, 1.15f, 0.08f), true);
}

} // namespace

void drawArrow(ViewState*   viewState,
               sf::Vector2f start,
               sf::Vector2f end,
               sf::Color    color,
               float,
               sf::VertexArray& lines,
               sf::VertexArray& triangles)
{
    auto direction    = end - start;
    bool hasDirection = direction.length() > 0.001f;
    if (hasDirection)
    {
        direction = direction.normalized();
    }

    sf::Vector2f p1;
    sf::Vector2f p2;
    if (hasDirection)
    {
        const sf::Vector2f n = {-direction.y, direction.x};
        p1                   = end - direction * 6.f + n * 3.f;
        p2                   = end - direction * 6.f - n * 3.f;
    }

    const sf::Vector2f sProj = viewState ? viewState->project3D(start, 3.f) : start;
    const sf::Vector2f eProj = viewState ? viewState->project3D(end, 3.f) : end;

    lines.append(sf::Vertex({.position = sProj, .color = color}));
    lines.append(sf::Vertex({.position = eProj, .color = color}));

    if (hasDirection)
    {
        const sf::Vector2f p1Proj = viewState ? viewState->project3D(p1, 3.f) : p1;
        const sf::Vector2f p2Proj = viewState ? viewState->project3D(p2, 3.f) : p2;
        triangles.append({.position = eProj, .color = color});
        triangles.append({.position = p1Proj, .color = color});
        triangles.append({.position = p2Proj, .color = color});
    }
}

std::string buildUnitLabel(MemoryPool& pool, int entityId)
{
    const auto*       mask        = pool.getMask(entityId);
    const CUnit*      unit        = pool.hasComponent<CUnit>(mask) ? &*pool.getComponent<CUnit>(entityId) : nullptr;
    const CHealth*    health      = pool.hasComponent<CHealth>(mask) ? &*pool.getComponent<CHealth>(entityId) : nullptr;
    const std::string targetLabel = (unit && unit->state == UnitStateKind::Combat)
                                        ? " T:" + std::to_string(unit->targetId)
                                        : "";
    const char*       classLabel  = "Unknown";
    if (unit)
    {
        switch (unit->unitClass)
        {
            case UnitClass::Warrior:
                classLabel = "Warrior";
                break;
            case UnitClass::Archer:
                classLabel = "Archer";
                break;
            case UnitClass::Tank:
                classLabel = "Tank";
                break;
            case UnitClass::Grunt:
                classLabel = "Grunt";
                break;
            case UnitClass::Shooter:
                classLabel = "Shooter";
                break;
            case UnitClass::Berserker:
                classLabel = "Berserker";
                break;
        }
    }

    std::array<char, 128> label{};
    std::snprintf(label.data(),
                  label.size(),
                  "#%d [T%d] %s %s HP %.1f/%1.f%s",
                  entityId,
                  GameStateDetail::getEntityTeam(pool, entityId),
                  classLabel,
                  unit ? toStateLabel(unit->state) : "None",
                  health ? static_cast<float>(health->hp) : 0.f,
                  health ? static_cast<float>(health->maxHp) : 0.f,
                  targetLabel.c_str());
    return {label.data()};
}

void drawWorldDebug(ViewState* viewState, const GameState* gameState)
{
    const auto& pool  = gameState->mAdmin.mPool;
    auto&       debug = viewState->debug();
    const auto& cam   = viewState->camera3D();

    static std::vector<sf::Vector2f> designatedPoints;
    designatedPoints.clear();
    for (const int wallId : pool.viewIndex<CWall>())
    {
        if (!viewState->isFriendly(wallId))
        {
            continue;
        }
        const auto wall = pool.getComponent<CWall>(wallId);
        if (wall->kind != WallKind::DesignatedPoint)
        {
            continue;
        }
        const auto* wallMask = pool.getMask(wallId);
        if (pool.hasComponent<CPosition>(wallMask))
        {
            const auto pos = pool.getComponent<CPosition>(wallId);
            designatedPoints.push_back(pos->pos);
        }
    }

    static sf::VertexArray linesToDesignaedPoint{sf::PrimitiveType::Lines};
    static sf::VertexArray linesCombatLinks(sf::PrimitiveType::Lines);
    static sf::VertexArray debugRangeTris(sf::PrimitiveType::Triangles);

    linesToDesignaedPoint.clear();
    linesCombatLinks.clear();
    debugRangeTris.clear();

    const bool needAgentProcessing = (debug.showCombatLinks || debug.showRanges || !designatedPoints.empty());
    if (needAgentProcessing)
    {
        auto processEntity = [&](int entityId)
        {
            const auto* mask = pool.getMask(entityId);
            if (!GameStateDetail::isAlive(gameState, entityId) || !pool.hasComponent<CPosition>(mask))
            {
                return;
            }
            const auto unit     = pool.getComponent<CUnit>(entityId);
            const auto position = pool.getComponent<CPosition>(entityId);

            {
                const auto& visionGrid = gameState->nav().getVisionGrid(viewState->localTeamId());
                const int   spacing    = gameState->nav().visionSpacing > 0 ? gameState->nav().visionSpacing : 16;
                const int   x          = static_cast<int>(position->pos.x) / spacing;
                const int   y          = static_cast<int>(position->pos.y) / spacing;

                if (!visionGrid.isValidCell(x, y) || visionGrid[x][y] == 0 || visionGrid[x][y] == 1)
                {
                    return;
                }
            }

            const sf::Color stateColor = getStateColor(unit->state);

            {
                if (unit->stance == CombatStance::Tactical && !designatedPoints.empty() && viewState->isFriendly(entityId))
                {
                    sf::Vector2f nearestPoint = designatedPoints[0];
                    float        minDist      = std::numeric_limits<float>::max();
                    for (const auto& point : designatedPoints)
                    {
                        float dist = std::hypot(point.x - position->pos.x, point.y - position->pos.y);
                        if (dist < minDist)
                        {
                            minDist      = dist;
                            nearestPoint = point;
                        }
                    }

                    linesToDesignaedPoint.append(
                        {.position = cam.project(nearestPoint, 4.f), .color = sf::Color(255, 215, 0, 95)});
                    linesToDesignaedPoint.append(
                        {.position = cam.project(position->pos, 4.f), .color = sf::Color(255, 215, 0, 95)});
                }
            }

            {
                if (debug.showCombatLinks && unit->state == UnitStateKind::Combat)
                {
                    const auto* targetMask = pool.getMask(unit->targetId);
                    if (GameStateDetail::isAlive(gameState, unit->targetId) && pool.hasComponent<CPosition>(targetMask))
                    {
                        const auto targetPosition = pool.getComponent<CPosition>(unit->targetId);
                        const bool hasSight = GameStateDetail::hasLineOfSight(gameState, position->pos, targetPosition->pos);
                        const bool      isFriendly = viewState->isFriendly(entityId);
                        const sf::Color linkColor  = hasSight ? (isFriendly ? sf::Color(50, 210, 120, 200)
                                                                            : sf::Color(255, 50, 50, 200))
                                                              : sf::Color(210, 70, 70, 200);

                        linesCombatLinks.append({.position = cam.project(position->pos, 14.f), .color = linkColor});
                        linesCombatLinks.append({.position = cam.project(targetPosition->pos, 14.f), .color = linkColor});
                    }
                }
            }

            {
                if (debug.showRanges && unit->state == UnitStateKind::Combat)
                {
                    addFlatRing3D(debugRangeTris,
                                  cam,
                                  position->pos,
                                  unit->attackRange,
                                  unit->attackRange + 1.5f,
                                  2.f,
                                  sf::Color(stateColor.r, stateColor.g, stateColor.b, 55));
                    addFlatRing3D(debugRangeTris,
                                  cam,
                                  position->pos,
                                  unit->engageRange,
                                  unit->engageRange + 1.2f,
                                  2.f,
                                  sf::Color(stateColor.r, stateColor.g, stateColor.b, 30));
                }
            }
        };

        for (const int entityId : pool.viewIndex<CUnit>())
        {
            processEntity(entityId);
        }
    }

    const auto& visionGrid = gameState->nav().getVisionGrid(viewState->localTeamId());
    int         w          = visionGrid.width();
    int         h          = visionGrid.height();

    if (w > 0 && h > 0)
    {
        const sf::Vector2u desiredSize(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
        if (viewState->mFogTexture.getSize() != desiredSize)
        {
            (void)viewState->mRawFogTexture.resize(desiredSize);
            viewState->mRawFogTexture.setSmooth(false);
            (void)viewState->mTempFogTexture.resize(desiredSize);
            viewState->mTempFogTexture.setSmooth(false);
            (void)viewState->mFogTexture.resize(desiredSize);
            viewState->mFogTexture.setSmooth(true);
            viewState->mLastVisionVersion = 0;
        }

        if (viewState->mLastVisionVersion != gameState->nav().visionVersion)
        {
            {
                PROFILE_SCOPE("Fog of War - Build Raw Pixels (CPU)");
                viewState->mLastVisionVersion = gameState->nav().visionVersion;

                const sf::Color colorLut[3] = {sf::Color(15, 19, 29, 239), sf::Color(8, 12, 20, 162), sf::Color(0, 0, 0, 0)};

                viewState->mFogPixels.resize(static_cast<size_t>(w * h));
                const char* srcData   = visionGrid.mData.data();
                sf::Color*  dstPixels = viewState->mFogPixels.data();

                constexpr int kTile = 16;
                for (int y0 = 0; y0 < h; y0 += kTile)
                {
                    const int yMax = std::min(h, y0 + kTile);
                    for (int x0 = 0; x0 < w; x0 += kTile)
                    {
                        const int xMax = std::min(w, x0 + kTile);
                        for (int y = y0; y < yMax; ++y)
                        {
                            sf::Color* rowDst = dstPixels + y * w;
                            for (int x = x0; x < xMax; ++x)
                            {
                                const uint8_t state = static_cast<uint8_t>(srcData[x * h + y]);
                                rowDst[x]           = colorLut[state <= 2 ? state : 0];
                            }
                        }
                    }
                }

                viewState->mRawFogTexture.update(reinterpret_cast<const std::uint8_t*>(dstPixels));
            }

            {
                PROFILE_SCOPE("Fog of War - Filter Gaussian 7x7 (GPU)");
                viewState->filterFogTexture();
            }
        }
    }

    if (debugRangeTris.getVertexCount() > 0)
    {
        viewState->window().draw(debugRangeTris);
    }
    if (linesToDesignaedPoint.getVertexCount() > 0)
    {
        viewState->window().draw(linesToDesignaedPoint);
    }
    if (linesCombatLinks.getVertexCount() > 0)
    {
        viewState->window().draw(linesCombatLinks);
    }

    if (debug.showVisionGrid && viewState->mFogTexture.getSize().x > 0 && viewState->mFogTexture.getSize().y > 0)
    {
        PROFILE_SCOPE("Fog of War - 3D Floating Canopy Mesh (GPU)");
        const float         mapW    = std::max(1.f, static_cast<float>(gameState->mapWidth()));
        const float         mapH    = std::max(1.f, static_cast<float>(gameState->mapHeight()));
        const sf::FloatRect frustum = cam.getGroundFrustumBounds(140.f);
        const float         time    = static_cast<float>(viewState->mCurrentFrame) * 0.042f;

        viewState->drawFogCanopy(viewState->window(), cam, frustum, {mapW, mapH}, time);
    }
}

void drawStructureRange3D(Game*            game,
                          const Camera3D&  cam,
                          sf::Vector2f     center,
                          float            range,
                          sf::Color        baseColor,
                          bool             isSelected,
                          int              currentFrame,
                          sf::VertexArray& groundTriBatch)
{
    (void)game;
    (void)cam;
    (void)center;
    (void)range;
    (void)baseColor;
    (void)isSelected;
    (void)currentFrame;
    (void)groundTriBatch;
}


void drawVietnamesePagodaStupa3D(sf::VertexArray&    triBatch,
                                 const Camera3D&     cam,
                                 const sf::Vector2f& center,
                                 float               baseSize,
                                 bool                isSelected,
                                 int                 currentFrame)
{
    const float R    = baseSize * 0.44f;
    const float sway = std::sin(static_cast<float>(currentFrame) * 0.05f) * 1.5f;

    const sf::Color stoneBaseCol = isSelected ? sf::Color(235, 210, 150) : sf::Color(130, 138, 142);
    const sf::Color stupaBrick   = sf::Color(162, 50, 36);
    const sf::Color stupaWood    = sf::Color(88, 46, 26);
    const sf::Color tileRed      = sf::Color(192, 54, 40);
    const sf::Color tileGold     = sf::Color(242, 192, 52);
    const sf::Color lanternLight = sf::Color(255, 222, 110);
    const sf::Color festiveRed   = sf::Color(245, 52, 48);

    addFlatRing3D(triBatch, cam, center, R * 0.35f, R * 1.15f, 0.05f, sf::Color(0, 0, 0, 52));

    addPrismLUT3D(triBatch, cam, center, R * 1.08f, R * 0.96f, 0.f, 4.5f, kUnitOctagon, stoneBaseCol, true);
    addPrismLUT3D(triBatch, cam, center, R * 0.96f, R * 0.88f, 4.5f, 9.0f, kUnitOctagon, shadeColor(stoneBaseCol, 1.12f), true);

    const float stairW = R * 0.45f;
    addBox3D(triBatch,
             cam,
             {center.x - stairW * 0.5f, center.y + R * 0.88f},
             {stairW, 5.f},
             0.f,
             4.5f,
             shadeColor(stoneBaseCol, 0.92f),
             true);
    addBox3D(triBatch, cam, {center.x - stairW * 0.5f, center.y + R * 0.76f}, {stairW, 5.f}, 4.5f, 9.0f, stoneBaseCol, true);

    addPrismLUT3D(triBatch, cam, center, R * 0.82f, R * 0.74f, 9.0f, 31.0f, kUnitOctagon, stupaBrick, true);

    const float doorW = R * 0.32f;
    addBox3D(triBatch, cam, {center.x - doorW * 0.5f, center.y + R * 0.70f}, {doorW, 2.5f}, 9.0f, 24.0f, lanternLight, true);

    addPrismLUT3D(triBatch, cam, center, R * 1.05f, R * 0.76f, 31.0f, 36.5f, kUnitOctagon, tileRed, true);
    addPrismLUT3D(triBatch, cam, center, R * 0.76f, R * 0.62f, 36.5f, 42.0f, kUnitOctagon, shadeColor(tileRed, 1.12f), true);

    addFlatRing3D(triBatch, cam, center, R * 0.58f, R * 0.68f, 42.5f, stupaWood);
    addPrismLUT3D(triBatch, cam, center, R * 0.62f, R * 0.52f, 42.0f, 60.0f, kUnitOctagon, stupaBrick, true);

    addBox3D(triBatch, cam, {center.x - doorW * 0.40f, center.y + R * 0.50f}, {doorW * 0.80f, 2.f}, 46.0f, 55.0f, lanternLight, true);

    addPrismLUT3D(triBatch, cam, center, R * 0.78f, R * 0.54f, 60.0f, 65.0f, kUnitOctagon, tileRed, true);
    addPrismLUT3D(triBatch, cam, center, R * 0.54f, R * 0.40f, 65.0f, 70.0f, kUnitOctagon, shadeColor(tileRed, 1.12f), true);

    addPrismLUT3D(triBatch, cam, center, R * 0.40f, R * 0.32f, 70.0f, 79.5f, kUnitOctagon, stupaBrick, true);
    addPrismLUT3D(triBatch, cam, center, R * 0.50f, R * 0.18f, 79.5f, 86.0f, kUnitOctagon, tileRed, true);

    addPrismLUT3D(triBatch, cam, center, 6.2f, 4.2f, 86.0f, 91.0f, kUnitOctagon, tileGold, true);
    addPrismLUT3D(triBatch, cam, center, 4.2f, 2.8f, 91.0f, 96.0f, kUnitOctagon, tileGold, true);
    addOctahedron3D(triBatch, cam, center, 4.5f, 100.0f, 4.5f, 0.f, tileGold);

    const sf::Vector2f lanternPos = center + sf::Vector2f(-R * 0.65f, R * 0.65f + sway);
    addOctahedron3D(triBatch, cam, lanternPos, 3.2f, 25.f, 3.2f, 0.f, festiveRed);
}

void drawVietnamesePagodaTemple3D(sf::VertexArray&    triBatch,
                                  const Camera3D&     cam,
                                  const sf::Vector2f& center,
                                  float               baseSize,
                                  bool                isSelected,
                                  int                 currentFrame)
{
    const float        W      = baseSize * 0.88f;
    const float        H      = baseSize * 0.74f;
    const sf::Vector2f origin = center - sf::Vector2f(W * 0.5f, H * 0.5f);
    const float        sway   = std::sin(static_cast<float>(currentFrame) * 0.05f) * 1.5f;

    const sf::Color stoneBaseCol = isSelected ? sf::Color(235, 210, 150) : sf::Color(132, 140, 145);
    const sf::Color woodLimDark  = sf::Color(82, 44, 24);
    const sf::Color woodSonRed   = sf::Color(152, 40, 32);
    const sf::Color deckWood     = sf::Color(122, 68, 38);
    const sf::Color tileRed      = sf::Color(195, 56, 42);
    const sf::Color goldTrim     = sf::Color(245, 195, 55);
    const sf::Color lanternLight = sf::Color(255, 222, 110);
    const sf::Color redSilk      = sf::Color(245, 50, 48);
    const sf::Color bronzeBell   = sf::Color(180, 140, 70);

    const sf::Vector2f shadowPos  = origin - sf::Vector2f(3.f, 2.f);
    const sf::Vector2f shadowSize = sf::Vector2f(W + 6.f, H + 6.f);
    appendQuad2D(triBatch,
                 cam.project(shadowPos.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y, 0.04f),
                 cam.project(shadowPos.x, shadowPos.y, 0.04f),
                 sf::Color(0, 0, 0, 52),
                 sf::Color(0, 0, 0, 52),
                 false);

    addBox3D(triBatch, cam, origin, {W, H}, 0.f, 6.f, stoneBaseCol, true);
    addBox3D(triBatch,
             cam,
             origin + sf::Vector2f(W * 0.04f, H * 0.04f),
             {W * 0.92f, H * 0.92f},
             6.f,
             11.f,
             shadeColor(stoneBaseCol, 1.10f),
             true);

    const float stairW      = 28.f;
    const float stairX0     = center.x - stairW * 0.5f;
    const float stairYFront = origin.y + H;
    addBox3D(triBatch, cam, {stairX0, stairYFront + 6.f}, {stairW, 5.f}, 0.f, 4.f, shadeColor(stoneBaseCol, 0.92f), true);
    addBox3D(triBatch, cam, {stairX0, stairYFront + 2.f}, {stairW, 5.f}, 4.f, 8.f, stoneBaseCol, true);
    addBox3D(triBatch, cam, {stairX0, stairYFront - 2.f}, {stairW, 5.f}, 8.f, 11.f, shadeColor(stoneBaseCol, 1.08f), true);

    addPrismLUT3D(triBatch, cam, {center.x, stairYFront + 10.f}, 4.5f, 3.5f, 0.f, 6.5f, kUnitOctagon, bronzeBell, true);
    addOctahedron3D(triBatch, cam, {center.x, stairYFront + 10.f}, 3.0f, 8.5f, 2.5f, 0.f, goldTrim);

    const sf::Vector2f stoneLanternL = origin + sf::Vector2f(W * 0.08f, H * 0.90f);
    const sf::Vector2f stoneLanternR = origin + sf::Vector2f(W * 0.92f, H * 0.90f);
    addPrismLUT3D(triBatch, cam, stoneLanternL, 3.5f, 2.5f, 11.f, 18.f, kUnitOctagon, stoneBaseCol, true);
    addOctahedron3D(triBatch, cam, stoneLanternL, 2.6f, 20.5f, 2.6f, 0.f, lanternLight);
    addPrismLUT3D(triBatch, cam, stoneLanternR, 3.5f, 2.5f, 11.f, 18.f, kUnitOctagon, stoneBaseCol, true);
    addOctahedron3D(triBatch, cam, stoneLanternR, 2.6f, 20.5f, 2.6f, 0.f, lanternLight);

    const sf::Vector2f deckPos  = origin + sf::Vector2f(W * 0.06f, H * 0.06f);
    const sf::Vector2f deckSize = {W * 0.88f, H * 0.88f};
    addBox3D(triBatch, cam, deckPos, deckSize, 11.f, 13.5f, deckWood, true);

    const sf::Vector2f hallPos  = center - sf::Vector2f(W * 0.38f, H * 0.35f);
    const sf::Vector2f hallSize = {W * 0.76f, H * 0.70f};
    addBox3D(triBatch, cam, hallPos, hallSize, 13.5f, 36.f, woodLimDark, true);

    const float colY     = hallPos.y + hallSize.y - 1.f;
    const float colXs[4] = {hallPos.x + hallSize.x * 0.08f,
                            hallPos.x + hallSize.x * 0.36f,
                            hallPos.x + hallSize.x * 0.64f,
                            hallPos.x + hallSize.x * 0.92f};
    for (int c = 0; c < 4; ++c)
    {
        addBox3D(triBatch, cam, {colXs[c] - 3.5f, colY - 3.5f}, {7.f, 7.f}, 13.5f, 15.5f, stoneBaseCol, true);
        addPrismLUT3D(triBatch, cam, {colXs[c], colY}, 3.4f, 2.8f, 15.5f, 36.0f, kUnitOctagon, woodSonRed, true);
    }

    const float doorMidW = hallSize.x * 0.28f;
    addBox3D(triBatch, cam, {center.x - doorMidW * 0.5f, colY}, {doorMidW, 2.5f}, 13.5f, 31.0f, lanternLight, true);

    const float winW = hallSize.x * 0.22f;
    addBox3D(triBatch, cam, {hallPos.x + hallSize.x * 0.12f, colY}, {winW, 2.f}, 18.0f, 29.0f, lanternLight, true);
    addBox3D(triBatch, cam, {hallPos.x + hallSize.x * 0.66f, colY}, {winW, 2.f}, 18.0f, 29.0f, lanternLight, true);

    addBox3D(triBatch, cam, {center.x - doorMidW * 0.6f, colY + 1.f}, {doorMidW * 1.2f, 2.2f}, 31.5f, 35.5f, goldTrim, true);

    addBox3D(triBatch,
             cam,
             center - sf::Vector2f(W * 0.46f, H * 0.43f),
             {W * 0.92f, H * 0.86f},
             35.5f,
             41.0f,
             shadeColor(tileRed, 0.82f),
             true);
    addBox3D(triBatch, cam, center - sf::Vector2f(W * 0.39f, H * 0.36f), {W * 0.78f, H * 0.72f}, 41.0f, 47.0f, tileRed, true);
    addBox3D(triBatch,
             cam,
             center - sf::Vector2f(W * 0.31f, H * 0.29f),
             {W * 0.62f, H * 0.58f},
             47.0f,
             52.0f,
             shadeColor(tileRed, 1.10f),
             true);

    for (int i = 0; i < 4; ++i)
    {
        const sf::Vector2f cornerDir((i == 1 || i == 2) ? 1.f : -1.f, (i >= 2) ? 1.f : -1.f);
        const sf::Vector2f tipPos = center + sf::Vector2f(cornerDir.x * (W * 0.48f), cornerDir.y * (H * 0.45f));
        addBox3D(triBatch, cam, tipPos - sf::Vector2f(3.f, 3.f), {6.f, 6.f}, 40.0f, 46.5f, goldTrim, true);
    }

    const sf::Vector2f loftPos  = center - sf::Vector2f(W * 0.25f, H * 0.23f);
    const sf::Vector2f loftSize = {W * 0.50f, H * 0.46f};
    addBox3D(triBatch, cam, loftPos, loftSize, 52.0f, 67.5f, woodLimDark, true);

    addPrismLUT3D(triBatch, cam, center, 4.5f, 3.2f, 54.0f, 64.0f, kUnitOctagon, bronzeBell, true);
    addBox3D(triBatch,
             cam,
             {center.x - loftSize.x * 0.25f, loftPos.y + loftSize.y - 1.f},
             {loftSize.x * 0.50f, 2.f},
             55.0f,
             63.0f,
             lanternLight,
             true);

    addBox3D(triBatch,
             cam,
             center - sf::Vector2f(W * 0.31f, H * 0.28f),
             {W * 0.62f, H * 0.56f},
             67.5f,
             73.0f,
             shadeColor(tileRed, 0.84f),
             true);
    addBox3D(triBatch, cam, center - sf::Vector2f(W * 0.23f, H * 0.21f), {W * 0.46f, H * 0.42f}, 73.0f, 79.0f, tileRed, true);
    addBox3D(triBatch,
             cam,
             center - sf::Vector2f(W * 0.14f, H * 0.13f),
             {W * 0.28f, H * 0.26f},
             79.0f,
             84.0f,
             shadeColor(tileRed, 1.12f),
             true);

    for (int i = 0; i < 4; ++i)
    {
        const sf::Vector2f cornerDir((i == 1 || i == 2) ? 1.f : -1.f, (i >= 2) ? 1.f : -1.f);
        const sf::Vector2f tipPos = center + sf::Vector2f(cornerDir.x * (W * 0.33f), cornerDir.y * (H * 0.30f));
        addBox3D(triBatch, cam, tipPos - sf::Vector2f(2.5f, 2.5f), {5.f, 5.f}, 72.0f, 78.5f, goldTrim, true);
    }

    const float ridgeLen = W * 0.32f;
    addBox3D(triBatch, cam, {center.x - ridgeLen * 0.5f, center.y - 2.5f}, {ridgeLen, 5.f}, 84.0f, 87.5f, goldTrim, true);
    addBox3D(triBatch, cam, {center.x - ridgeLen * 0.5f - 1.f, center.y - 3.f}, {4.f, 6.f}, 86.5f, 91.5f, goldTrim, true);
    addBox3D(triBatch, cam, {center.x + ridgeLen * 0.5f - 3.f, center.y - 3.f}, {4.f, 6.f}, 86.5f, 91.5f, goldTrim, true);

    addPrismLUT3D(triBatch, cam, center, 6.5f, 4.5f, 87.5f, 92.0f, kUnitOctagon, goldTrim, true);
    addOctahedron3D(triBatch, cam, center, 4.5f, 96.0f, 4.5f, 0.f, goldTrim);

    const sf::Vector2f lanternL = {center.x - doorMidW * 0.70f, colY + 3.f + sway};
    const sf::Vector2f lanternR = {center.x + doorMidW * 0.70f, colY + 3.f - sway};
    addOctahedron3D(triBatch, cam, lanternL, 3.6f, 25.f, 3.6f, 0.f, redSilk);
    addOctahedron3D(triBatch, cam, lanternR, 3.6f, 25.f, 3.6f, 0.f, redSilk);
}

void drawBase(Game*                game,
              const Camera3D&      cam,
              const CRectDragDrop& baseRect,
              const CBase*         baseComponent,
              const CHealth*       healthComponent,
              bool                 isSelected,
              int                  currentFrame,
              sf::VertexArray&     triBatch,
              sf::VertexArray&     lineBatch,
              sf::VertexArray*     hudBatch,
              bool                 isFriendly)
{
    (void)game;
    (void)baseComponent;
    (void)lineBatch;

    const bool         isMainBase = baseRect.size.x > 120.f;
    const sf::Vector2f center     = baseRect.pos + baseRect.size * 0.5f;

    if (isMainBase)
    {
        drawVietnamesePagodaTemple3D(triBatch, cam, center, baseRect.size.x, isSelected, currentFrame);
    }
    else
    {
        drawVietnamesePagodaStupa3D(triBatch, cam, center, baseRect.size.x, isSelected, currentFrame);
    }

    const float barZ = isMainBase ? 104.f : 110.f;

    if (healthComponent)
    {
        sf::VertexArray& barTarget = hudBatch ? *hudBatch : triBatch;
        const float healthRatio = healthComponent->maxHp > 0
                                      ? std::clamp((float)healthComponent->hp / (float)healthComponent->maxHp, 0.f, 1.f)
                                      : 0.f;

        const float barW = isMainBase ? 68.f : 50.f;
        const float barD = 5.5f;

        if (healthComponent->armor > 0)
        {
            const sf::Vector2f armorPos{center.x - barW * 0.5f, center.y - barD * 0.5f};
            addBox3D(barTarget, cam, armorPos, {barW, barD}, barZ + 8.f, barZ + 13.f, sf::Color(255, 205, 55), true);
        }

        const sf::Vector2f bgPos{center.x - barW * 0.5f, center.y - barD * 0.5f};
        addBox3D(barTarget, cam, bgPos, {barW, barD}, barZ, barZ + 5.5f, sf::Color(28, 34, 48, 220), true);
        if (healthRatio > 0.01f)
        {
            const sf::Color hpColor = isFriendly ? sf::Color(60, 205, 255) : sf::Color(255, 85, 85);
            addBox3D(barTarget, cam, bgPos, {barW * healthRatio, barD + 0.4f}, barZ + 0.3f, barZ + 5.8f, hpColor, true);
        }
    }
}

void drawRock3D(sf::VertexArray& triBatch, const Camera3D& cam, const CRectDragDrop& wallRect, int level, int frame)
{
    (void)frame;
    const sf::Vector2f center = wallRect.pos + wallRect.size * 0.5f;
    const float        minDim = std::min(wallRect.size.x, wallRect.size.y);

    if (level == 1)
    {
        const sf::Color sandBase(215, 175, 120);
        const sf::Color sandMid(235, 198, 142);
        const sf::Color sandHigh(250, 218, 165);
        const sf::Color sandCrest(255, 238, 190);
        const sf::Color sandShade(178, 135, 88);

        addPrismLUT3D(triBatch, cam, center, minDim * 1.05f, minDim * 0.85f, 0.f, 6.f, kUnitOctagon, sandBase, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.85f, minDim * 0.65f, 6.f, 16.f, kUnitOctagon, sandMid, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.65f, minDim * 0.44f, 16.f, 28.f, kUnitOctagon, sandHigh, true);

        const sf::Vector2f crestShift(minDim * 0.08f, -minDim * 0.05f);
        addPrismLUT3D(triBatch, cam, center + crestShift * 0.5f, minDim * 0.44f, minDim * 0.22f, 28.f, 38.f, kUnitHexagon, sandHigh, true);

        addPrismLUT3D(triBatch, cam, center + crestShift, minDim * 0.22f, minDim * 0.05f, 38.f, 46.f, kUnitHexagon, sandCrest, true);

        const sf::Vector2f wingCenter = center + sf::Vector2f(wallRect.size.x * 0.32f, -wallRect.size.y * 0.16f);
        addPrismLUT3D(triBatch, cam, wingCenter, minDim * 0.48f, minDim * 0.30f, 0.f, 12.f, kUnitHexagon, sandMid, true);
        addPrismLUT3D(triBatch, cam, wingCenter, minDim * 0.30f, minDim * 0.10f, 12.f, 24.f, kUnitHexagon, sandHigh, true);

        addFlatRing3D(triBatch, cam, center, minDim * 0.62f, minDim * 0.70f, 14.f, sandShade);
        addFlatRing3D(triBatch, cam, center + crestShift * 0.5f, minDim * 0.38f, minDim * 0.44f, 26.f, sandCrest);
    }
    else if (level == 2)
    {
        const sf::Color duneBase(205, 162, 112);
        const sf::Color duneMid(225, 185, 130);
        const sf::Color duneHigh(242, 205, 150);
        const sf::Color duneSummit(255, 225, 175);
        const sf::Color rockShelf(178, 120, 75);

        addPrismLUT3D(triBatch, cam, center, minDim * 1.05f, minDim * 0.82f, 0.f, 8.f, kUnitOctagon, duneBase, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.82f, minDim * 0.60f, 8.f, 20.f, kUnitOctagon, duneMid, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.60f, minDim * 0.38f, 20.f, 34.f, kUnitOctagon, duneHigh, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.38f, minDim * 0.18f, 34.f, 46.f, kUnitHexagon, duneHigh, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.18f, minDim * 0.04f, 46.f, 52.f, kUnitHexagon, duneSummit, true);

        const sf::Vector2f secCenter = center + sf::Vector2f(wallRect.size.x * 0.28f, wallRect.size.y * 0.20f);
        addPrismLUT3D(triBatch, cam, secCenter, minDim * 0.44f, minDim * 0.26f, 0.f, 14.f, kUnitHexagon, duneMid, true);
        addPrismLUT3D(triBatch, cam, secCenter, minDim * 0.26f, minDim * 0.08f, 14.f, 26.f, kUnitHexagon, duneHigh, true);

        addPrismLUT3D(triBatch,
                      cam,
                      center + sf::Vector2f(-minDim * 0.25f, minDim * 0.12f),
                      minDim * 0.20f,
                      minDim * 0.12f,
                      12.f,
                      22.f,
                      kUnitHexagon,
                      rockShelf,
                      true);
        addFlatDisc3D(triBatch, cam, center, minDim * 0.12f, 51.5f, sf::Color(255, 235, 190));
    }
    else
    {
        const sf::Color whiteBeach(242, 235, 210);
        const sf::Color surfFoam(215, 248, 255, 210);
        const sf::Color cliffBase(85, 98, 105);
        const sf::Color cliffBody(130, 142, 148);
        const sf::Color tropicalJungle(32, 155, 75);

        addPrismLUT3D(triBatch, cam, center, minDim * 1.02f, minDim * 0.82f, 0.f, 6.f, kUnitOctagon, whiteBeach, true);
        addFlatRing3D(triBatch, cam, center, minDim * 0.88f, minDim * 1.05f, 0.5f, surfFoam);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.82f, minDim * 0.60f, 6.f, 22.f, kUnitOctagon, cliffBase, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.60f, minDim * 0.38f, 22.f, 38.f, kUnitOctagon, cliffBody, true);

        addPrismLUT3D(triBatch, cam, center, minDim * 0.38f, minDim * 0.12f, 38.f, 50.f, kUnitHexagon, tropicalJungle, true);
        addFlatDisc3D(triBatch, cam, center, minDim * 0.22f, 50.5f, sf::Color(55, 185, 88));

        const sf::Vector2f seaStack = center + sf::Vector2f(wallRect.size.x * 0.30f, wallRect.size.y * 0.18f);
        addPrismLUT3D(triBatch, cam, seaStack, minDim * 0.38f, minDim * 0.24f, 0.f, 6.f, kUnitHexagon, whiteBeach, true);
        addPrismLUT3D(triBatch, cam, seaStack, minDim * 0.24f, minDim * 0.12f, 6.f, 24.f, kUnitHexagon, cliffBody, true);
        addPrismLUT3D(triBatch, cam, seaStack, minDim * 0.12f, minDim * 0.04f, 24.f, 30.f, kUnitHexagon, tropicalJungle, true);
    }
}

void drawBush3D(sf::VertexArray& triBatch, const Camera3D& cam, sf::Vector2f pos, sf::Vector2f size, int level, int frame)
{
    const sf::Vector2f center = pos + size * 0.5f;
    const float        minDim = std::min(size.x, size.y);
    const float        sway   = std::sin(static_cast<float>(frame) * 0.045f + pos.x * 0.02f) * 1.2f;

    sf::Color baseCol;
    sf::Color midCol;
    sf::Color topCol;

    if (level == 1 || level == 2)
    {
        baseCol = sf::Color(145, 120, 55, 235);
        midCol  = sf::Color(185, 158, 70, 240);
        topCol  = sf::Color(225, 198, 90, 245);
    }
    else
    {
        baseCol = sf::Color(35, 115, 48, 235);
        midCol  = sf::Color(58, 175, 65, 240);
        topCol  = sf::Color(95, 225, 90, 245);
    }


    const float h1 = 12.f;
    const float r1 = minDim * 0.42f;
    addPrismLUT3D(triBatch, cam, center + sf::Vector2f(sway, 0.f), r1, r1 * 0.70f, 0.f, h1, kUnitOctagon, baseCol, true);
    addPrismLUT3D(triBatch, cam, center + sf::Vector2f(sway * 1.2f, 0.f), r1 * 0.70f, r1 * 0.25f, h1, h1 + 6.f, kUnitOctagon, midCol, true);

    const sf::Vector2f leftCenter = center + sf::Vector2f(-minDim * 0.20f + sway * 0.8f, minDim * 0.08f);
    addPrismLUT3D(triBatch, cam, leftCenter, r1 * 0.65f, r1 * 0.20f, 0.f, h1 + 2.f, kUnitHexagon, midCol, true);
    const sf::Vector2f rightCenter = center + sf::Vector2f(minDim * 0.22f + sway * 0.9f, -minDim * 0.06f);
    addPrismLUT3D(triBatch, cam, rightCenter, r1 * 0.68f, r1 * 0.22f, 0.f, h1 + 3.f, kUnitHexagon, topCol, true);
}

void drawHazardPool3D(sf::VertexArray& triBatch,
                      const Camera3D&  cam,
                      sf::Vector2f     pos,
                      sf::Vector2f     size,
                      ZoneKind         kind,
                      int              level,
                      int              frame)
{
    const sf::Vector2f center = pos + size * 0.5f;
    const float        rAvg   = (size.x + size.y) * 0.25f;

    if (kind == ZoneKind::Water)
    {
        if (level == 3)
        {
            addSunkenBasin3D(triBatch, cam, center, rAvg * 1.05f, rAvg * 0.78f, 0.f, -6.5f, sf::Color(220, 212, 185));
            addFlatDisc3D(triBatch, cam, center, rAvg * 0.78f, -6.5f, sf::Color(205, 198, 172));
            addFlatRing3D(triBatch,
                          cam,
                          center + sf::Vector2f(rAvg * 0.2f, -rAvg * 0.15f),
                          rAvg * 0.15f,
                          rAvg * 0.32f,
                          -5.5f,
                          sf::Color(240, 95, 120, 210));
            addFlatDisc3D(triBatch, cam, center, rAvg * 0.92f, -3.8f, sf::Color(28, 175, 215, 195));
        }
        else
        {
            addFlatRing3D(triBatch, cam, center, rAvg * 0.98f, rAvg * 1.10f, 0.1f, sf::Color(48, 68, 52, 200));
            addSunkenBasin3D(triBatch, cam, center, rAvg * 0.98f, rAvg * 0.75f, 0.f, -7.5f, sf::Color(65, 82, 60));
            addFlatDisc3D(triBatch, cam, center, rAvg * 0.75f, -7.5f, sf::Color(28, 48, 55));
            addFlatDisc3D(triBatch, cam, center, rAvg * 0.88f, -4.2f, sf::Color(22, 105, 168, 205));
            addFlatDisc3D(triBatch, cam, center, rAvg * 0.55f, -4.3f, sf::Color(14, 72, 138, 225));
        }
    }
    else if (kind == ZoneKind::Quicksand)
    {
        addFlatRing3D(triBatch, cam, center, rAvg * 0.95f, rAvg * 1.08f, 0.1f, sf::Color(150, 115, 75, 210));
        addSunkenBasin3D(triBatch, cam, center, rAvg * 0.95f, rAvg * 0.45f, 0.f, -8.5f, sf::Color(175, 138, 88));
        addFlatDisc3D(triBatch, cam, center, rAvg * 0.45f, -8.5f, sf::Color(125, 90, 52));
        const float swirlR     = rAvg * (0.25f + std::fmod(static_cast<float>(frame) * 0.006f, 0.5f));
        const auto  swirlAlpha = static_cast<uint8_t>(std::clamp(130.f * (1.f - swirlR / (rAvg * 0.8f)), 0.f, 130.f));
        addFlatRing3D(triBatch, cam, center, swirlR * 0.8f, swirlR, -5.5f, sf::Color(115, 82, 48, swirlAlpha));
    }
    else if (kind == ZoneKind::Lava)
    {
        addFlatRing3D(triBatch, cam, center, rAvg * 0.92f, rAvg * 1.08f, 0.1f, sf::Color(45, 22, 18, 240));
        addSunkenBasin3D(triBatch, cam, center, rAvg * 0.92f, rAvg * 0.72f, 0.f, -6.5f, sf::Color(95, 30, 15));
        addFlatDisc3D(triBatch, cam, center, rAvg * 0.82f, -3.5f, sf::Color(215, 45, 12, 235));
        const float pulse = std::sin(static_cast<float>(frame) * 0.09f + pos.x * 0.02f) * 18.f;
        const auto  coreG = static_cast<uint8_t>(std::clamp(125.f + pulse, 90.f, 170.f));
        addFlatDisc3D(triBatch, cam, center, rAvg * 0.50f, -3.2f, sf::Color(255, coreG, 15, 245));
    }
}

void drawTree3D(sf::VertexArray& triBatch, const Camera3D& cam, sf::Vector2f pos, sf::Vector2f size, int level, int frame)
{
    const sf::Vector2f center = pos + size * 0.5f;
    const float        minDim = std::min(size.x, size.y);
    const float        sway   = std::sin(static_cast<float>(frame) * 0.045f + pos.x * 0.015f) * 2.0f;

    uint32_t h        = static_cast<uint32_t>(static_cast<int>(pos.x)) * 374761393u +
                        static_cast<uint32_t>(static_cast<int>(pos.y)) * 668265263u;
    h                 = ((h ^ (h >> 13)) * 1274126177u);
    const int variant = static_cast<int>(((h ^ (h >> 16))) % 4);

    if (variant == 0)
    {
        const float trunkH = 34.f;
        sf::Color   barkColor;
        sf::Color   leafCol;

        if (level == 1)
        {
            barkColor = sf::Color(100, 72, 44);
            leafCol   = sf::Color(88, 146, 48);
        }
        else if (level == 2)
        {
            barkColor = sf::Color(65, 45, 28);
            leafCol   = sf::Color(55, 148, 45);
        }
        else
        {
            barkColor = sf::Color(130, 108, 85);
            leafCol   = sf::Color(35, 155, 75);
        }

        addPrismLUT3D(triBatch, cam, center, 8.5f, 5.2f, 0.f, 6.f, kUnitOctagon, shadeColor(barkColor, 0.82f), true);
        addPrismLUT3D(triBatch, cam, center, 5.2f, 3.6f, 6.f, trunkH, kUnitOctagon, barkColor, true);

        const sf::Vector2f bNW = center + sf::Vector2f(-minDim * 0.18f, -minDim * 0.08f);
        const sf::Vector2f bSE = center + sf::Vector2f(minDim * 0.20f, minDim * 0.08f);
        const sf::Vector2f bSW = center + sf::Vector2f(-minDim * 0.10f, minDim * 0.16f);
        addPrismLUT3D(triBatch, cam, (center + bNW) * 0.5f, 3.0f, 1.8f, trunkH - 2.f, trunkH + 7.f, kUnitHexagon, barkColor, true);
        addPrismLUT3D(triBatch, cam, (center + bSE) * 0.5f, 2.8f, 1.8f, trunkH - 2.f, trunkH + 6.f, kUnitHexagon, barkColor, true);
        addPrismLUT3D(triBatch, cam, (center + bSW) * 0.5f, 2.8f, 1.8f, trunkH - 2.f, trunkH + 5.f, kUnitHexagon, barkColor, true);

        addFoliageLobe3D(triBatch, cam, center, minDim * 0.38f, trunkH + 2.f, trunkH + 13.f, trunkH + 25.f, leafCol, sway);
        addFoliageLobe3D(triBatch,
                         cam,
                         bNW,
                         minDim * 0.28f,
                         trunkH - 3.f,
                         trunkH + 8.f,
                         trunkH + 18.f,
                         shadeColor(leafCol, 1.05f),
                         sway * 0.8f);
        addFoliageLobe3D(triBatch,
                         cam,
                         bSE,
                         minDim * 0.27f,
                         trunkH - 3.f,
                         trunkH + 7.f,
                         trunkH + 17.f,
                         shadeColor(leafCol, 0.94f),
                         sway * 0.9f);
        addFoliageLobe3D(triBatch,
                         cam,
                         bSW,
                         minDim * 0.25f,
                         trunkH - 5.f,
                         trunkH + 6.f,
                         trunkH + 16.f,
                         shadeColor(leafCol, 0.86f),
                         sway * 0.7f);
        addFoliageLobe3D(triBatch,
                         cam,
                         center + sf::Vector2f(0.f, -minDim * 0.05f),
                         minDim * 0.22f,
                         trunkH + 14.f,
                         trunkH + 24.f,
                         trunkH + 33.f,
                         shadeColor(leafCol, 1.20f, 0.08f),
                         sway * 1.2f);
    }
    else if (variant == 1)
    {
        const float        trunkH      = 40.f;
        const sf::Vector2f crownCenter = center + sf::Vector2f(sway * 0.6f, 0.f);
        const sf::Color    barkColor(140, 118, 92);

        addPrismLUT3D(triBatch, cam, center, 6.2f, 4.0f, 0.f, 5.f, kUnitOctagon, sf::Color(155, 132, 105), true);

        addPrismLUT3D(triBatch, cam, center, 4.0f, 2.2f, 5.f, trunkH, kUnitHexagon, barkColor, true);

        const sf::Color palmGreen(36, 165, 70);

        addFoliageLobe3D(triBatch, cam, crownCenter, minDim * 0.25f, trunkH - 1.f, trunkH + 8.f, trunkH + 17.f, palmGreen, sway);

        for (int i = 0; i < 6; ++i)
        {
            const float        ang = static_cast<float>(i) * (std::numbers::pi_v<float> / 3.0f) + 0.18f;
            const sf::Vector2f dir(std::cos(ang), std::sin(ang));
            const sf::Vector2f frondPos = crownCenter + dir * (minDim * 0.28f);

            addFoliageLobe3D(triBatch,
                             cam,
                             frondPos,
                             minDim * 0.20f,
                             trunkH - 3.f,
                             trunkH + 6.f,
                             trunkH + 15.f,
                             (i % 2 == 0) ? palmGreen : shadeColor(palmGreen, 0.92f),
                             sway * (0.9f + 0.08f * static_cast<float>(i)));
        }

        addFoliageLobe3D(triBatch,
                         cam,
                         crownCenter + sf::Vector2f(0.f, -minDim * 0.04f),
                         minDim * 0.18f,
                         trunkH + 10.f,
                         trunkH + 18.f,
                         trunkH + 25.f,
                         shadeColor(palmGreen, 1.18f, 0.08f),
                         sway * 1.3f);
    }
    else if (variant == 2)
    {
        const float     trunkH = 32.f;
        const sf::Color barkColor(72, 50, 32);

        addPrismLUT3D(triBatch, cam, center, 7.8f, 4.8f, 0.f, 5.f, kUnitOctagon, shadeColor(barkColor, 0.85f), true);
        addPrismLUT3D(triBatch, cam, center, 4.8f, 3.4f, 5.f, trunkH, kUnitOctagon, barkColor, true);

        const sf::Vector2f bLeft  = center + sf::Vector2f(-minDim * 0.18f, minDim * 0.08f);
        const sf::Vector2f bRight = center + sf::Vector2f(minDim * 0.19f, -minDim * 0.07f);
        addPrismLUT3D(triBatch, cam, (center + bLeft) * 0.5f, 3.0f, 1.8f, trunkH - 2.f, trunkH + 7.f, kUnitHexagon, barkColor, true);
        addPrismLUT3D(triBatch, cam, (center + bRight) * 0.5f, 2.8f, 1.7f, trunkH - 2.f, trunkH + 6.f, kUnitHexagon, barkColor, true);

        sf::Color bloomBase;
        sf::Color bloomTop;

        if (level == 1)
        {
            bloomBase = sf::Color(218, 175, 42);
            bloomTop  = sf::Color(255, 218, 65);
        }
        else if (level == 2)
        {
            bloomBase = sf::Color(208, 48, 52);
            bloomTop  = sf::Color(248, 78, 70);
        }
        else
        {
            bloomBase = sf::Color(205, 52, 145);
            bloomTop  = sf::Color(248, 88, 180);
        }

        addFoliageLobe3D(triBatch, cam, center, minDim * 0.36f, trunkH + 2.f, trunkH + 13.f, trunkH + 24.f, bloomBase, sway);
        addFoliageLobe3D(triBatch, cam, bLeft, minDim * 0.27f, trunkH - 2.f, trunkH + 8.f, trunkH + 18.f, bloomTop, sway * 0.8f);
        addFoliageLobe3D(triBatch,
                         cam,
                         bRight,
                         minDim * 0.26f,
                         trunkH - 2.f,
                         trunkH + 8.f,
                         trunkH + 17.f,
                         shadeColor(bloomTop, 0.95f),
                         sway * 0.9f);
        addFoliageLobe3D(triBatch,
                         cam,
                         center + sf::Vector2f(0.f, -minDim * 0.05f),
                         minDim * 0.21f,
                         trunkH + 14.f,
                         trunkH + 23.f,
                         trunkH + 31.f,
                         shadeColor(bloomTop, 1.15f),
                         sway * 1.2f);
    }
    else
    {
        const sf::Color pineBark(58, 44, 32);

        addPrismLUT3D(triBatch, cam, center, 7.6f, 4.8f, 0.f, 5.f, kUnitOctagon, shadeColor(pineBark, 0.85f), true);
        addPrismLUT3D(triBatch, cam, center, 4.8f, 2.8f, 5.f, 40.f, kUnitHexagon, pineBark, true);

        sf::Color pineGreen;
        if (level == 1)
        {
            pineGreen = sf::Color(38, 98, 52);
        }
        else if (level == 2)
        {
            pineGreen = sf::Color(22, 92, 50);
        }
        else
        {
            pineGreen = sf::Color(18, 106, 75);
        }
        const sf::Color needleHighlight = shadeColor(pineGreen, 1.25f, 0.05f);

        const sf::Vector2f cPlate1 = center + sf::Vector2f(minDim * 0.22f, minDim * 0.09f);
        const sf::Vector2f dir1    = cPlate1 - center;
        const float        len1    = std::hypot(dir1.x, dir1.y);
        const float        ang1    = std::atan2(dir1.y, dir1.x) * 180.f / std::numbers::pi_v<float>;
        addOrientedBox3D(triBatch, cam, center, {len1, 2.8f}, {0.f, 1.4f}, 16.f, 20.f, ang1, pineBark);
        addFoliageLobe3D(triBatch, cam, cPlate1, minDim * 0.22f, 16.f, 22.f, 28.f, pineGreen, sway * 0.7f);

        const sf::Vector2f cPlate2 = center + sf::Vector2f(-minDim * 0.22f, -minDim * 0.05f);
        const sf::Vector2f dir2    = cPlate2 - center;
        const float        len2    = std::hypot(dir2.x, dir2.y);
        const float        ang2    = std::atan2(dir2.y, dir2.x) * 180.f / std::numbers::pi_v<float>;
        addOrientedBox3D(triBatch, cam, center, {len2, 2.6f}, {0.f, 1.3f}, 23.f, 27.f, ang2, pineBark);
        addFoliageLobe3D(triBatch, cam, cPlate2, minDim * 0.25f, 24.f, 30.f, 36.f, shadeColor(pineGreen, 0.95f), sway * 0.8f);

        const sf::Vector2f cPlate3 = center + sf::Vector2f(minDim * 0.08f, -minDim * 0.08f);
        const sf::Vector2f dir3    = cPlate3 - center;
        const float        len3    = std::hypot(dir3.x, dir3.y);
        const float        ang3    = std::atan2(dir3.y, dir3.x) * 180.f / std::numbers::pi_v<float>;
        addOrientedBox3D(triBatch, cam, center, {len3, 2.4f}, {0.f, 1.2f}, 31.f, 35.f, ang3, pineBark);
        addFoliageLobe3D(triBatch, cam, cPlate3, minDim * 0.26f, 32.f, 38.f, 44.f, pineGreen, sway);

        addFoliageLobe3D(triBatch,
                         cam,
                         center + sf::Vector2f(0.f, -minDim * 0.02f),
                         minDim * 0.22f,
                         38.f,
                         46.f,
                         54.f,
                         needleHighlight,
                         sway * 1.2f);
    }
}

void addFlatRingArc3D(
    sf::VertexArray& va,
    const Camera3D&  cam,
    sf::Vector2f     center,
    float            rIn,
    float            rOut,
    float            startAngle,
    float            endAngle,
    float            z,
    sf::Color        col,
    int              numSteps = 5)
{
    float span = endAngle - startAngle;
    while (span < 0.f)
    {
        span += 2.f * std::numbers::pi_v<float>;
    }
    if (span <= 0.02f)
    {
        return;
    }
    const float step = span / static_cast<float>(numSteps);
    for (int i = 0; i < numSteps; ++i)
    {
        const float        a1 = startAngle + static_cast<float>(i) * step;
        const float        a2 = a1 + step;
        const sf::Vector2f dir1(std::cos(a1), std::sin(a1));
        const sf::Vector2f dir2(std::cos(a2), std::sin(a2));
        const sf::Vector2f in1  = cam.project(center + dir1 * rIn, z);
        const sf::Vector2f out1 = cam.project(center + dir1 * rOut, z);
        const sf::Vector2f in2  = cam.project(center + dir2 * rIn, z);
        const sf::Vector2f out2 = cam.project(center + dir2 * rOut, z);

        va.append(sf::Vertex(in1, col));
        va.append(sf::Vertex(out1, col));
        va.append(sf::Vertex(out2, col));
        va.append(sf::Vertex(out2, col));
        va.append(sf::Vertex(in2, col));
        va.append(sf::Vertex(in1, col));
    }
}

void drawRoads3D(sf::VertexArray& triBatch, const Camera3D& cam, const std::vector<GameConfig::RoadSegment>& roads, int level)
{
    if (roads.empty())
    {
        return;
    }

    sf::Color roadBedColor;
    sf::Color roadBorderColor;
    sf::Color roadCenterColor;
    sf::Color plazaRingColor;
    sf::Color plazaBedColor;
    sf::Color plazaEmblemColor;

    if (level == 1)
    {
        roadBedColor     = sf::Color(205, 172, 125, 235);
        roadBorderColor  = sf::Color(145, 112, 72, 240);
        roadCenterColor  = sf::Color(235, 210, 160, 220);
        plazaRingColor   = sf::Color(165, 125, 80, 245);
        plazaBedColor    = sf::Color(225, 195, 150, 240);
        plazaEmblemColor = sf::Color(190, 135, 45, 245);
    }
    else if (level == 2)
    {
        roadBedColor     = sf::Color(220, 185, 130, 235);
        roadBorderColor  = sf::Color(160, 120, 72, 240);
        roadCenterColor  = sf::Color(240, 212, 160, 220);
        plazaRingColor   = sf::Color(175, 132, 80, 245);
        plazaBedColor    = sf::Color(225, 192, 140, 240);
        plazaEmblemColor = sf::Color(195, 140, 50, 245);
    }
    else
    {
        roadBedColor     = sf::Color(48, 52, 58, 245);
        roadBorderColor  = sf::Color(180, 186, 192, 255);
        roadCenterColor  = sf::Color(245, 218, 60, 230);
        plazaRingColor   = sf::Color(130, 138, 148, 255);
        plazaBedColor    = sf::Color(65, 70, 78, 255);
        plazaEmblemColor = sf::Color(45, 175, 80, 255);
    }

    constexpr float kRoadZ = 0.12f;

    struct JunctionBranch
    {
        float angle;
        float halfAngleWidth;
        float width;
    };

    struct JunctionInfo
    {
        sf::Vector2f                pos;
        float                       radius         = 0.f;
        bool                        isContinuation = false;
        std::vector<JunctionBranch> branches;
    };

    auto ptKey = [](sf::Vector2f p) -> uint64_t
    {
        const auto ix = static_cast<int32_t>(std::round(p.x * 0.5f));
        const auto iy = static_cast<int32_t>(std::round(p.y * 0.5f));
        return (static_cast<uint64_t>(ix) << 32) | (static_cast<uint64_t>(iy) & 0xFFFFFFFFULL);
    };

    struct CachedRoadNetwork
    {
        int                                        level = -1;
        std::unordered_map<uint64_t, JunctionInfo> junctionMap;
    };
    static CachedRoadNetwork sRoadNetworkCache;

    if (sRoadNetworkCache.level != level)
    {
        sRoadNetworkCache.level = level;
        sRoadNetworkCache.junctionMap.clear();
        sRoadNetworkCache.junctionMap.reserve(roads.size() * 2);

        for (const auto& road : roads)
        {
            const sf::Vector2f pStart(static_cast<float>(road.start.x), static_cast<float>(road.start.y));
            const sf::Vector2f pEnd(static_cast<float>(road.end.x), static_cast<float>(road.end.y));
            const sf::Vector2f delta = pEnd - pStart;
            const float        len   = std::hypot(delta.x, delta.y);
            if (len < 1.f)
                continue;

            const uint64_t kStart = ptKey(pStart);
            auto&          jStart = sRoadNetworkCache.junctionMap[kStart];
            jStart.pos            = pStart;
            const float angStart  = std::atan2(delta.y, delta.x);
            jStart.branches.push_back({angStart, 0.f, road.width});

            const uint64_t kEnd = ptKey(pEnd);
            auto&          jEnd = sRoadNetworkCache.junctionMap[kEnd];
            jEnd.pos            = pEnd;
            const float angEnd  = std::atan2(-delta.y, -delta.x);
            jEnd.branches.push_back({angEnd, 0.f, road.width});
        }

        for (auto& [key, junc] : sRoadNetworkCache.junctionMap)
        {
            if (junc.branches.size() < 2)
                continue;

            if (junc.branches.size() == 2)
            {
                float diff = std::abs(junc.branches[0].angle - junc.branches[1].angle);
                if (diff > std::numbers::pi_v<float>)
                {
                    diff = 2.f * std::numbers::pi_v<float> - diff;
                }
                if (diff >= 165.f * (std::numbers::pi_v<float> / 180.f))
                {
                    junc.isContinuation = true;
                    continue;
                }
            }

            float maxW = 0.f;
            for (const auto& b : junc.branches)
            {
                maxW = std::max(maxW, b.width);
            }
            junc.radius = maxW * 0.54f;

            for (auto& b : junc.branches)
            {
                const float ratio = std::clamp((b.width * 0.5f) / junc.radius, 0.f, 0.95f);
                b.halfAngleWidth  = std::asin(ratio);
            }

            std::sort(junc.branches.begin(),
                      junc.branches.end(),
                      [](const JunctionBranch& a, const JunctionBranch& b) { return a.angle < b.angle; });
        }
    }

    const auto&         junctionMap = sRoadNetworkCache.junctionMap;
    const sf::FloatRect camBounds   = cam.getGroundFrustumBounds(100.f);
    const float         camLeft     = camBounds.position.x;
    const float         camRight    = camLeft + camBounds.size.x;
    const float         camTop      = camBounds.position.y;
    const float         camBottom   = camTop + camBounds.size.y;

    for (const auto& road : roads)
    {
        const sf::Vector2f rStart(static_cast<float>(road.start.x), static_cast<float>(road.start.y));
        const sf::Vector2f rEnd(static_cast<float>(road.end.x), static_cast<float>(road.end.y));
        const float        halfW = road.width * 0.5f;

        const float minX = std::min(rStart.x, rEnd.x) - halfW;
        const float maxX = std::max(rStart.x, rEnd.x) + halfW;
        const float minY = std::min(rStart.y, rEnd.y) - halfW;
        const float maxY = std::max(rStart.y, rEnd.y) + halfW;
        if (maxX < camLeft || minX > camRight || maxY < camTop || minY > camBottom)
            continue;

        const sf::Vector2f delta(static_cast<float>(road.end.x - road.start.x),
                                 static_cast<float>(road.end.y - road.start.y));
        const float        len = std::hypot(delta.x, delta.y);
        if (len < 1.f)
            continue;

        const sf::Vector2f dir = delta / len;
        const sf::Vector2f perp(-dir.y, dir.x);

        const sf::Vector2f sL = rStart - perp * halfW;
        const sf::Vector2f sR = rStart + perp * halfW;
        const sf::Vector2f eL = rEnd - perp * halfW;
        const sf::Vector2f eR = rEnd + perp * halfW;

        appendQuad2D(triBatch,
                     cam.project(sL, kRoadZ),
                     cam.project(sR, kRoadZ),
                     cam.project(eR, kRoadZ),
                     cam.project(eL, kRoadZ),
                     roadBedColor,
                     roadBedColor,
                     false);
    }

    for (const auto& [key, junc] : junctionMap)
    {
        if (junc.branches.size() < 2 || junc.isContinuation)
            continue;
        if (junc.pos.x + junc.radius < camLeft || junc.pos.x - junc.radius > camRight ||
            junc.pos.y + junc.radius < camTop || junc.pos.y - junc.radius > camBottom)
            continue;
        addFlatDisc3D(triBatch, cam, junc.pos, junc.radius, kRoadZ, roadBedColor);
    }

    const float curbW = 3.5f;
    for (const auto& [key, junc] : junctionMap)
    {
        if (junc.branches.size() < 2 || junc.isContinuation)
            continue;
        if (junc.pos.x + junc.radius < camLeft || junc.pos.x - junc.radius > camRight ||
            junc.pos.y + junc.radius < camTop || junc.pos.y - junc.radius > camBottom)
            continue;
        const size_t n = junc.branches.size();
        for (size_t i = 0; i < n; ++i)
        {
            const auto& curr = junc.branches[i];
            const auto& next = junc.branches[(i + 1) % n];

            const float gapStart    = curr.angle + curr.halfAngleWidth;
            float       spanBetween = next.angle - curr.angle;
            while (spanBetween <= 0.f)
            {
                spanBetween += 2.f * std::numbers::pi_v<float>;
            }

            const float gapSpan = spanBetween - (curr.halfAngleWidth + next.halfAngleWidth);
            if (gapSpan > 0.05f)
            {
                addFlatRingArc3D(triBatch,
                                 cam,
                                 junc.pos,
                                 junc.radius - curbW,
                                 junc.radius,
                                 gapStart,
                                 gapStart + gapSpan,
                                 kRoadZ + 0.02f,
                                 roadBorderColor,
                                 5);
            }
        }
    }

    for (const auto& road : roads)
    {
        const sf::Vector2f rStart(static_cast<float>(road.start.x), static_cast<float>(road.start.y));
        const sf::Vector2f rEnd(static_cast<float>(road.end.x), static_cast<float>(road.end.y));
        const float        halfW = road.width * 0.5f;

        const float minX = std::min(rStart.x, rEnd.x) - halfW;
        const float maxX = std::max(rStart.x, rEnd.x) + halfW;
        const float minY = std::min(rStart.y, rEnd.y) - halfW;
        const float maxY = std::max(rStart.y, rEnd.y) + halfW;
        if (maxX < camLeft || minX > camRight || maxY < camTop || minY > camBottom)
            continue;
        const sf::Vector2f delta(static_cast<float>(road.end.x - road.start.x),
                                 static_cast<float>(road.end.y - road.start.y));
        const float        len = std::hypot(delta.x, delta.y);
        if (len < 1.f)
            continue;

        const sf::Vector2f dir = delta / len;
        const sf::Vector2f perp(-dir.y, dir.x);

        float tStart = 0.f;
        float tEnd   = len;

        auto itStart = junctionMap.find(ptKey(rStart));
        if (itStart != junctionMap.end() && itStart->second.branches.size() >= 2 && !itStart->second.isContinuation)
        {
            tStart = std::min(len * 0.45f, itStart->second.radius - 1.5f);
        }

        auto itEnd = junctionMap.find(ptKey(rEnd));
        if (itEnd != junctionMap.end() && itEnd->second.branches.size() >= 2 && !itEnd->second.isContinuation)
        {
            tEnd = std::max(tStart + 1.f, len - (itEnd->second.radius - 1.5f));
        }

        if (tEnd > tStart + 2.f)
        {
            const sf::Vector2f pS = rStart + dir * tStart;
            const sf::Vector2f pE = rStart + dir * tEnd;

            const sf::Vector2f sL    = pS - perp * halfW;
            const sf::Vector2f sL_in = pS - perp * (halfW - curbW);
            const sf::Vector2f eL    = pE - perp * halfW;
            const sf::Vector2f eL_in = pE - perp * (halfW - curbW);
            appendQuad2D(triBatch,
                         cam.project(sL, kRoadZ + 0.02f),
                         cam.project(sL_in, kRoadZ + 0.02f),
                         cam.project(eL_in, kRoadZ + 0.02f),
                         cam.project(eL, kRoadZ + 0.02f),
                         roadBorderColor,
                         roadBorderColor,
                         false);

            const sf::Vector2f sR_in = pS + perp * (halfW - curbW);
            const sf::Vector2f sR    = pS + perp * halfW;
            const sf::Vector2f eR_in = pE + perp * (halfW - curbW);
            const sf::Vector2f eR    = pE + perp * halfW;
            appendQuad2D(triBatch,
                         cam.project(sR_in, kRoadZ + 0.02f),
                         cam.project(sR, kRoadZ + 0.02f),
                         cam.project(eR, kRoadZ + 0.02f),
                         cam.project(eR_in, kRoadZ + 0.02f),
                         roadBorderColor,
                         roadBorderColor,
                         false);
        }
    }

    const float centerW = (level == 3) ? 3.8f : 2.2f;
    for (const auto& road : roads)
    {
        const sf::Vector2f delta(static_cast<float>(road.end.x - road.start.x),
                                 static_cast<float>(road.end.y - road.start.y));
        const float        len = std::hypot(delta.x, delta.y);
        if (len < 1.f)
            continue;

        const sf::Vector2f dir = delta / len;
        const sf::Vector2f perp(-dir.y, dir.x);

        const sf::Vector2f rStart(static_cast<float>(road.start.x), static_cast<float>(road.start.y));
        const sf::Vector2f rEnd(static_cast<float>(road.end.x), static_cast<float>(road.end.y));

        const float minX = std::min(rStart.x, rEnd.x) - centerW;
        const float maxX = std::max(rStart.x, rEnd.x) + centerW;
        const float minY = std::min(rStart.y, rEnd.y) - centerW;
        const float maxY = std::max(rStart.y, rEnd.y) + centerW;
        if (maxX < camLeft || minX > camRight || maxY < camTop || minY > camBottom)
            continue;

        float tStart = 0.f;
        float tEnd   = len;

        auto itStart = junctionMap.find(ptKey(rStart));
        if (itStart != junctionMap.end() && itStart->second.branches.size() >= 2 && !itStart->second.isContinuation)
        {
            tStart = std::min(len * 0.45f, itStart->second.radius + 4.f);
        }

        auto itEnd = junctionMap.find(ptKey(rEnd));
        if (itEnd != junctionMap.end() && itEnd->second.branches.size() >= 2 && !itEnd->second.isContinuation)
        {
            tEnd = std::max(tStart + 1.f, len - (itEnd->second.radius + 4.f));
        }

        if (tEnd > tStart + 3.f)
        {
            const sf::Vector2f pS = rStart + dir * tStart;
            const sf::Vector2f pE = rStart + dir * tEnd;

            const sf::Vector2f sC_L = pS - perp * centerW;
            const sf::Vector2f sC_R = pS + perp * centerW;
            const sf::Vector2f eC_L = pE - perp * centerW;
            const sf::Vector2f eC_R = pE + perp * centerW;

            appendQuad2D(triBatch,
                         cam.project(sC_L, kRoadZ + 0.04f),
                         cam.project(sC_R, kRoadZ + 0.04f),
                         cam.project(eC_R, kRoadZ + 0.04f),
                         cam.project(eC_L, kRoadZ + 0.04f),
                         roadCenterColor,
                         roadCenterColor,
                         false);
        }
    }

    for (const auto& [key, junc] : junctionMap)
    {
        if (junc.branches.size() >= 3 && !junc.isContinuation)
        {
            if (junc.pos.x + junc.radius < camLeft || junc.pos.x - junc.radius > camRight ||
                junc.pos.y + junc.radius < camTop || junc.pos.y - junc.radius > camBottom)
                continue;

            const float rIsland = (junc.branches.size() >= 5) ? junc.radius * 0.44f : junc.radius * 0.36f;

            addFlatRing3D(triBatch, cam, junc.pos, rIsland - 2.8f, rIsland, kRoadZ + 0.03f, plazaRingColor);

            addFlatDisc3D(triBatch, cam, junc.pos, rIsland - 2.8f, kRoadZ + 0.03f, plazaBedColor);

            addOctahedron3D(triBatch, cam, junc.pos, 2.6f, kRoadZ + 1.2f, 2.6f, 45.f, plazaEmblemColor);
        }
    }
}

const std::vector<UtilityPole>& getLevelUtilityPoles(int level)
{
    (void)level;
    static const std::vector<UtilityPole> sEmptyPoles;
    return sEmptyPoles;
}

#if 0
const std::vector<UtilityPole>& getLevelUtilityPoles_Disabled(int level)
{
    static std::array<std::vector<UtilityPole>, GameConfig::kLevelCount> sPolesCache;
    static std::array<bool, GameConfig::kLevelCount> sCached = {false, false, false};

    const int levelIdx = std::clamp(level - 1, 0, GameConfig::kLevelCount - 1);
    if (sCached[levelIdx])
    {
        return sPolesCache[levelIdx];
    }

    auto& outPoles = sPolesCache[levelIdx];
    outPoles.clear();

    const auto& roads = GameConfig::getLevelRoads(level);
    const auto& walls = GameConfig::getLevelWalls(level);
    const auto& zones = GameConfig::getLevelZones(level);

    auto collidesWithBuilding = [&](sf::Vector2f pt, float pad = 12.f) -> bool
    {
        for (const auto& w : walls)
        {
            if (pt.x >= static_cast<float>(w.pos.x) - pad &&
                pt.x <= static_cast<float>(w.pos.x + w.size.x) + pad &&
                pt.y >= static_cast<float>(w.pos.y) - pad &&
                pt.y <= static_cast<float>(w.pos.y + w.size.y) + pad)
            {
                return true;
            }
        }
        return false;
    };

    auto collidesWithTree = [&](sf::Vector2f pt, float pad = 14.f) -> bool
    {
        for (const auto& z : zones)
        {
            if (z.kind == ZoneKind::Tree)
            {
                const sf::Vector2f zCenter(static_cast<float>(z.pos.x) + static_cast<float>(z.size.x) * 0.5f,
                                           static_cast<float>(z.pos.y) + static_cast<float>(z.size.y) * 0.5f);
                if (std::hypot(pt.x - zCenter.x, pt.y - zCenter.y) < pad + static_cast<float>(z.size.x) * 0.35f)
                {
                    return true;
                }
            }
        }
        return false;
    };

    for (int rIdx = 0; rIdx < static_cast<int>(roads.size()); ++rIdx)
    {
        const auto& road = roads[rIdx];
        const sf::Vector2f start(static_cast<float>(road.start.x), static_cast<float>(road.start.y));
        const sf::Vector2f end(static_cast<float>(road.end.x), static_cast<float>(road.end.y));
        const sf::Vector2f delta = end - start;
        const float len = std::hypot(delta.x, delta.y);
        if (len < 90.f)
        {
            continue;
        }

        const sf::Vector2f dir = delta / len;
        const sf::Vector2f perp(-dir.y, dir.x);
        const float offsetDist = road.width * 0.5f + 7.0f;

        constexpr float kMargin = 38.f;
        const float span = len - 2.f * kMargin;
        if (span < 40.f)
        {
            continue;
        }

        const int nSpans = std::max(1, static_cast<int>(std::round(span / 190.f)));
        const int nPoles = nSpans + 1;
        const float step = span / static_cast<float>(nSpans);

        float preferredSide = 1.0f;
        if (std::abs(perp.y) > std::abs(perp.x))
        {
            preferredSide = (perp.y > 0.f) ? -1.0f : 1.0f;
        }
        else
        {
            preferredSide = (perp.x > 0.f) ? -1.0f : 1.0f;
        }

        std::vector<UtilityPole> segPoles;
        segPoles.reserve(nPoles);

        for (int p = 0; p < nPoles; ++p)
        {
            const float dist = kMargin + static_cast<float>(p) * step;
            const sf::Vector2f centerPt = start + dir * dist;

            float useSide = preferredSide;
            sf::Vector2f candidatePos = centerPt + perp * (useSide * offsetDist);
            if (collidesWithBuilding(candidatePos) || collidesWithTree(candidatePos))
            {
                useSide = -preferredSide;
                candidatePos = centerPt + perp * (useSide * offsetDist);
            }

            if (collidesWithBuilding(candidatePos) || collidesWithTree(candidatePos))
            {
                const sf::Vector2f shifted = candidatePos + dir * 22.f;
                if (!collidesWithBuilding(shifted) && !collidesWithTree(shifted))
                {
                    candidatePos = shifted;
                }
                else
                {
                    continue;
                }
            }

            const sf::Vector2f armDir = -perp * useSide;
            const bool hasTransformer = (p % 3 == 1);

            segPoles.push_back(UtilityPole{
                .pos            = candidatePos,
                .dir            = dir,
                .perp           = perp,
                .armDir         = armDir,
                .roadIndex      = rIdx,
                .indexInRoad    = static_cast<int>(segPoles.size()),
                .totalInRoad    = 0,
                .hasTransformer = hasTransformer
            });
        }

        const int total = static_cast<int>(segPoles.size());
        for (auto& pole : segPoles)
        {
            pole.totalInRoad = total;
            outPoles.push_back(pole);
        }
    }

    sCached[levelIdx] = true;
    return outPoles;
}
#endif

void drawStreetlightGlow3D(sf::VertexArray& triBatch, const Camera3D& cam, const std::vector<UtilityPole>& poles)
{
    constexpr float     kGroundZ     = 0.15f;
    constexpr float     kLightRadius = 18.0f;
    constexpr int       kSegments    = 16;
    constexpr sf::Color cCore(255, 230, 140, 38);
    constexpr sf::Color cEdge(255, 215, 100, 0);

    for (const auto& pole : poles)
    {
        const sf::Vector2f lightPos = pole.pos + pole.armDir * 13.5f;
        const sf::Vector2f cProj    = cam.project(lightPos, kGroundZ);

        for (int p = 0; p < kSegments; ++p)
        {
            const int   next = (p + 1) % kSegments;
            const float a1 = static_cast<float>(p) * (2.f * std::numbers::pi_v<float> / static_cast<float>(kSegments));
            const float a2 = static_cast<float>(next) * (2.f * std::numbers::pi_v<float> / static_cast<float>(kSegments));
            const sf::Vector2f pt1 = cam.project(lightPos + sf::Vector2f(std::cos(a1), std::sin(a1)) * kLightRadius, kGroundZ);
            const sf::Vector2f pt2 = cam.project(lightPos + sf::Vector2f(std::cos(a2), std::sin(a2)) * kLightRadius, kGroundZ);

            triBatch.append(sf::Vertex(cProj, cCore));
            triBatch.append(sf::Vertex(pt1, cEdge));
            triBatch.append(sf::Vertex(pt2, cEdge));
        }
    }
}

void drawSinglePole3D(sf::VertexArray& triBatch, const Camera3D& cam, const UtilityPole& pole, int level, int frame)
{
    (void)frame;
    const sf::Vector2f center = pole.pos;

    sf::Color basePlinthCol;
    sf::Color shaftCol;
    sf::Color crossarmCol;
    sf::Color insCol;
    sf::Color lampCol;

    if (level == 1)
    {
        basePlinthCol = sf::Color(145, 138, 126);
        shaftCol      = sf::Color(165, 150, 132);
        crossarmCol   = sf::Color(85, 75, 62);
        insCol        = sf::Color(242, 238, 228);
        lampCol       = sf::Color(255, 218, 125);
    }
    else if (level == 2)
    {
        basePlinthCol = sf::Color(122, 126, 130);
        shaftCol      = sf::Color(138, 142, 146);
        crossarmCol   = sf::Color(65, 70, 75);
        insCol        = sf::Color(235, 240, 248);
        lampCol       = sf::Color(255, 238, 145);
    }
    else
    {
        basePlinthCol = sf::Color(130, 134, 138);
        shaftCol      = sf::Color(148, 150, 154);
        crossarmCol   = sf::Color(60, 68, 76);
        insCol        = sf::Color(220, 242, 248);
        lampCol       = sf::Color(255, 228, 135);
    }

    addPrismLUT3D(triBatch, cam, center, 3.4f, 2.8f, 0.f, 3.5f, kUnitOctagon, basePlinthCol, true);

    addPrismLUT3D(triBatch, cam, center, 2.4f, 1.4f, 3.5f, 45.0f, kUnitOctagon, shaftCol, true);

    for (int k = 0; k < 4; ++k)
    {
        const float        rZ    = 12.f + static_cast<float>(k) * 6.5f;
        const float        rSide = (k % 2 == 0) ? 1.f : -1.f;
        const sf::Vector2f rPt   = center + pole.perp * (rSide * 2.2f);
        addBox3D(triBatch, cam, rPt - sf::Vector2f(0.8f, 0.8f), sf::Vector2f(1.6f, 1.6f), rZ, rZ + 1.2f, sf::Color(75, 80, 85));
    }

    if (pole.hasTransformer)
    {
        const sf::Vector2f transPos = center - pole.armDir * 3.2f;
        addPrismLUT3D(triBatch, cam, transPos, 2.6f, 2.6f, 28.f, 37.f, kUnitOctagon, sf::Color(80, 85, 92), true);
        addBox3D(triBatch,
                 cam,
                 (center + transPos) * 0.5f - sf::Vector2f(1.2f, 1.2f),
                 sf::Vector2f(2.4f, 2.4f),
                 31.f,
                 34.f,
                 sf::Color(55, 60, 65));
        addPrismLUT3D(triBatch,
                      cam,
                      transPos + sf::Vector2f(-1.1f, 0.f),
                      0.7f,
                      0.5f,
                      37.f,
                      39.f,
                      kUnitHexagon,
                      sf::Color(210, 215, 220),
                      true);
        addPrismLUT3D(triBatch,
                      cam,
                      transPos + sf::Vector2f(1.1f, 0.f),
                      0.7f,
                      0.5f,
                      37.f,
                      39.f,
                      kUnitHexagon,
                      sf::Color(210, 215, 220),
                      true);
    }

    const float angleDeg = std::atan2(pole.perp.y, pole.perp.x) * 180.f / std::numbers::pi_v<float>;
    addOrientedBox3D(triBatch, cam, center, sf::Vector2f(18.0f, 2.4f), sf::Vector2f(9.0f, 1.2f), 41.5f, 43.5f, angleDeg, crossarmCol);

    addWireSegment3D(triBatch,
                     cam,
                     {center.x, center.y, 36.5f},
                     {center.x - pole.perp.x * 6.5f, center.y - pole.perp.y * 6.5f, 41.5f},
                     1.4f,
                     sf::Color(65, 70, 75),
                     sf::Color(65, 70, 75));
    addWireSegment3D(triBatch,
                     cam,
                     {center.x, center.y, 36.5f},
                     {center.x + pole.perp.x * 6.5f, center.y + pole.perp.y * 6.5f, 41.5f},
                     1.4f,
                     sf::Color(65, 70, 75),
                     sf::Color(65, 70, 75));

    constexpr std::array<float, 3> kInsOffsets = {-7.0f, 0.0f, 7.0f};
    for (float u : kInsOffsets)
    {
        const sf::Vector2f insPos = center + pole.perp * u;
        addPrismLUT3D(triBatch, cam, insPos, 1.3f, 0.9f, 43.5f, 46.0f, kUnitHexagon, insCol, true);
        addPrismLUT3D(triBatch, cam, insPos, 0.5f, 0.5f, 46.0f, 46.8f, kUnitHexagon, sf::Color(160, 140, 80), true);
    }

    const sf::Vector2f lampHeadPos = center + pole.armDir * 13.5f;
    addWireSegment3D(triBatch,
                     cam,
                     {center.x, center.y, 35.5f},
                     {center.x + pole.armDir.x * 7.f, center.y + pole.armDir.y * 7.f, 38.0f},
                     1.6f,
                     sf::Color(55, 60, 65),
                     sf::Color(55, 60, 65));
    addWireSegment3D(triBatch,
                     cam,
                     {center.x + pole.armDir.x * 7.f, center.y + pole.armDir.y * 7.f, 38.0f},
                     {lampHeadPos.x, lampHeadPos.y, 36.5f},
                     1.6f,
                     sf::Color(55, 60, 65),
                     sf::Color(55, 60, 65));

    const float armAngleDeg = std::atan2(pole.armDir.y, pole.armDir.x) * 180.f / std::numbers::pi_v<float>;
    addOrientedBox3D(triBatch,
                     cam,
                     lampHeadPos,
                     sf::Vector2f(4.4f, 2.8f),
                     sf::Vector2f(2.2f, 1.4f),
                     36.2f,
                     37.8f,
                     armAngleDeg,
                     sf::Color(45, 50, 55));
    addOrientedBox3D(triBatch, cam, lampHeadPos, sf::Vector2f(3.6f, 2.2f), sf::Vector2f(1.8f, 1.1f), 35.6f, 36.2f, armAngleDeg, lampCol);
    addOctahedron3D(triBatch, cam, lampHeadPos, 1.2f, 35.8f, 0.8f, 0.f, sf::Color(255, 255, 230));
}

void drawPowerLines3D(sf::VertexArray& triBatch, const Camera3D& cam, const std::vector<UtilityPole>& poles, int level, int currentFrame)
{
    (void)triBatch;
    (void)cam;
    (void)poles;
    (void)level;
    (void)currentFrame;
}

void drawHouse3D(sf::VertexArray&     triBatch,
                 const Camera3D&      cam,
                 const CRectDragDrop& wallRect,
                 bool                 isSelected,
                 int                  currentFrame,
                 int                  level,
                 int                  graphicsLevel)
{
    const float pX     = static_cast<float>(wallRect.pos.x);
    const float W      = static_cast<float>(wallRect.size.x);
    const float H      = static_cast<float>(wallRect.size.y);
    const float minDim = std::min(W, H);

    const sf::Vector2f shadowPos  = wallRect.pos - sf::Vector2f(3.f, 1.5f);
    const sf::Vector2f shadowSize = wallRect.size + sf::Vector2f(6.f, 5.f);
    appendQuad2D(triBatch,
                 cam.project(shadowPos.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y, 0.04f),
                 cam.project(shadowPos.x, shadowPos.y, 0.04f),
                 sf::Color(0, 0, 0, 52),
                 sf::Color(0, 0, 0, 52),
                 false);

    sf::Color plinthColor;
    if (level == 1)
    {
        plinthColor = sf::Color(172, 155, 134);
    }
    else if (level == 2)
    {
        plinthColor = sf::Color(172, 155, 134);
    }
    else
    {
        plinthColor = sf::Color(145, 150, 156);
    }

    addBox3D(triBatch, cam, wallRect.pos - sf::Vector2f(2.f, 2.f), wallRect.size + sf::Vector2f(4.f, 4.f), 0.f, 4.5f, plinthColor, true);

    sf::Color wallColor;
    if (level == 1)
    {
        wallColor = isSelected ? sf::Color(245, 225, 175) : sf::Color(228, 212, 185);
    }
    else if (level == 2)
    {
        wallColor = isSelected ? sf::Color(245, 225, 175) : sf::Color(228, 212, 185);
    }
    else
    {
        wallColor = isSelected ? sf::Color(250, 245, 210) : sf::Color(232, 236, 242);
    }

    addBox3D(triBatch, cam, wallRect.pos, wallRect.size, 4.5f, 32.f, wallColor, true);

    const sf::Color trimColor = shadeColor(wallColor, 0.88f);
    const float     qS        = std::min(4.5f, minDim * 0.08f);
    addBox3D(triBatch, cam, wallRect.pos, {qS, qS}, 4.5f, 32.f, trimColor, true);
    addBox3D(triBatch, cam, {wallRect.pos.x + W - qS, wallRect.pos.y}, {qS, qS}, 4.5f, 32.f, trimColor, true);
    addBox3D(triBatch, cam, {wallRect.pos.x, wallRect.pos.y + H - qS}, {qS, qS}, 4.5f, 32.f, trimColor, true);
    addBox3D(triBatch, cam, {wallRect.pos.x + W - qS, wallRect.pos.y + H - qS}, {qS, qS}, 4.5f, 32.f, trimColor, true);

    if (W >= 135.f)
    {
        const float pX1 = wallRect.pos.x + W * 0.33f - qS * 0.5f;
        const float pX2 = wallRect.pos.x + W * 0.67f - qS * 0.5f;
        addBox3D(triBatch, cam, {pX1, wallRect.pos.y + H - qS}, {qS, qS}, 4.5f, 32.f, trimColor, true);
        addBox3D(triBatch, cam, {pX2, wallRect.pos.y + H - qS}, {qS, qS}, 4.5f, 32.f, trimColor, true);
    }
    if (H >= 135.f)
    {
        const float pY1 = wallRect.pos.y + H * 0.33f - qS * 0.5f;
        const float pY2 = wallRect.pos.y + H * 0.67f - qS * 0.5f;
        addBox3D(triBatch, cam, {wallRect.pos.x, pY1}, {qS, qS}, 4.5f, 32.f, trimColor, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - qS, pY1}, {qS, qS}, 4.5f, 32.f, trimColor, true);
        addBox3D(triBatch, cam, {wallRect.pos.x, pY2}, {qS, qS}, 4.5f, 32.f, trimColor, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - qS, pY2}, {qS, qS}, 4.5f, 32.f, trimColor, true);
    }

    const float yFront = wallRect.pos.y + H + 0.15f;

    const float doorW  = std::min(16.f, W * 0.35f);
    const float doorX0 = pX + W * 0.5f - doorW * 0.5f;
    const float doorX1 = doorX0 + doorW;

    appendQuad2D(triBatch,
                 cam.project(doorX0 - 2.f, yFront + 0.1f, 4.0f),
                 cam.project(doorX1 + 2.f, yFront + 0.1f, 4.0f),
                 cam.project(doorX1 + 2.f, yFront + 0.1f, 5.0f),
                 cam.project(doorX0 - 2.f, yFront + 0.1f, 5.0f),
                 sf::Color(145, 140, 135),
                 sf::Color(165, 160, 155),
                 false);

    appendQuad2D(triBatch,
                 cam.project(doorX0 - 1.2f, yFront, 4.5f),
                 cam.project(doorX1 + 1.2f, yFront, 4.5f),
                 cam.project(doorX1 + 1.2f, yFront, 22.5f),
                 cam.project(doorX0 - 1.2f, yFront, 22.5f),
                 sf::Color(65, 42, 28),
                 sf::Color(78, 52, 35),
                 false);

    appendQuad2D(triBatch,
                 cam.project(doorX0, yFront + 0.05f, 4.5f),
                 cam.project(doorX1, yFront + 0.05f, 4.5f),
                 cam.project(doorX1, yFront + 0.05f, 22.0f),
                 cam.project(doorX0, yFront + 0.05f, 22.0f),
                 sf::Color(95, 58, 36),
                 sf::Color(122, 75, 48),
                 false);

    const sf::Vector2f knobPt = cam.project(doorX1 - 3.2f, yFront + 0.12f, 13.0f);
    appendQuad2D(triBatch,
                 knobPt - sf::Vector2f(1.f, 1.f),
                 knobPt + sf::Vector2f(1.f, -1.f),
                 knobPt + sf::Vector2f(1.f, 1.f),
                 knobPt - sf::Vector2f(1.f, -1.f),
                 sf::Color(245, 210, 60),
                 sf::Color(255, 230, 90),
                 false);

    sf::Color canopyColor;
    if (level == 1)
        canopyColor = sf::Color(192, 92, 55);
    else if (level == 2)
        canopyColor = sf::Color(172, 68, 52);
    else
        canopyColor = sf::Color(62, 118, 148);

    addBox3D(triBatch, cam, {doorX0 - 3.5f, wallRect.pos.y + H - 1.5f}, {doorW + 7.f, 6.5f}, 22.2f, 24.5f, canopyColor, true);

    if (W >= 65.f)
    {
        auto drawWindowWithFlowers = [&](float winCenterX)
        {
            const float wHalf = std::min(6.8f, (doorX0 - pX) * 0.35f);
            const float wX0   = winCenterX - wHalf;
            const float wX1   = winCenterX + wHalf;

            appendQuad2D(triBatch,
                         cam.project(wX0 - 1.0f, yFront, 11.0f),
                         cam.project(wX1 + 1.0f, yFront, 11.0f),
                         cam.project(wX1 + 1.0f, yFront, 22.5f),
                         cam.project(wX0 - 1.0f, yFront, 22.5f),
                         sf::Color(58, 46, 38),
                         sf::Color(72, 58, 48),
                         false);

            appendQuad2D(triBatch,
                         cam.project(wX0, yFront + 0.05f, 12.0f),
                         cam.project(wX1, yFront + 0.05f, 12.0f),
                         cam.project(wX1, yFront + 0.05f, 21.5f),
                         cam.project(wX0, yFront + 0.05f, 21.5f),
                         sf::Color(242, 192, 98, 240),
                         sf::Color(255, 232, 142, 245),
                         false);

            appendQuad2D(triBatch,
                         cam.project(wX0, yFront + 0.1f, 16.5f),
                         cam.project(wX1, yFront + 0.1f, 16.5f),
                         cam.project(wX1, yFront + 0.1f, 17.5f),
                         cam.project(wX0, yFront + 0.1f, 17.5f),
                         sf::Color(58, 46, 38),
                         sf::Color(58, 46, 38),
                         false);
            appendQuad2D(triBatch,
                         cam.project(winCenterX - 0.5f, yFront + 0.1f, 12.0f),
                         cam.project(winCenterX + 0.5f, yFront + 0.1f, 12.0f),
                         cam.project(winCenterX + 0.5f, yFront + 0.1f, 21.5f),
                         cam.project(winCenterX - 0.5f, yFront + 0.1f, 21.5f),
                         sf::Color(58, 46, 38),
                         sf::Color(58, 46, 38),
                         false);

            appendQuad2D(triBatch,
                         cam.project(wX0 - 1.2f, yFront + 0.12f, 8.5f),
                         cam.project(wX1 + 1.2f, yFront + 0.12f, 8.5f),
                         cam.project(wX1 + 1.2f, yFront + 0.12f, 11.2f),
                         cam.project(wX0 - 1.2f, yFront + 0.12f, 11.2f),
                         sf::Color(95, 62, 40),
                         sf::Color(115, 75, 48),
                         false);

            appendQuad2D(triBatch,
                         cam.project(wX0 - 0.5f, yFront + 0.18f, 10.8f),
                         cam.project(wX1 + 0.5f, yFront + 0.18f, 10.8f),
                         cam.project(wX1 + 0.5f, yFront + 0.18f, 12.2f),
                         cam.project(wX0 - 0.5f, yFront + 0.18f, 12.2f),
                         sf::Color(42, 125, 48),
                         sf::Color(65, 168, 62),
                         false);

            const sf::Vector2f flw1 = cam.project(winCenterX - 3.5f, yFront + 0.22f, 12.0f);
            const sf::Vector2f flw2 = cam.project(winCenterX + 3.5f, yFront + 0.22f, 12.0f);
            appendQuad2D(triBatch,
                         flw1 - sf::Vector2f(1.f, 1.f),
                         flw1 + sf::Vector2f(1.f, -1.f),
                         flw1 + sf::Vector2f(1.f, 1.f),
                         flw1 - sf::Vector2f(1.f, -1.f),
                         sf::Color(245, 65, 80),
                         sf::Color(255, 110, 130),
                         false);
            appendQuad2D(triBatch,
                         flw2 - sf::Vector2f(1.f, 1.f),
                         flw2 + sf::Vector2f(1.f, -1.f),
                         flw2 + sf::Vector2f(1.f, 1.f),
                         flw2 - sf::Vector2f(1.f, -1.f),
                         sf::Color(255, 215, 50),
                         sf::Color(255, 240, 90),
                         false);
        };

        if (W >= 135.f)
        {
            const float leftSpace = doorX0 - pX;
            drawWindowWithFlowers(pX + leftSpace * 0.30f);
            drawWindowWithFlowers(pX + leftSpace * 0.72f);

            const float rightSpace = pX + W - doorX1;
            drawWindowWithFlowers(doorX1 + rightSpace * 0.28f);
            drawWindowWithFlowers(doorX1 + rightSpace * 0.70f);
        }
        else
        {
            const float leftWinCenter  = pX + (doorX0 - pX) * 0.5f;
            const float rightWinCenter = doorX1 + (pX + W - doorX1) * 0.5f;
            drawWindowWithFlowers(leftWinCenter);
            drawWindowWithFlowers(rightWinCenter);
        }
    }

    sf::Color roofColor;
    if (level == 1)
    {
        roofColor = sf::Color(192, 92, 55);
    }
    else if (level == 2)
    {
        roofColor = sf::Color(192, 92, 55);
    }
    else
    {
        roofColor = sf::Color(65, 75, 88);
    }

    const float inset1 = std::min(3.0f, minDim * 0.05f);
    const float inset2 = std::min(8.0f, minDim * 0.12f);
    const float inset3 = std::min(14.0f, minDim * 0.22f);
    const float inset4 = std::min(20.0f, minDim * 0.32f);

    addBox3D(triBatch,
             cam,
             wallRect.pos - sf::Vector2f(3.5f, 3.5f),
             wallRect.size + sf::Vector2f(7.f, 7.f),
             32.f,
             35.5f,
             shadeColor(roofColor, 0.78f),
             true);

    addBox3D(triBatch,
             cam,
             wallRect.pos + sf::Vector2f(inset1, inset1),
             wallRect.size - sf::Vector2f(inset1 * 2.f, inset1 * 2.f),
             35.5f,
             41.5f,
             roofColor,
             true);

    addBox3D(triBatch,
             cam,
             wallRect.pos + sf::Vector2f(inset2, inset2),
             wallRect.size - sf::Vector2f(inset2 * 2.f, inset2 * 2.f),
             41.5f,
             46.5f,
             shadeColor(roofColor, 1.06f),
             true);

    addBox3D(triBatch,
             cam,
             wallRect.pos + sf::Vector2f(inset3, inset3),
             wallRect.size - sf::Vector2f(inset3 * 2.f, inset3 * 2.f),
             46.5f,
             50.5f,
             shadeColor(roofColor, 1.12f),
             true);

    if (W >= H)
    {
        const float ridgeLen = std::max(8.f, W - inset4 * 2.f);
        addBox3D(triBatch,
                 cam,
                 {wallRect.pos.x + (W - ridgeLen) * 0.5f, wallRect.pos.y + H * 0.5f - 3.5f},
                 {ridgeLen, 7.f},
                 50.5f,
                 52.5f,
                 shadeColor(roofColor, 1.18f),
                 true);
    }
    else
    {
        const float ridgeLen = std::max(8.f, H - inset4 * 2.f);
        addBox3D(triBatch,
                 cam,
                 {wallRect.pos.x + W * 0.5f - 3.5f, wallRect.pos.y + (H - ridgeLen) * 0.5f},
                 {7.f, ridgeLen},
                 50.5f,
                 52.5f,
                 shadeColor(roofColor, 1.18f),
                 true);
    }

    std::vector<sf::Vector2f> chimneys;
    if (W >= 135.f)
    {
        chimneys.push_back({wallRect.pos.x + W * 0.22f, wallRect.pos.y + 8.f});
        chimneys.push_back({wallRect.pos.x + W * 0.78f, wallRect.pos.y + 8.f});
    }
    else if (H >= 135.f)
    {
        chimneys.push_back({wallRect.pos.x + W * 0.76f, wallRect.pos.y + 12.f});
        chimneys.push_back({wallRect.pos.x + W * 0.76f, wallRect.pos.y + H - 24.f});
    }
    else
    {
        chimneys.push_back({wallRect.pos.x + W * 0.76f, wallRect.pos.y + 8.f});
    }

    for (size_t cIdx = 0; cIdx < chimneys.size(); ++cIdx)
    {
        const sf::Vector2f chimPos = chimneys[cIdx];
        addBox3D(triBatch, cam, chimPos, {9.f, 9.f}, 35.5f, 56.f, sf::Color(145, 58, 42), true);
        addBox3D(triBatch, cam, chimPos - sf::Vector2f(1.f, 1.f), {11.f, 11.f}, 56.f, 58.5f, sf::Color(72, 70, 68), true);
    }

    if (isSelected)
    {
        const sf::Color selCol(160, 255, 240, 235);
        const float     bSz  = 6.f;
        const float     topZ = 53.f;
        addBox3D(triBatch, cam, wallRect.pos - sf::Vector2f(2.f, 2.f), {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - bSz + 2.f, wallRect.pos.y - 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x - 2.f, wallRect.pos.y + H - bSz + 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - bSz + 2.f, wallRect.pos.y + H - bSz + 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
    }
}

void drawMilitaryFacility3D(sf::VertexArray&     triBatch,
                            const Camera3D&      cam,
                            const CRectDragDrop& wallRect,
                            const CWall*         wallComponent,
                            bool                 isSelected,
                            int                  currentFrame,
                            int                  level)
{
    (void)level;
    const float pX     = static_cast<float>(wallRect.pos.x);
    const float W      = static_cast<float>(wallRect.size.x);
    const float H      = static_cast<float>(wallRect.size.y);
    const float minDim = std::min(W, H);

    const sf::Vector2f shadowPos  = wallRect.pos - sf::Vector2f(3.f, 1.5f);
    const sf::Vector2f shadowSize = wallRect.size + sf::Vector2f(6.f, 5.f);
    appendQuad2D(triBatch,
                 cam.project(shadowPos.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y + shadowSize.y, 0.04f),
                 cam.project(shadowPos.x + shadowSize.x, shadowPos.y, 0.04f),
                 cam.project(shadowPos.x, shadowPos.y, 0.04f),
                 sf::Color(0, 0, 0, 55),
                 sf::Color(0, 0, 0, 55),
                 false);

    addBox3D(triBatch,
             cam,
             wallRect.pos - sf::Vector2f(2.f, 2.f),
             wallRect.size + sf::Vector2f(4.f, 4.f),
             0.f,
             4.0f,
             sf::Color(82, 86, 92),
             true);

    const sf::Color wallColor = getWallColor(wallComponent, isSelected);
    addBox3D(triBatch, cam, wallRect.pos, wallRect.size, 4.0f, 34.0f, wallColor, false);

    const sf::Color    deckColor(55, 60, 68);
    const sf::Vector2f t01 = cam.project(wallRect.pos.x, wallRect.pos.y + H, 34.0f);
    const sf::Vector2f t11 = cam.project(wallRect.pos.x + W, wallRect.pos.y + H, 34.0f);
    const sf::Vector2f t10 = cam.project(wallRect.pos.x + W, wallRect.pos.y, 34.0f);
    const sf::Vector2f t00 = cam.project(wallRect.pos.x, wallRect.pos.y, 34.0f);
    appendQuad2D(triBatch, t01, t11, t10, t00, deckColor, deckColor, false);

    const float     pThick     = std::min(3.5f, minDim * 0.08f);
    const sf::Color parapetCol = shadeColor(wallColor, 1.12f, 0.05f);
    addBox3D(triBatch, cam, wallRect.pos, {W, pThick}, 34.0f, 37.5f, parapetCol, true);
    addBox3D(triBatch, cam, {wallRect.pos.x, wallRect.pos.y + H - pThick}, {W, pThick}, 34.0f, 37.5f, parapetCol, true);
    addBox3D(triBatch, cam, {wallRect.pos.x, wallRect.pos.y + pThick}, {pThick, H - pThick * 2.f}, 34.0f, 37.5f, parapetCol, true);
    addBox3D(triBatch,
             cam,
             {wallRect.pos.x + W - pThick, wallRect.pos.y + pThick},
             {pThick, H - pThick * 2.f},
             34.0f,
             37.5f,
             parapetCol,
             true);

    const float yFront = wallRect.pos.y + H + 0.15f;

    const float blastDoorW  = std::min(22.f, W * 0.35f);
    const float blastDoorX0 = pX + W * 0.5f - blastDoorW * 0.5f;
    const float blastDoorX1 = blastDoorX0 + blastDoorW;

    appendQuad2D(triBatch,
                 cam.project(blastDoorX0 - 1.5f, yFront, 4.0f),
                 cam.project(blastDoorX1 + 1.5f, yFront, 4.0f),
                 cam.project(blastDoorX1 + 1.5f, yFront, 22.5f),
                 cam.project(blastDoorX0 - 1.5f, yFront, 22.5f),
                 sf::Color(45, 48, 54),
                 sf::Color(58, 62, 70),
                 false);

    appendQuad2D(triBatch,
                 cam.project(blastDoorX0, yFront + 0.05f, 4.0f),
                 cam.project(blastDoorX1, yFront + 0.05f, 4.0f),
                 cam.project(blastDoorX1, yFront + 0.05f, 22.0f),
                 cam.project(blastDoorX0, yFront + 0.05f, 22.0f),
                 sf::Color(65, 70, 78),
                 sf::Color(82, 88, 98),
                 false);

    appendQuad2D(triBatch,
                 cam.project(wallRect.pos.x + W * 0.5f - 0.7f, yFront + 0.08f, 4.0f),
                 cam.project(wallRect.pos.x + W * 0.5f + 0.7f, yFront + 0.08f, 4.0f),
                 cam.project(wallRect.pos.x + W * 0.5f + 0.7f, yFront + 0.08f, 22.0f),
                 cam.project(wallRect.pos.x + W * 0.5f - 0.7f, yFront + 0.08f, 22.0f),
                 sf::Color(35, 38, 42),
                 sf::Color(35, 38, 42),
                 false);

    appendQuad2D(triBatch,
                 cam.project(blastDoorX0 + 2.f, yFront + 0.1f, 8.0f),
                 cam.project(blastDoorX1 - 2.f, yFront + 0.1f, 8.0f),
                 cam.project(blastDoorX1 - 2.f, yFront + 0.1f, 10.5f),
                 cam.project(blastDoorX0 + 2.f, yFront + 0.1f, 10.5f),
                 sf::Color(220, 185, 30),
                 sf::Color(240, 205, 40),
                 false);

    const sf::Color    lampCol = (wallComponent && wallComponent->kind == WallKind::Gun)    ? sf::Color(255, 60, 60)
                                 : (wallComponent && wallComponent->kind == WallKind::Fire) ? sf::Color(255, 140, 30)
                                 : (wallComponent && wallComponent->kind == WallKind::Electric)
                                     ? sf::Color(0, 210, 255)
                                     : sf::Color(40, 240, 120);
    const sf::Vector2f lampPt  = cam.project(wallRect.pos.x + W * 0.5f, yFront + 0.12f, 24.5f);
    appendQuad2D(triBatch,
                 lampPt - sf::Vector2f(2.f, 2.f),
                 lampPt + sf::Vector2f(2.f, -2.f),
                 lampPt + sf::Vector2f(2.f, 2.f),
                 lampPt - sf::Vector2f(2.f, -2.f),
                 lampCol,
                 lampCol,
                 false);

    if (W >= 70.f)
    {
        auto drawSlitWindow = [&](float cx)
        {
            const float wW = std::min(8.5f, (blastDoorX0 - pX) * 0.35f);
            appendQuad2D(triBatch,
                         cam.project(cx - wW - 0.8f, yFront, 15.0f),
                         cam.project(cx + wW + 0.8f, yFront, 15.0f),
                         cam.project(cx + wW + 0.8f, yFront, 20.5f),
                         cam.project(cx - wW - 0.8f, yFront, 20.5f),
                         sf::Color(40, 44, 50),
                         sf::Color(48, 52, 60),
                         false);
            appendQuad2D(triBatch,
                         cam.project(cx - wW, yFront + 0.05f, 15.8f),
                         cam.project(cx + wW, yFront + 0.05f, 15.8f),
                         cam.project(cx + wW, yFront + 0.05f, 19.8f),
                         cam.project(cx - wW, yFront + 0.05f, 19.8f),
                         sf::Color(40, 195, 185, 230),
                         sf::Color(80, 245, 220, 245),
                         false);
        };

        const float leftSlitX  = pX + (blastDoorX0 - pX) * 0.5f;
        const float rightSlitX = blastDoorX1 + (pX + W - blastDoorX1) * 0.5f;
        drawSlitWindow(leftSlitX);
        drawSlitWindow(rightSlitX);
    }

    const sf::Vector2f hvacPos{wallRect.pos.x + pThick + 4.f, wallRect.pos.y + pThick + 4.f};
    addBox3D(triBatch, cam, hvacPos, {16.f, 12.f}, 34.0f, 40.5f, sf::Color(120, 126, 134), true);
    addFlatDisc3D(triBatch, cam, hvacPos + sf::Vector2f(8.f, 6.f), 4.2f, 40.6f, sf::Color(65, 70, 78));

    if (isSelected)
    {
        const sf::Color selCol(160, 255, 240, 235);
        const float     bSz  = 6.f;
        const float     topZ = 40.f;
        addBox3D(triBatch, cam, wallRect.pos - sf::Vector2f(2.f, 2.f), {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - bSz + 2.f, wallRect.pos.y - 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x - 2.f, wallRect.pos.y + H - bSz + 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
        addBox3D(triBatch, cam, {wallRect.pos.x + W - bSz + 2.f, wallRect.pos.y + H - bSz + 2.f}, {bSz, bSz}, 0.f, topZ, selCol, true);
    }
}

void drawWall(Game*                game,
              const Camera3D&      cam,
              const CRectDragDrop& wallRect,
              const CWall*         wallComponent,
              bool                 isSelected,
              int                  currentFrame,
              DeterministicRandom& random,
              sf::VertexArray&     triBatch,
              sf::VertexArray&     lineBatch,
              int                  level)
{
    (void)lineBatch;

    if (wallComponent && wallComponent->kind == WallKind::Rock)
    {
        drawRock3D(triBatch, cam, wallRect, level, currentFrame);
        return;
    }

    if (wallComponent && wallComponent->kind == WallKind::Defensive)
    {
        drawHouse3D(triBatch, cam, wallRect, isSelected, currentFrame, level, game->mGraphicsLevel);
        return;
    }

    const bool         isPortal = wallComponent && wallComponent->kind == WallKind::DesignatedPoint;
    const sf::Vector2f center   = wallRect.pos + wallRect.size * 0.5f;

    if (isPortal)
    {
        if (isSelected)
        {
            const sf::Color selCol(160, 255, 240, 235);
            addPrismLUT3D(triBatch, cam, center, 24.f, 21.f, 0.f, 2.5f, kUnitHexagon, selCol, false);
        }
    }
    else
    {
        drawMilitaryFacility3D(triBatch, cam, wallRect, wallComponent, isSelected, currentFrame, level);
    }

    const float capHeight = 34.0f;

    if (wallComponent && wallComponent->kind == WallKind::Gun)
    {
        addPrismLUT3D(triBatch, cam, center, 16.f, 13.5f, capHeight, capHeight + 16.f, kUnitOctagon, sf::Color(125, 132, 150), true);

        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {32.f, 10.f},
                         {4.f, 5.0f},
                         capHeight + 6.f,
                         capHeight + 17.f,
                         wallComponent->gunAngle,
                         sf::Color(88, 95, 112));

        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {7.f, 13.f},
                         {-25.f, 6.5f},
                         capHeight + 5.f,
                         capHeight + 18.f,
                         wallComponent->gunAngle,
                         sf::Color(180, 190, 210));

        addPrismLUT3D(triBatch, cam, center, 11.5f, 8.f, capHeight + 16.f, capHeight + 25.f, kUnitOctagon, sf::Color(168, 178, 198), true);

        if (game->mGraphicsLevel >= 1)
        {
            addOrientedBox3D(triBatch,
                             cam,
                             center,
                             {48.f, 1.8f},
                             {-28.f, 0.9f},
                             capHeight + 10.f,
                             capHeight + 12.f,
                             wallComponent->gunAngle,
                             sf::Color(255, 60, 60, 145));
        }
    }
    else if (wallComponent && wallComponent->kind == WallKind::Electric)
    {
        addPrismLUT3D(triBatch, cam, center, 14.f, 7.5f, capHeight, capHeight + 38.f, kUnitOctagon, sf::Color(70, 150, 215), true);

        for (int ringIdx = 0; ringIdx < 3; ++ringIdx)
        {
            const float rZ   = capHeight + 7.f + ringIdx * 11.f;
            const float rRad = 16.5f - ringIdx * 2.2f;
            addPrismLUT3D(triBatch, cam, center, rRad, rRad * 0.94f, rZ, rZ + 4.5f, kUnitHexagon, sf::Color(160, 230, 255), true);
        }

        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {22.f, 4.0f},
                         {0.f, -5.0f},
                         capHeight + 12.f,
                         capHeight + 19.f,
                         wallComponent->gunAngle,
                         sf::Color(200, 245, 255));
        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {22.f, 4.0f},
                         {0.f, 9.0f},
                         capHeight + 12.f,
                         capHeight + 19.f,
                         wallComponent->gunAngle,
                         sf::Color(200, 245, 255));
    }
    else if (wallComponent && wallComponent->kind == WallKind::Fire)
    {
        addPrismLUT3D(triBatch, cam, center, 16.f, 13.5f, capHeight, capHeight + 22.f, kUnitCircle12, sf::Color(175, 55, 30), true);

        addPrismLUT3D(triBatch,
                      cam,
                      center,
                      10.5f,
                      8.5f,
                      capHeight + 22.f,
                      capHeight + 26.f,
                      kUnitHexagon,
                      sf::Color(255, 165, 30),
                      true);

        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {36.f, 7.0f},
                         {18.f, 3.5f},
                         capHeight + 22.f,
                         capHeight + 29.5f,
                         wallComponent->spinAngle,
                         sf::Color(255, 125, 55));
        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {36.f, 7.0f},
                         {18.f, 3.5f},
                         capHeight + 22.f,
                         capHeight + 29.5f,
                         wallComponent->spinAngle + 90.f,
                         sf::Color(255, 125, 55));

        addPrismLUT3D(triBatch, cam, center, 6.5f, 4.5f, capHeight + 29.5f, capHeight + 36.f, kUnitOctagon, sf::Color(90, 30, 20), true);
        addOctahedron3D(triBatch, cam, center, 6.0f, capHeight + 43.f, 9.0f, wallComponent->spinAngle * 1.5f, sf::Color(255, 200, 50));
    }
    else if (wallComponent && wallComponent->kind == WallKind::Radar)
    {
        addPrismLUT3D(triBatch, cam, center, 9.f, 5.5f, capHeight, capHeight + 26.f, kUnitOctagon, sf::Color(70, 140, 105), true);

        addPrismLUT3D(triBatch,
                      cam,
                      center,
                      6.5f,
                      18.5f,
                      capHeight + 22.f,
                      capHeight + 33.f,
                      kUnitCircle12,
                      sf::Color(90, 225, 145, 215),
                      false);

        addOrientedBox3D(triBatch,
                         cam,
                         center,
                         {26.f, 4.2f},
                         {6.f, 2.1f},
                         capHeight + 31.f,
                         capHeight + 36.f,
                         wallComponent->spinAngle,
                         sf::Color(195, 255, 215));

        const float        rad    = wallComponent->spinAngle * std::numbers::pi_v<float> / 180.f;
        const sf::Vector2f tipPos = center + sf::Vector2f(std::cos(rad), std::sin(rad)) * 16.f;
        addOctahedron3D(triBatch, cam, tipPos, 3.8f, capHeight + 36.f, 5.5f, wallComponent->spinAngle, sf::Color(180, 255, 210));

        addOctahedron3D(triBatch, cam, center, 6.0f, capHeight + 41.f, 8.5f, wallComponent->spinAngle * 2.f, sf::Color(130, 255, 175));
    }
    else if (wallComponent && wallComponent->kind == WallKind::DesignatedPoint)
    {
        const float blinkPhase = (float)currentFrame * 0.12f;
        const float blinkPulse = std::sin(blinkPhase) * 0.5f + 0.5f;

        constexpr float baseZ = 0.f;

        addPrismLUT3D(triBatch, cam, center, 20.f, 17.5f, baseZ, baseZ + 6.f, kUnitHexagon, sf::Color(255, 215, 35), true);

        for (int c = 0; c < 4; ++c)
        {
            const float        ang = (c * 90.f + 45.f) * std::numbers::pi_v<float> / 180.f;
            const sf::Vector2f pPos{center.x + std::cos(ang) * 17.5f - 2.5f, center.y + std::sin(ang) * 17.5f - 2.5f};
            addBox3D(triBatch, cam, pPos, {5.f, 5.f}, baseZ, baseZ + 16.f, sf::Color(235, 185, 30), true);
        }

        for (int tier = 0; tier < 3; ++tier)
        {
            const float   tierZ     = baseZ + 10.f + tier * 10.f + blinkPulse * 3.5f;
            const float   tierR     = 14.5f - tier * 2.2f;
            const uint8_t ringAlpha = static_cast<uint8_t>(140 + blinkPulse * 100 - tier * 20);
            addPrismLUT3D(triBatch,
                          cam,
                          center,
                          tierR,
                          tierR * 0.92f,
                          tierZ,
                          tierZ + 3.5f,
                          kUnitOctagon,
                          sf::Color(255, 235, 110, ringAlpha),
                          false);
        }

        addOctahedron3D(triBatch, cam, center, 7.5f, baseZ + 32.f + blinkPulse * 4.5f, 16.f, 0.f, sf::Color(255, 240, 130));
    }
}

} // namespace ViewStateDetail
