#include "GuiMinimap.hpp"

#include "Action.hpp"
#include "ClientGame.hpp"
#include "Components.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "Profiler.hpp"
#include "SystemGameState.hpp"
#include "ViewState.hpp"
#include "ViewStateDetail.hpp"

#include <algorithm>

GUIMinimap::GUIMinimap(sf::Font& font) : mFont(font)
{
}

GUIMinimap::~GUIMinimap()
{
    logout("~GUIMinimap");
}

void GUIMinimap::update()
{
    PROFILE_FUNCTION();
}

sf::FloatRect GUIMinimap::getMinimapRect(ClientGame* client) const
{
    if (!client || !client->mGame)
        return {};

    float viewH = 1170.f;
    float scale = (viewH / 768.f);

    float       margin = 14.f * scale;
    const float mapW   = client->gameState().mapWidth();
    const float mapH   = client->gameState().mapHeight();

    float minimapW = 280.f * scale;
    float minimapH = minimapW * (mapH / mapW);

    return {{margin, margin}, {minimapW, minimapH}};
}

bool GUIMinimap::input(const sf::Event& event)
{
    auto* client = dynamic_cast<ClientGame*>(parent);
    if (!client || client->isGameOver())
        return false;

    mMinimapRect = getMinimapRect(client);

    auto& window = client->window();

    if (const auto* mousePress = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePress->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f uiPos = window.mapPixelToCoords(mousePress->position, client->mViewState->uiView());
            if (mMinimapRect.contains(uiPos))
            {
                mIsDragging                                   = true;
                client->mViewState->input().isMinimapDragging = true;
                handleMoveCameraAt(client, uiPos);
                return true;
            }
        }
    }
    else if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>())
    {
        if (mIsDragging)
        {
            sf::Vector2f uiPos = window.mapPixelToCoords(mouseMove->position, client->mViewState->uiView());
            handleMoveCameraAt(client, uiPos);
            return true;
        }
    }
    else if (const auto* mouseRelease = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mIsDragging && mouseRelease->button == sf::Mouse::Button::Left)
        {
            mIsDragging                                   = false;
            client->mViewState->input().isMinimapDragging = false;
            return true;
        }
    }
    else if (const auto* touchBegan = event.getIf<sf::Event::TouchBegan>())
    {
        sf::Vector2f uiPos = window.mapPixelToCoords(touchBegan->position, client->mViewState->uiView());
        if (mMinimapRect.contains(uiPos))
        {
            mIsDragging                                   = true;
            mActiveFingerId                               = static_cast<int>(touchBegan->finger);
            client->mViewState->input().isMinimapDragging = true;
            handleMoveCameraAt(client, uiPos);
            return true;
        }
    }
    else if (const auto* touchMoved = event.getIf<sf::Event::TouchMoved>())
    {
        if (mIsDragging && static_cast<int>(touchMoved->finger) == mActiveFingerId)
        {
            sf::Vector2f uiPos = window.mapPixelToCoords(touchMoved->position, client->mViewState->uiView());
            handleMoveCameraAt(client, uiPos);
            return true;
        }
    }
    else if (const auto* touchEnded = event.getIf<sf::Event::TouchEnded>())
    {
        if (mIsDragging && static_cast<int>(touchEnded->finger) == mActiveFingerId)
        {
            mIsDragging                                   = false;
            mActiveFingerId                               = -1;
            client->mViewState->input().isMinimapDragging = false;
            return true;
        }
    }

    return false;
}

void GUIMinimap::handleMoveCameraAt(ClientGame* client, sf::Vector2f uiPos)
{
    if (mMinimapRect.size.x <= 0.f || mMinimapRect.size.y <= 0.f)
        return;

    float u = std::clamp((uiPos.x - mMinimapRect.position.x) / mMinimapRect.size.x, 0.f, 1.f);
    float v = std::clamp((uiPos.y - mMinimapRect.position.y) / mMinimapRect.size.y, 0.f, 1.f);

    const float targetWorldX = u * client->gameState().mapWidth();
    const float targetWorldY = v * client->gameState().mapHeight();

    pushAction(Action(Action::Type::MoveCamera, sf::Vector2f{targetWorldX, targetWorldY}));
}

