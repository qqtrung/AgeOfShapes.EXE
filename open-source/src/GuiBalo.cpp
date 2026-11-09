#include "GuiBalo.hpp"

#include "ClientGame.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "GameStateDetail.hpp"
#include "Profiler.hpp"
#include "TextUtil.hpp"
#include "ViewState.hpp"
#include "ViewStateDetail.hpp"

GuiBalo::Slot::Slot(sf::Font& font) : rect({90.f, 90.f}), stockText(font, "", TextUtil::kBaseCharSize)
{
    TextUtil::setSize(stockText, 16);
    rect.setFillColor(sf::Color(30, 35, 45, 200));
    stockText.setFillColor(sf::Color::White);
}

GuiBalo::Slot::~Slot()
{
    logout("~Slot()");
}

void GuiBalo::Slot::update()
{
    PROFILE_FUNCTION();
}

bool GuiBalo::Slot::input(const sf::Event& event)
{
    auto* guiBalo = dynamic_cast<GuiBalo*>(parent);
    if (!guiBalo)
    {
        return false;
    }

    auto* client = dynamic_cast<ClientGame*>(guiBalo->parent);
    if (!client || client->isGameOver())
    {
        return false;
    }

    sf::Vector2i pixelPos;
    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Left)
        {
            pixelPos = mouse->position;
        }
        else
        {
            return false;
        }
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
    {
        pixelPos = touch->position;
    }
    else
    {
        return false;
    }

    auto&        window = client->window();
    sf::Vector2f uiPos  = window.mapPixelToCoords(pixelPos, client->mViewState->uiView());

    float width  = 2532.f;
    float height = 1170.f;

    float scale         = 1.55f;
    float scaledSpacing = 120.f * scale;
    float scaledSlotW   = 90.f * scale;
    float totalWidth    = scaledSpacing * 5.f + scaledSlotW;

    float startX = (width - totalWidth) / 2.f;
    float startY = height - 120.f * scale;

    const int index = mIndex;

    sf::Vector2f  pos = {startX + index * scaledSpacing, startY};
    sf::FloatRect slotRect(pos, {scaledSlotW, scaledSlotW});
    if (!slotRect.contains(uiPos))
    {
        return false;
    }

    const bool unitMode = client->mViewState->input().build.palette == PlacementPalette::Unit;
    Action     action   = unitMode ? Action(Action::Type::SelectUnitClass, mUnitClass)
                                   : Action(Action::Type::SelectWallKind, mKind);
    pushAction(action);
    return true;
}

void GuiBalo::Slot::render()
{
    PROFILE_FUNCTION();
    auto* guiBalo = dynamic_cast<GuiBalo*>(parent);
    if (!guiBalo)
    {
        return;
    }

    auto* client = dynamic_cast<ClientGame*>(guiBalo->parent);
    if (!client)
    {
        return;
    }

    auto& window = client->window();

    float width  = 2532.f;
    float height = 1170.f;

    float scale         = 1.55f;
    float scaledSpacing = 120.f * scale;
    float scaledSlotW   = 90.f * scale;
    float totalWidth    = scaledSpacing * 5.f + scaledSlotW;

    float startX = (width - totalWidth) / 2.f;
    float startY = height - 120.f * scale;

    const int index = mIndex;

    sf::Vector2f pos = {startX + index * scaledSpacing, startY};

    const bool unitMode = client->mViewState->input().build.palette == PlacementPalette::Unit;
    int        stock    = 0;
    const int  playerId = client->mViewState->playerEntityId();
    if (const int* pStock = unitMode ? GameStateDetail::getUnitStock(&client->gameState(), mUnitClass, playerId)
                                     : GameStateDetail::getWallStock(&client->gameState(), mKind, playerId))
    {
        stock = *pStock;
    }

    CWall     wallData  = GameStateDetail::createBuildWallData(mKind);
    sf::Color wallColor = unitMode ? GameConfig::Visual::kUnitClassColors[static_cast<int>(mUnitClass)]
                                   : ViewStateDetail::getWallColor(&wallData, false);

    bool selected = client->mViewState->input().build.placementMode &&
                    (unitMode ? client->mViewState->input().build.selectedUnitClass == mUnitClass
                              : client->mViewState->input().build.selectedKind == mKind);

    sf::Vector2f  mpos = window.mapPixelToCoords(sf::Mouse::getPosition(window), client->mViewState->uiView());
    sf::FloatRect slotRect(pos, {scaledSlotW, scaledSlotW});
    bool          hovered = slotRect.contains(mpos);

    const bool stateChanged = mNeedsRebuild || (selected != mPrevSelected) || (hovered != mPrevHovered) ||
                              (unitMode != mPrevUnitMode) || (stock != mPrevStock) || (wallColor != mPrevWallColor);

    if (stateChanged)
    {
        mCachedBatch.clear();
        drawShapes(mCachedBatch, pos, stock, selected, wallColor, GameStateDetail::getBuildWallSize(mKind), scale, unitMode, hovered);
        mPrevSelected  = selected;
        mPrevHovered   = hovered;
        mPrevUnitMode  = unitMode;
        mPrevStock     = stock;
        mPrevWallColor = wallColor;
        mNeedsRebuild  = false;
    }

    mCachedBatch.draw(window);
    drawText(window, pos, stock, scale, hovered);
}

