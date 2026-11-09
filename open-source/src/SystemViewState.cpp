#include "SystemViewState.hpp"

#include "ClientGame.hpp"
#include "Components.hpp"
#include "FloatFixed.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "GameStateDetail.hpp"
#include "Profiler.hpp"
#include "ViewState.hpp"

#include <SFML/Graphics/Rect.hpp>

#include <algorithm>
#include <vector>

namespace SystemViewState
{
void SystemAnimation::update()
{
    PROFILE_FUNCTION();
}

void SystemCamera::update()
{
    PROFILE_FUNCTION();
    const float dragLerp       = 0.4f;
    const float easeFactor     = 0.08f;
    const float frictionActive = 0.82f;

    sf::Vector2f playerPos;
    bool         hasPlayer  = false;
    const auto&  pool       = context->mClientGame->gameState().mAdmin.mPool;
    const int    myPlayerId = context->mClientGame->mViewState->playerEntityId();
    const auto*  playerMask = pool.getMask(myPlayerId);
    if (pool.hasComponent<CPosition>(playerMask))
    {
        const auto posComp = pool.getComponent<CPosition>(myPlayerId);
        playerPos          = sf::Vector2f(static_cast<float>(posComp->pos.x), static_cast<float>(posComp->pos.y));
        hasPlayer          = true;
    }

    if (!context->mClientGame->mViewState->input().isCameraDragging)
    {
        mIsMoving = false;
    }

    if (context->input().isCameraDragging)
    {
        sf::Vector2i pixelDelta  = context->mClientGame->mViewState->input().prevMousePixelPos -
                                   context->mClientGame->mViewState->input().mousePixelPos;
        const float  invCosPitch = 1.f / std::max(0.5f, context->camera3D().cosPitch);
        context->mClientGame->mViewState->input()
            .cameraDragDelta = sf::Vector2f{(float)pixelDelta.x * context->mClientGame->mViewState->zoomLevel() * 1.5f,
                                            (float)pixelDelta.y * context->mClientGame->mViewState->zoomLevel() * 1.5f *
                                                invCosPitch};
        context->mClientGame->mViewState->input().prevMousePixelPos = context->mClientGame->mViewState->input().mousePixelPos;
    }
    else
    {
        context->mClientGame->mViewState->input().cameraDragDelta = {0.f, 0.f};
        context->mClientGame->mViewState->input().prevMousePixelPos = context->mClientGame->mViewState->input().mousePixelPos;
    }

    if (context->input().isMinimapDragging)
    {
        mVelocity = {0.f, 0.f};
    }
    else if (hasPlayer)
    {
        if (context->input().isCameraDragging ||
            (mIsMoving && (context->input().cameraDragDelta.x != 0.f ||
                           context->mClientGame->mViewState->input().cameraDragDelta.y != 0.f)))
        {
            mVelocity.x = mVelocity.x * (1.f - dragLerp) +
                          context->mClientGame->mViewState->input().cameraDragDelta.x * dragLerp;
            mVelocity.y = mVelocity.y * (1.f - dragLerp) +
                          context->mClientGame->mViewState->input().cameraDragDelta.y * dragLerp;
        }
        else
        {
            sf::Vector2f delta = playerPos - context->mClientGame->mViewState->worldView().getCenter();
            mVelocity += delta * easeFactor;
            mVelocity *= frictionActive;
        }
    }
    else
    {
        mVelocity *= 0.88f;
        if (std::abs(mVelocity.x) < 0.05f)
            mVelocity.x = 0.f;
        if (std::abs(mVelocity.y) < 0.05f)
            mVelocity.y = 0.f;
    }

    context->mClientGame->mViewState->worldView().move(mVelocity);

    if (context->input().zoomDelta != 0.f)
    {
        context->mClientGame->mViewState->zoomLevel() += context->mClientGame->mViewState->input().zoomDelta;
        context->mClientGame->mViewState->zoomLevel() = std::clamp(context->zoomLevel(), 0.2f, 1.5f);

        auto viewSize = sf::Vector2f(context->uiView().getSize() / 1.5f);
        context->mClientGame->mViewState->worldView().setSize(viewSize * context->mClientGame->mViewState->zoomLevel());

        context->mClientGame->mViewState->input().zoomDelta = 0.f;
    }

    if (context->input().pinchZoomDelta != 0.f)
    {
        context->mClientGame->mViewState->zoomLevel() += context->mClientGame->mViewState->input().pinchZoomDelta;
        context->mClientGame->mViewState->zoomLevel() = std::clamp(context->zoomLevel(), 0.2f, 1.5f);

        auto viewSize = sf::Vector2f(context->uiView().getSize() / 1.5f);
        context->mClientGame->mViewState->worldView().setSize(viewSize * context->mClientGame->mViewState->zoomLevel());

        context->mClientGame->mViewState->input().pinchZoomDelta = 0.f;
    }

    auto  center = context->mClientGame->mViewState->worldView().getCenter();
    auto  size   = context->mClientGame->mViewState->worldView().getSize();
    float minX   = size.x / 2.f;
    float minY   = size.y / 2.f;
    float maxX   = std::max(minX, context->mapWidth() - size.x / 2.f);
    float maxY   = std::max(minY, context->mapHeight() - size.y / 2.f);

    center.x = std::clamp(center.x, minX, maxX);
    center.y = std::clamp(center.y, minY, maxY);
    context->mClientGame->mViewState->worldView().setCenter(center);
    context->mClientGame->mViewState->camera3D().setup(context->mClientGame->mViewState->worldView());
}

void SystemCamera::setTargetPosition(sf::Vector2f target)
{
    mTargetPosition = target;
    mIsMoving       = true;
    mVelocity       = {0.f, 0.f};
}

void SystemDragDrop::update()
{
    PROFILE_FUNCTION();

    if (context->input().inputMode != InputMode::DragDrop || context->mClientGame->mViewState->input().isMinimapDragging)
    {
        return;
    }

    auto& mInput = context->mClientGame->mViewState->input();

    if (mInput.leftMouseJustPressed)
    {
        const float x           = mInput.mouseWorldPos.x;
        const float y           = mInput.mouseWorldPos.y;
        const int   localPlayer = context->mClientGame->mViewState->playerEntityId();

        mInput.dragSelectedId = -1;

        int foundId = GameStateDetail::findDraggableEntityAtPosition(&context->mClientGame->gameState(), x, y, false, localPlayer);
        if (foundId == -1)
        {
            foundId = GameStateDetail::findDraggableEntityAtPosition(&context->mClientGame->gameState(), x, y + 18.f, false, localPlayer);
        }

        if (foundId != -1)
        {
            mInput.dragSelectedId = foundId;
            const auto wallRect   = context->mClientGame->gameState().mAdmin.mPool.getComponent<CRectDragDrop>(foundId);
            mInput.dragOffsetX    = x - wallRect->pos.x;
            mInput.dragOffsetY    = y - wallRect->pos.y;
        }
        else
        {
            foundId = GameStateDetail::findDraggableEntityAtPosition(&context->mClientGame->gameState(), x, y, true, localPlayer);
            if (foundId == -1)
            {
                foundId = GameStateDetail::findDraggableEntityAtPosition(&context->mClientGame->gameState(), x, y + 18.f, true, localPlayer);
            }

            if (foundId != -1)
            {
                mInput.dragSelectedId = foundId;
                const auto wallRect = context->mClientGame->gameState().mAdmin.mPool.getComponent<CRectDragDrop>(foundId);
                mInput.dragOffsetX = x - wallRect->pos.x;
                mInput.dragOffsetY = y - wallRect->pos.y;
            }
        }
    }

    if (mInput.leftMouseJustReleased)
    {
        const auto* dragMask = context->mClientGame->gameState().mAdmin.mPool.getMask(mInput.dragSelectedId);
        if (mInput.dragSelectedId != -1 &&
            context->mClientGame->gameState().mAdmin.mPool.hasComponent<CRectDragDrop>(dragMask))
        {
            const auto wallRect = context->mClientGame->gameState().mAdmin.mPool.getComponent<CRectDragDrop>(
                mInput.dragSelectedId);
            const float x       = mInput.mouseWorldPos.x;
            const float y       = mInput.mouseWorldPos.y;
            float       targetX = (x - mInput.dragOffsetX) + wallRect->size.x * 0.5f;
            float       targetY = (y - mInput.dragOffsetY) + wallRect->size.y * 0.5f;

            context->mClientGame->mViewState->send(
                Command(CommandType::MoveWall,
                        CmdMoveWall{.playerId  = context->mClientGame->mViewState->playerEntityId(),
                                    .wallId    = mInput.dragSelectedId,
                                    .worldX    = targetX,
                                    .worldY    = targetY,
                                    .releasing = true,
                                    .wallGen   = context->mClientGame->gameState().mAdmin.mPool.getGeneration(
                                        mInput.dragSelectedId)}));
        }
        mInput.dragSelectedId = -1;
    }
}

void SystemSelection::update()
{
    PROFILE_FUNCTION();

    auto& mInput = context->mClientGame->mViewState->input();

    if (mInput.requestMenuScene)
    {
        mInput.requestMenuScene = false;
        context->mClientGame->mGame->popScene();
    }

    if (context->input().inputMode != InputMode::Select)
    {
        return;
    }

    if (mInput.leftMouseJustPressed)
    {
        if (mInput.dragSelectedId != -1)
        {
            return;
        }

        mInput.isSelecting    = true;
        mInput.selectionStart = mInput.mouseWorldPos;
        mInput.selectionEnd   = mInput.mouseWorldPos;
    }

    if (mInput.isSelecting && mInput.dragSelectedId != -1)
    {
        mInput.isSelecting = false;
        return;
    }

    if (mInput.isSelecting && mInput.leftMousePressed)
    {
        mInput.selectionEnd = mInput.mouseWorldPos;
    }

    if (mInput.isSelecting && mInput.leftMouseJustReleased)
    {
        mInput.isSelecting = false;

        float rectWidth  = std::abs(mInput.selectionEnd.x - mInput.selectionStart.x);
        float rectHeight = std::abs(mInput.selectionEnd.y - mInput.selectionStart.y);
        bool  isClick    = (rectWidth < 10.f && rectHeight < 10.f);

        sf::FloatRect selectionRect;
        if (!isClick)
        {
            selectionRect.position.x = std::min(mInput.selectionStart.x, mInput.selectionEnd.x);
            selectionRect.position.y = std::min(mInput.selectionStart.y, mInput.selectionEnd.y);
            selectionRect.size.x     = rectWidth;
            selectionRect.size.y     = rectHeight;
        }

        std::vector<int> selectedUnitIds;
        selectedUnitIds.reserve(32);
        const int localTeam = context->localTeamId();
        for (int id : context->mClientGame->gameState().mAdmin.mPool.viewIndex<CUnit>())
        {
            if (GameStateDetail::getEntityTeam(context->mClientGame->gameState().mAdmin.mPool, id) != localTeam)
            {
                continue;
            }
            const auto* mask = context->mClientGame->gameState().mAdmin.mPool.getMask(id);
            if (!GameStateDetail::isAlive(&context->mClientGame->gameState(), id) ||
                !context->mClientGame->gameState().mAdmin.mPool.hasComponent<CPosition>(mask))
            {
                continue;
            }

            const auto pos          = context->mClientGame->gameState().mAdmin.mPool.getComponent<CPosition>(id);
            bool       shouldSelect = false;

            if (isClick)
            {
                float distGround = std::hypot(pos->pos.x - mInput.selectionStart.x, pos->pos.y - mInput.selectionStart.y);
                float distBody     = std::hypot(pos->pos.x - mInput.selectionStart.x,
                                                (pos->pos.y - 12.f) - mInput.selectionStart.y);
                float dist         = std::min(distGround, distBody);
                float selectRadius = 30.f;
                if (context->mClientGame->gameState().mAdmin.mPool.hasComponent<CBoundCircle>(mask))
                {
                    selectRadius = context->mClientGame->gameState().mAdmin.mPool.getComponent<CBoundCircle>(id)->radius +
                                   15.f;
                }
                if (dist <= selectRadius)
                {
                    shouldSelect = true;
                }
            }
            else
            {
                if (selectionRect.contains(pos->pos) || selectionRect.contains(sf::Vector2f(pos->pos.x, pos->pos.y - 12.f)))
                {
                    shouldSelect = true;
                }
            }

            if (shouldSelect)
            {
                selectedUnitIds.push_back(id);
            }
        }

        context->mClientGame->mViewState->send(
            Command(CommandType::SelectUnits,
                    CmdSelectUnits{.playerId = context->mClientGame->mViewState->playerEntityId(),
                                   .unitIds  = std::move(selectedUnitIds)}));
    }
}

} // namespace SystemViewState