void GUIMinimap::render()
{
    PROFILE_FUNCTION();
    auto* client = dynamic_cast<ClientGame*>(parent);
    if (!client)
        return;

    auto& window = client->window();

    sf::View savedView = window.getView();
    window.setView(client->mViewState->uiView());

    float viewH = 1170.f;
    float scale = (viewH / 768.f);

    mMinimapRect     = getMinimapRect(client);
    const float mapW = client->gameState().mapWidth();
    const float mapH = client->gameState().mapHeight();

    mBatch.clear();

    mBatch.addRect(mMinimapRect.position, mMinimapRect.size, sf::Color(82, 86, 96, 235));
    mBatch.draw(window);
    mBatch.clear();

    if (client->mViewState && client->mViewState->mFogTexture.getSize().x > 0 &&
        client->mViewState->mFogTexture.getSize().y > 0)
    {
        PROFILE_SCOPE("Minimap - Fog of War");
        sf::Sprite fogSprite(client->mViewState->mFogTexture.getTexture());
        fogSprite.setPosition(mMinimapRect.position);
        const float scaleX = mMinimapRect.size.x / static_cast<float>(client->mViewState->mFogTexture.getSize().x);
        const float scaleY = mMinimapRect.size.y / static_cast<float>(client->mViewState->mFogTexture.getSize().y);
        fogSprite.setScale({scaleX, scaleY});
        window.draw(fogSprite);
    }

    const auto& pool = client->gameState().mAdmin.mPool;

    const int       currentLevel = client->gameState().campaign().currentMap;
    const sf::Color roadColor    = (currentLevel == 1)   ? sf::Color(195, 165, 120, 160)
                                   : (currentLevel == 2) ? sf::Color(215, 180, 130, 180)
                                                         : sf::Color(65, 72, 85, 210);
    const auto&     roads        = GameConfig::getLevelRoads(currentLevel);
    for (const auto& road : roads)
    {
        float sx = mMinimapRect.position.x + (static_cast<float>(road.start.x) / mapW) * mMinimapRect.size.x;
        float sy = mMinimapRect.position.y + (static_cast<float>(road.start.y) / mapH) * mMinimapRect.size.y;
        float ex = mMinimapRect.position.x + (static_cast<float>(road.end.x) / mapW) * mMinimapRect.size.x;
        float ey = mMinimapRect.position.y + (static_cast<float>(road.end.y) / mapH) * mMinimapRect.size.y;

        mBatch.addLine({sx, sy}, {ex, ey}, roadColor);
    }

    const auto& envZones = pool.viewComponent<CEnvironmentZone>();
    for (const auto& zone : envZones)
    {
        float zx = mMinimapRect.position.x + (zone.pos.x / mapW) * mMinimapRect.size.x;
        float zy = mMinimapRect.position.y + (zone.pos.y / mapH) * mMinimapRect.size.y;
        float zw = (zone.size.x / mapW) * mMinimapRect.size.x;
        float zh = (zone.size.y / mapH) * mMinimapRect.size.y;

        sf::Color zoneColor(40, 110, 50, 180);
        switch (zone.kind)
        {
            case ZoneKind::Bush:
                zoneColor = (currentLevel == 1)   ? sf::Color(145, 160, 65, 200)
                            : (currentLevel == 2) ? sf::Color(45, 145, 55, 210)
                                                  : sf::Color(40, 165, 125, 210);
                break;
            case ZoneKind::Tree:
                zoneColor = (currentLevel == 1)   ? sf::Color(105, 145, 45, 230)
                            : (currentLevel == 2) ? sf::Color(22, 105, 35, 240)
                                                  : sf::Color(28, 160, 80, 240);
                break;
            case ZoneKind::Water:
                zoneColor = (currentLevel == 3) ? sf::Color(28, 175, 215, 190) : sf::Color(40, 120, 190, 180);
                break;
            case ZoneKind::Quicksand:
                zoneColor = sf::Color(170, 130, 80, 180);
                break;
            case ZoneKind::Lava:
                zoneColor = sf::Color(210, 60, 20, 190);
                break;
        }
        mBatch.addRect({zx, zy}, {std::max(zw, 1.5f * scale), std::max(zh, 1.5f * scale)}, zoneColor);
    }

    const auto& wallRects = pool.viewComponent<CRectDragDrop>();
    const auto& wallIds   = pool.viewIndex<CRectDragDrop>();
    for (size_t i = 0; i < wallIds.size(); ++i)
    {
        int         entityId = wallIds[i];
        const auto* mask     = pool.getMask(entityId);
        if (!pool.hasComponent<CWall>(mask))
            continue;

        const auto& wRect = wallRects[i];
        const auto  wall  = pool.getComponent<CWall>(entityId);

        float wx = mMinimapRect.position.x + (wRect.pos.x / mapW) * mMinimapRect.size.x;
        float wy = mMinimapRect.position.y + (wRect.pos.y / mapH) * mMinimapRect.size.y;
        float ww = (wRect.size.x / mapW) * mMinimapRect.size.x;
        float wh = (wRect.size.y / mapH) * mMinimapRect.size.y;

        sf::Color wallColor(95, 110, 130);
        switch (wall->kind)
        {
            case WallKind::Electric:
                wallColor = sf::Color(0, 200, 255);
                break;
            case WallKind::Fire:
                wallColor = sf::Color(255, 90, 40);
                break;
            case WallKind::Gun:
                wallColor = sf::Color(170, 180, 195);
                break;
            case WallKind::Radar:
                wallColor = sf::Color(40, 240, 120);
                break;
            case WallKind::DesignatedPoint:
                wallColor = sf::Color(255, 215, 0);
                break;
            case WallKind::Defensive:
                wallColor = (currentLevel == 1)   ? sf::Color(192, 92, 55)
                            : (currentLevel == 2) ? sf::Color(172, 68, 52)
                                                  : sf::Color(62, 118, 148);
                break;
            case WallKind::Rock:
                wallColor = (currentLevel == 1)   ? sf::Color(215, 175, 120)
                            : (currentLevel == 2) ? sf::Color(78, 115, 68)
                                                  : sf::Color(110, 140, 145);
                break;
            default:
                wallColor = sf::Color(100, 115, 135);
                break;
        }

        mBatch.addRect({wx, wy}, {std::max(ww, 1.5f * scale), std::max(wh, 1.5f * scale)}, wallColor);
    }

    for (int baseId : pool.viewIndex<CBase>())
    {
        const auto* mask = pool.getMask(baseId);
        if (client->gameState().admin().isPendingRemove(baseId))
            continue;

        sf::Vector2f baseCenter;
        if (pool.hasComponent<CPosition>(mask))
        {
            baseCenter = pool.getComponent<CPosition>(baseId)->pos;
        }
        else if (pool.hasComponent<CRectDragDrop>(mask))
        {
            const auto r = pool.getComponent<CRectDragDrop>(baseId);
            baseCenter   = r->pos + r->size * 0.5f;
        }
        else
        {
            continue;
        }

        float bx    = mMinimapRect.position.x + (baseCenter.x / mapW) * mMinimapRect.size.x;
        float by    = mMinimapRect.position.y + (baseCenter.y / mapH) * mMinimapRect.size.y;
        float bSize = 7.f * scale;

        const auto* health         = pool.hasComponent<CHealth>(mask) ? &*pool.getComponent<CHealth>(baseId) : nullptr;
        const bool  isUnderAttack  = health && (health->hp < health->maxHp);
        const bool  isFriendlyBase = client->mViewState->isFriendly(baseId);

        const sf::Color fillCol = isFriendlyBase ? (isUnderAttack ? sf::Color(255, 180, 40) : sf::Color(60, 205, 255))
                                                 : sf::Color(255, 70, 70);
        mBatch.addRectWithOutline({bx - bSize * 0.6f, by - bSize * 0.6f},
                                  {bSize * 1.2f, bSize * 1.2f},
                                  fillCol,
                                  sf::Color::White,
                                  1.2f * scale);

        mBatch.addLine({bx - bSize * 0.85f, by}, {bx + bSize * 0.85f, by}, sf::Color::White);
        mBatch.addLine({bx, by - bSize * 0.85f}, {bx, by + bSize * 0.85f}, sf::Color::White);
    }

    const auto& visionGrid = client->gameState().nav().getVisionGrid(client->mViewState->localTeamId());
    int visionSpacing      = client->gameState().nav().visionSpacing > 0 ? client->gameState().nav().visionSpacing : 16;

    for (int unitId : pool.viewIndex<CUnit>())
    {
        const auto* mask = pool.getMask(unitId);
        if (!pool.hasComponent<CPosition>(mask) || client->gameState().admin().isPendingRemove(unitId))
            continue;

        const auto pos = pool.getComponent<CPosition>(unitId)->pos;
        float      mx  = mMinimapRect.position.x + (pos.x / mapW) * mMinimapRect.size.x;
        float      my  = mMinimapRect.position.y + (pos.y / mapH) * mMinimapRect.size.y;

        const bool isFriendly = client->mViewState->isFriendly(unitId);
        if (isFriendly)
        {
            float r = 1.3f * scale;
            mBatch.addRect({mx - r, my - r}, {r * 2.f, r * 2.f}, sf::Color(50, 230, 80));
        }
        else
        {
            int cellX = static_cast<int>(pos.x) / visionSpacing;
            int cellY = static_cast<int>(pos.y) / visionSpacing;
            if (!visionGrid.isValidCell(cellX, cellY) || visionGrid[cellX][cellY] != 2)
            {
                continue;
            }

            float r = 1.3f * scale;
            mBatch.addRect({mx - r, my - r}, {r * 2.f, r * 2.f}, sf::Color(255, 55, 65));
        }
    }

    for (int playerId : pool.viewIndex<CPlayer>())
    {
        const auto* mask = pool.getMask(playerId);
        if (!pool.hasComponent<CPosition>(mask) || client->gameState().admin().isPendingRemove(playerId))
            continue;

        const auto player = pool.getComponent<CPlayer>(playerId);
        if (player->respawnTimer > 0)
        {
            continue;
        }

        const auto pos = pool.getComponent<CPosition>(playerId)->pos;
        float      mx  = mMinimapRect.position.x + (pos.x / mapW) * mMinimapRect.size.x;
        float      my  = mMinimapRect.position.y + (pos.y / mapH) * mMinimapRect.size.y;

        const bool isFriendly = client->mViewState->isFriendly(playerId);
        if (isFriendly)
        {
            float     r           = 2.0f * scale;
            int       cId         = client->mViewState->getPlayerClientId(playerId);
            sf::Color playerColor = ViewStateDetail::getPlayerPalette(cId).fill;
            mBatch.addRectWithOutline({mx - r, my - r}, {r * 2.f, r * 2.f}, playerColor, sf::Color::White, 0.8f * scale);
        }
        else
        {
            int cellX = static_cast<int>(pos.x) / visionSpacing;
            int cellY = static_cast<int>(pos.y) / visionSpacing;
            if (!visionGrid.isValidCell(cellX, cellY) || visionGrid[cellX][cellY] != 2)
            {
                continue;
            }

            float r = 2.0f * scale;
            mBatch.addRectWithOutline({mx - r, my - r}, {r * 2.f, r * 2.f}, sf::Color(255, 55, 65), sf::Color::White, 0.8f * scale);
        }
    }

    sf::View     worldView = client->mViewState->worldView();
    sf::Vector2f camCenter = worldView.getCenter();
    sf::Vector2f camHalf   = worldView.getSize() * 0.5f;
    auto&        cam3D     = client->mViewState->camera3D();
    cam3D.setup(worldView);

    auto worldToMinimapClamped = [&](sf::Vector2f wPos) -> sf::Vector2f
    {
        const float mx = mMinimapRect.position.x + (wPos.x / mapW) * mMinimapRect.size.x;
        const float my = mMinimapRect.position.y + (wPos.y / mapH) * mMinimapRect.size.y;
        return {std::clamp(mx, mMinimapRect.position.x, mMinimapRect.position.x + mMinimapRect.size.x),
                std::clamp(my, mMinimapRect.position.y, mMinimapRect.position.y + mMinimapRect.size.y)};
    };

    const sf::Vector2f tl = worldToMinimapClamped(
        cam3D.unprojectToGround({camCenter.x - camHalf.x, camCenter.y - camHalf.y}));
    const sf::Vector2f tr = worldToMinimapClamped(
        cam3D.unprojectToGround({camCenter.x + camHalf.x, camCenter.y - camHalf.y}));
    const sf::Vector2f br = worldToMinimapClamped(
        cam3D.unprojectToGround({camCenter.x + camHalf.x, camCenter.y + camHalf.y}));
    const sf::Vector2f bl = worldToMinimapClamped(
        cam3D.unprojectToGround({camCenter.x - camHalf.x, camCenter.y + camHalf.y}));

    const sf::Color frustumCol(255, 255, 255, 225);
    mBatch.addLine(tl, tr, frustumCol);
    mBatch.addLine(tr, br, frustumCol);
    mBatch.addLine(br, bl, frustumCol);
    mBatch.addLine(bl, tl, frustumCol);

    mBatch.addRectWithOutline(mMinimapRect.position,
                              mMinimapRect.size,
                              sf::Color::Transparent,
                              sf::Color(65, 110, 160, 200),
                              1.5f * scale);

    mBatch.draw(window);

    window.setView(savedView);
}