void GuiBalo::Slot::drawShapes(
    RenderBatch& batch,
    sf::Vector2f pos,
    int          stock,
    bool         selected,
    sf::Color    wallColor,
    sf::Vector2f wallSize,
    float        scale,
    bool         unitMode,
    bool         hovered)
{
    sf::Vector2f baseSize   = {90.f * scale, 90.f * scale};
    sf::Vector2f renderSize = baseSize;
    sf::Vector2f offset     = {0.f, 0.f};
    if (hovered)
    {
        renderSize = baseSize * 1.05f;
        offset     = (baseSize - renderSize) * 0.5f;
    }

    sf::Color fillColor;
    sf::Color outlineColor;
    float     outlineThickness = NAN;

    if (selected)
    {
        fillColor        = sf::Color(40, 50, 75, 220);
        outlineColor     = sf::Color(0, 205, 255, 230);
        outlineThickness = 3.5f * scale;
    }
    else if (hovered)
    {
        fillColor        = sf::Color(35, 40, 55, 195);
        outlineColor     = sf::Color(100, 220, 255, 180);
        outlineThickness = 2.25f * scale;
    }
    else
    {
        fillColor        = sf::Color(22, 26, 38, 170);
        outlineColor     = sf::Color(100, 115, 140, 100);
        outlineThickness = 1.5f * scale;
    }

    batch.addRectWithOutline(pos + offset, renderSize, fillColor, outlineColor, outlineThickness);

    if (selected)
    {
        sf::Color brColor = sf::Color(0, 205, 255);
        batch.addTechBrackets(pos + offset + sf::Vector2f(3.f * scale, 3.f * scale),
                              renderSize - sf::Vector2f(6.f * scale, 6.f * scale),
                              scale,
                              10.f,
                              brColor);
    }

    sf::Vector2f innerBorderSize = renderSize - sf::Vector2f(8.f * scale, 8.f * scale);
    sf::Vector2f innerBorderPos  = pos + offset + sf::Vector2f(4.f * scale, 4.f * scale);
    batch.addRectWithOutline(innerBorderPos, innerBorderSize, sf::Color::Transparent, sf::Color(255, 255, 255, 20), 1.0f * scale);

    {
        const sf::Vector2f slotCenter = pos + offset + sf::Vector2f(renderSize.x * 0.5f, renderSize.y * 0.68f);

        ViewStateDetail::Camera3D iconCam;
        iconCam.center      = slotCenter;
        iconCam.size        = renderSize;
        iconCam.cosPitch    = 0.66f;
        iconCam.sinPitch    = 0.44f;
        iconCam.heightScale = 1.15f;
        iconCam.heightPersp = 0.35f;
        iconCam.focalDist   = 420.f * scale;

        auto appendQuad3D =
            [&](sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Color cBottom, sf::Color cTop, bool cull = true)
        {
            if (cull)
            {
                const float cross = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
                if (cross >= 0.f)
                    return;
            }
            batch.triangles.append(sf::Vertex(p0, cBottom));
            batch.triangles.append(sf::Vertex(p1, cBottom));
            batch.triangles.append(sf::Vertex(p2, cTop));
            batch.triangles.append(sf::Vertex(p2, cTop));
            batch.triangles.append(sf::Vertex(p3, cTop));
            batch.triangles.append(sf::Vertex(p0, cBottom));
        };

        auto addMiniBox3D = [&](sf::Vector2f cXY, sf::Vector2f sz, float z0, float z1, sf::Color col)
        {
            const float        x0  = cXY.x - sz.x * 0.5f;
            const float        y0  = cXY.y - sz.y * 0.5f;
            const float        x1  = cXY.x + sz.x * 0.5f;
            const float        y1  = cXY.y + sz.y * 0.5f;
            const sf::Vector2f b00 = iconCam.project(x0, y0, z0);
            const sf::Vector2f b10 = iconCam.project(x1, y0, z0);
            const sf::Vector2f b11 = iconCam.project(x1, y1, z0);
            const sf::Vector2f b01 = iconCam.project(x0, y1, z0);
            const sf::Vector2f t00 = iconCam.project(x0, y0, z1);
            const sf::Vector2f t10 = iconCam.project(x1, y0, z1);
            const sf::Vector2f t11 = iconCam.project(x1, y1, z1);
            const sf::Vector2f t01 = iconCam.project(x0, y1, z1);

            const sf::Color cW = ViewStateDetail::computeLitColor(col, -1.f, 0.f, 0.f);
            const sf::Color cE = ViewStateDetail::computeLitColor(col, 1.f, 0.f, 0.f);
            const sf::Color cS = ViewStateDetail::computeLitColor(col, 0.f, 1.f, 0.f);
            const sf::Color cR = ViewStateDetail::computeLitColor(col, 0.f, 0.f, 1.f);

            appendQuad3D(b00, b01, t01, t00, ViewStateDetail::shadeColor(cW, 0.74f), cW, true);
            appendQuad3D(b11, b10, t10, t11, ViewStateDetail::shadeColor(cE, 0.74f), cE, true);
            appendQuad3D(b01, b11, t11, t01, ViewStateDetail::shadeColor(cS, 0.78f), cS, false);
            appendQuad3D(t01, t11, t10, t00, cR, ViewStateDetail::shadeColor(cR, 1.10f, 0.05f), false);
        };

        auto addMiniPrism3D =
            [&](sf::Vector2f cXY, float rBot, float rTop, float z0, float z1, int sides, sf::Color col, float rotDeg = 0.f)
        {
            const float rotRad = rotDeg * 3.14159265f / 180.f;
            const float step   = 2.f * 3.14159265f / static_cast<float>(sides);
            for (int i = 0; i < sides; ++i)
            {
                const float        a0   = rotRad + i * step;
                const float        a1   = rotRad + (i + 1) * step;
                const sf::Vector2f d0   = {std::cos(a0), std::sin(a0)};
                const sf::Vector2f d1   = {std::cos(a1), std::sin(a1)};
                const sf::Vector2f b0   = iconCam.project(cXY + d0 * rBot, z0);
                const sf::Vector2f b1   = iconCam.project(cXY + d1 * rBot, z0);
                const sf::Vector2f t0   = iconCam.project(cXY + d0 * rTop, z1);
                const sf::Vector2f t1   = iconCam.project(cXY + d1 * rTop, z1);
                const sf::Vector2f n    = (d0 + d1) * 0.5f;
                const sf::Color    fCol = ViewStateDetail::computeLitColor(col, n.x, n.y, 0.15f);
                appendQuad3D(b1, b0, t0, t1, ViewStateDetail::shadeColor(fCol, 0.72f), fCol, true);
            }
            const sf::Vector2f topC = iconCam.project(cXY, z1);
            const sf::Color    cCap = ViewStateDetail::shadeColor(col, 1.15f, 0.08f);
            const sf::Color    cEdg = ViewStateDetail::shadeColor(col, 0.96f, 0.02f);
            for (int i = 0; i < sides; ++i)
            {
                const float        a0 = rotRad + i * step;
                const float        a1 = rotRad + (i + 1) * step;
                const sf::Vector2f t0 = iconCam.project(cXY + sf::Vector2f(std::cos(a0), std::sin(a0)) * rTop, z1);
                const sf::Vector2f t1 = iconCam.project(cXY + sf::Vector2f(std::cos(a1), std::sin(a1)) * rTop, z1);
                batch.triangles.append(sf::Vertex(topC, cCap));
                batch.triangles.append(sf::Vertex(t0, cEdg));
                batch.triangles.append(sf::Vertex(t1, cEdg));
            }
        };

        auto addMiniOctahedron3D =
            [&](sf::Vector2f cXY, float r, float zCenter, float halfH, sf::Color col, float rotDeg = 25.f)
        {
            const float        rad  = rotDeg * 3.14159265f / 180.f;
            const sf::Vector2f pTop = iconCam.project(cXY, zCenter + halfH);
            const sf::Vector2f pBot = iconCam.project(cXY, zCenter - halfH);
            for (int i = 0; i < 4; ++i)
            {
                const float        a0 = rad + i * 1.5707963f;
                const float        a1 = rad + (i + 1) * 1.5707963f;
                const sf::Vector2f d0 = {std::cos(a0), std::sin(a0)};
                const sf::Vector2f d1 = {std::cos(a1), std::sin(a1)};
                const sf::Vector2f m0 = iconCam.project(cXY + d0 * r, zCenter);
                const sf::Vector2f m1 = iconCam.project(cXY + d1 * r, zCenter);
                const sf::Vector2f n  = (d0 + d1) * 0.5f;
                const sf::Color    cL = ViewStateDetail::computeLitColor(col, n.x, n.y, -0.4f);
                const sf::Color    cH = ViewStateDetail::computeLitColor(col, n.x, n.y, 0.65f);
                batch.triangles.append(sf::Vertex(pBot, ViewStateDetail::shadeColor(cL, 0.72f)));
                batch.triangles.append(sf::Vertex(m1, cL));
                batch.triangles.append(sf::Vertex(m0, cL));
                batch.triangles.append(sf::Vertex(pTop, ViewStateDetail::shadeColor(cH, 1.2f, 0.16f)));
                batch.triangles.append(sf::Vertex(m0, cH));
                batch.triangles.append(sf::Vertex(m1, cH));
            }
        };

        if (unitMode)
        {
            addMiniPrism3D(slotCenter, 22.f * scale, 20.f * scale, 0.f, 4.f * scale, 6, sf::Color(45, 65, 95), 30.f);

            if (mUnitClass == UnitClass::Tank)
            {
                addMiniBox3D(slotCenter + sf::Vector2f(-12.f * scale, 0.f),
                             {8.f * scale, 26.f * scale},
                             4.f * scale,
                             14.f * scale,
                             sf::Color(50, 56, 66));
                addMiniBox3D(slotCenter + sf::Vector2f(12.f * scale, 0.f),
                             {8.f * scale, 26.f * scale},
                             4.f * scale,
                             14.f * scale,
                             sf::Color(50, 56, 66));
                addMiniBox3D(slotCenter, {20.f * scale, 24.f * scale}, 7.f * scale, 21.f * scale, wallColor);
                addMiniPrism3D(slotCenter,
                               11.f * scale,
                               9.f * scale,
                               21.f * scale,
                               31.f * scale,
                               8,
                               ViewStateDetail::shadeColor(wallColor, 1.15f));
                addMiniBox3D(slotCenter + sf::Vector2f(0.f, 12.f * scale),
                             {5.f * scale, 18.f * scale},
                             24.f * scale,
                             29.f * scale,
                             sf::Color(190, 205, 225));
            }
            else if (mUnitClass == UnitClass::Archer || mUnitClass == UnitClass::Shooter)
            {
                addMiniBox3D(slotCenter,
                             {32.f * scale, 8.f * scale},
                             14.f * scale,
                             20.f * scale,
                             ViewStateDetail::shadeColor(wallColor, 0.88f));
                addMiniPrism3D(slotCenter, 13.f * scale, 10.f * scale, 11.f * scale, 24.f * scale, 4, wallColor, 45.f);
                addMiniOctahedron3D(slotCenter, 7.f * scale, 31.f * scale, 9.f * scale, sf::Color(225, 250, 255), 35.f);
            }
            else if (mUnitClass == UnitClass::Berserker)
            {
                addMiniBox3D(slotCenter + sf::Vector2f(-6.f * scale, 0.f),
                             {6.f * scale, 7.f * scale},
                             4.f * scale,
                             14.f * scale,
                             ViewStateDetail::shadeColor(wallColor, 0.65f));
                addMiniBox3D(slotCenter + sf::Vector2f(6.f * scale, 0.f),
                             {6.f * scale, 7.f * scale},
                             4.f * scale,
                             14.f * scale,
                             ViewStateDetail::shadeColor(wallColor, 0.65f));
                addMiniBox3D(slotCenter, {18.f * scale, 14.f * scale}, 12.f * scale, 28.f * scale, wallColor);
                addMiniBox3D(slotCenter + sf::Vector2f(-12.f * scale, 4.f * scale),
                             {4.f * scale, 16.f * scale},
                             16.f * scale,
                             24.f * scale,
                             sf::Color(255, 215, 215));
                addMiniBox3D(slotCenter + sf::Vector2f(12.f * scale, 4.f * scale),
                             {4.f * scale, 16.f * scale},
                             16.f * scale,
                             24.f * scale,
                             sf::Color(255, 215, 215));
                addMiniOctahedron3D(slotCenter, 6.5f * scale, 35.f * scale, 9.f * scale, sf::Color(255, 60, 60), 20.f);
            }
            else
            {
                addMiniBox3D(slotCenter + sf::Vector2f(-5.5f * scale, 0.f),
                             {6.f * scale, 6.f * scale},
                             4.f * scale,
                             14.f * scale,
                             ViewStateDetail::shadeColor(wallColor, 0.65f));
                addMiniBox3D(slotCenter + sf::Vector2f(5.5f * scale, 0.f),
                             {6.f * scale, 6.f * scale},
                             4.f * scale,
                             14.f * scale,
                             ViewStateDetail::shadeColor(wallColor, 0.65f));
                addMiniPrism3D(slotCenter, 12.f * scale, 10.f * scale, 12.f * scale, 29.f * scale, 8, wallColor);
                addMiniPrism3D(slotCenter,
                               8.f * scale,
                               5.5f * scale,
                               29.f * scale,
                               38.f * scale,
                               8,
                               ViewStateDetail::shadeColor(wallColor, 1.15f));
                addMiniBox3D(slotCenter + sf::Vector2f(0.f, 5.f * scale),
                             {9.f * scale, 3.f * scale},
                             31.f * scale,
                             35.f * scale,
                             sf::Color(235, 255, 255));
                addMiniBox3D(slotCenter + sf::Vector2f(-11.f * scale, 3.f * scale),
                             {4.f * scale, 11.f * scale},
                             15.f * scale,
                             28.f * scale,
                             sf::Color(255, 215, 90));
            }
        }
        else
        {
            if (mKind == WallKind::DesignatedPoint)
            {
                addMiniPrism3D(slotCenter, 16.f * scale, 14.f * scale, 0.f, 5.f * scale, 6, sf::Color(255, 215, 35));
                addMiniPrism3D(slotCenter, 11.f * scale, 10.f * scale, 7.f * scale, 10.f * scale, 8, sf::Color(255, 235, 120));
                addMiniOctahedron3D(slotCenter, 7.f * scale, 19.f * scale, 12.f * scale, sf::Color(255, 245, 150), 30.f);
            }
            else
            {
                const float        pedH    = 18.f * scale;
                const sf::Vector2f pedSize = sf::Vector2f(38.f * scale, 32.f * scale);
                const sf::Vector2f capSize = sf::Vector2f(33.f * scale, 27.f * scale);
                addMiniBox3D(slotCenter, pedSize, 0.f, pedH, wallColor);
                addMiniBox3D(slotCenter, capSize, pedH, pedH + 4.f * scale, ViewStateDetail::shadeColor(wallColor, 1.15f));
                const float topZ = pedH + 4.f * scale;

                if (mKind == WallKind::Defensive)
                {
                    addMiniPrism3D(slotCenter, 10.f * scale, 14.f * scale, topZ, topZ + 9.f * scale, 8, sf::Color(55, 48, 44));
                    addMiniOctahedron3D(slotCenter, 9.5f * scale, topZ + 17.f * scale, 12.f * scale, sf::Color(255, 80, 20), 20.f);
                    addMiniOctahedron3D(slotCenter, 5.5f * scale, topZ + 19.f * scale, 8.5f * scale, sf::Color(255, 215, 55), 65.f);
                }
                else if (mKind == WallKind::Gun)
                {
                    addMiniPrism3D(slotCenter, 12.f * scale, 10.f * scale, topZ, topZ + 11.f * scale, 8, sf::Color(130, 140, 160));
                    addMiniBox3D(slotCenter + sf::Vector2f(8.f * scale, 6.f * scale),
                                 {20.f * scale, 7.f * scale},
                                 topZ + 4.f * scale,
                                 topZ + 11.f * scale,
                                 sf::Color(90, 98, 115));
                    addMiniPrism3D(slotCenter,
                                   8.f * scale,
                                   5.5f * scale,
                                   topZ + 11.f * scale,
                                   topZ + 17.f * scale,
                                   8,
                                   sf::Color(175, 185, 205));
                }
                else if (mKind == WallKind::Electric)
                {
                    addMiniPrism3D(slotCenter, 10.f * scale, 5.5f * scale, topZ, topZ + 22.f * scale, 8, sf::Color(70, 155, 220));
                    addMiniPrism3D(slotCenter,
                                   13.f * scale,
                                   12.f * scale,
                                   topZ + 7.f * scale,
                                   topZ + 10.f * scale,
                                   6,
                                   sf::Color(170, 235, 255));
                    addMiniOctahedron3D(slotCenter, 7.5f * scale, topZ + 29.f * scale, 9.5f * scale, sf::Color(150, 245, 255), 35.f);
                }
                else if (mKind == WallKind::Fire)
                {
                    addMiniPrism3D(slotCenter, 12.f * scale, 10.f * scale, topZ, topZ + 13.f * scale, 10, sf::Color(180, 55, 30));
                    addMiniBox3D(slotCenter,
                                 {28.f * scale, 6.f * scale},
                                 topZ + 13.f * scale,
                                 topZ + 18.f * scale,
                                 sf::Color(255, 130, 50));
                    addMiniBox3D(slotCenter,
                                 {6.f * scale, 26.f * scale},
                                 topZ + 13.f * scale,
                                 topZ + 18.f * scale,
                                 sf::Color(255, 130, 50));
                    addMiniOctahedron3D(slotCenter, 5.5f * scale, topZ + 24.f * scale, 7.5f * scale, sf::Color(255, 215, 65), 15.f);
                }
                else if (mKind == WallKind::Radar)
                {
                    addMiniPrism3D(slotCenter, 7.f * scale, 4.5f * scale, topZ, topZ + 15.f * scale, 8, sf::Color(70, 145, 105));
                    addMiniPrism3D(slotCenter,
                                   5.f * scale,
                                   15.f * scale,
                                   topZ + 13.f * scale,
                                   topZ + 21.f * scale,
                                   10,
                                   sf::Color(95, 230, 150));
                    addMiniOctahedron3D(slotCenter, 5.f * scale, topZ + 26.f * scale, 7.f * scale, sf::Color(160, 255, 195), 40.f);
                }
            }
        }
    }

    stockText.setString(std::to_string(stock));
    TextUtil::setSize(stockText, 14.f * scale);
    auto textBounds = TextUtil::bounds(stockText);

    float        badgeW          = textBounds.size.x + 10.f * scale;
    float        badgeH          = 18.f * scale;
    sf::Vector2f badgePos        = pos + offset + sf::Vector2f(6.f * scale, 6.f * scale);
    sf::Color    badgeOutlineCol = stock > 0 ? sf::Color(0, 205, 120, 180) : sf::Color(140, 140, 140, 120);
    batch.addRectWithOutline(badgePos, {badgeW, badgeH}, sf::Color(12, 15, 24, 210), badgeOutlineCol, 1.f * scale);
}

void GuiBalo::Slot::drawText(sf::RenderTarget& target, sf::Vector2f pos, int stock, float scale, bool hovered)
{
    if (stock != mPrevTextStock || hovered != mPrevTextHovered)
    {
        sf::Vector2f baseSize   = {90.f * scale, 90.f * scale};
        sf::Vector2f renderSize = baseSize;
        sf::Vector2f offset     = {0.f, 0.f};
        if (hovered)
        {
            renderSize = baseSize * 1.05f;
            offset     = (baseSize - renderSize) * 0.5f;
        }

        stockText.setString(std::to_string(stock));
        TextUtil::setSize(stockText, 14.f * scale);
        auto textBounds       = stockText.getLocalBounds();
        auto textScreenBounds = TextUtil::bounds(stockText);

        float badgeW = textScreenBounds.size.x + 10.f * scale;
        float badgeH = 18.f * scale;

        stockText.setOrigin(
            {textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f});
        stockText.setPosition(pos + offset + sf::Vector2f(6.f * scale + badgeW / 2.f, 6.f * scale + badgeH / 2.f));
        stockText.setFillColor(stock > 0 ? sf::Color::White : sf::Color(150, 150, 150));
        stockText.setOutlineThickness(0.f);

        mPrevTextStock   = stock;
        mPrevTextHovered = hovered;
    }

    target.draw(stockText);
}

GuiBalo::GuiBalo(sf::Font& font)
{
    for (int i = 0; i < 6; ++i)
    {
        auto* slot       = addChild<Slot>(font);
        slot->mIndex     = i;
        slot->mKind      = static_cast<WallKind>(i);
        slot->mUnitClass = static_cast<UnitClass>(i);
    }
}

bool GuiBalo::input(const sf::Event&)
{
    return false;
}

void GuiBalo::update()
{
    PROFILE_FUNCTION();
}

void GuiBalo::render()
{
    PROFILE_FUNCTION();
    auto* client = dynamic_cast<ClientGame*>(parent);

    if (!client)
    {
        return;
    }

    auto& window = client->window();
    window.setView(client->mViewState->uiView());

    float width  = 2532.f;
    float height = 1170.f;

    float scale = 1.55f;

    mBatch.clear();

    float trayWidth  = 730.f * scale;
    float trayHeight = 110.f * scale;
    float tx         = width / 2.f - trayWidth / 2.f;
    float ty         = height - 75.f * scale - trayHeight / 2.f;

    mBatch.addRectWithOutline({tx, ty},
                              {trayWidth, trayHeight},
                              sf::Color(14, 18, 30, 200),
                              sf::Color(100, 120, 150, 75),
                              1.5f * scale);

    sf::Color trayGridCol(0, 205, 255, 12);
    for (float gx = 25.f * scale; gx < trayWidth; gx += 30.f * scale)
    {
        mBatch.addLine({tx + gx, ty}, {tx + gx, ty + trayHeight}, trayGridCol);
    }

    mBatch.addTechBrackets({tx, ty}, {trayWidth, trayHeight}, scale, 12.f, sf::Color(0, 205, 255, 180));

    mBatch.draw(window);
}
