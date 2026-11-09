#include "EntityManager.hpp"

#include "ClientGame.hpp"
#include "ComponentBundle.hpp"
#include "Components.hpp"
#include "GameState.hpp"
#include "SystemGameState.hpp"

int EntityManager::getID()
{
    if (mFreeIds.empty())
        return mMaxId++;

    const int id = mFreeIds.top();
    mFreeIds.pop();
    return id;
}

void EntityManager::add(IComponentBundle* components)
{
    if (!components)
    {
        return;
    }

    const int entityID   = getID();
    components->entityID = entityID;
    commandBufferAdd.push_back(components);
}

void EntityManager::remove(int entityID)
{
    if (entityID < 0)
    {
        return;
    }

    if (entityID >= static_cast<int>(mPendingRemove.size()))
    {
        mPendingRemove.resize(entityID + 1, 0);
    }

    if (mPendingRemove[entityID])
    {
        return;
    }

    mPendingRemove[entityID] = 1;
    commandBufferRemove.push_back(entityID);
}

void EntityManager::update()
{
    if (gameState)
    {
        auto& col = gameState->collision();

        for (auto entityIdDead : commandBufferRemove)
        {
            const auto* mask = mPool.getMask(entityIdDead);
            if (mPool.hasComponent<CBoundCircle>(mask) && mPool.hasComponent<CPosition>(mask))
            {
                auto position = mPool.getComponent<CPosition>(entityIdDead);
                int  chunkX   = static_cast<int>(position->pos.x / col.spacing);
                int  chunkY   = static_cast<int>(position->pos.y / col.spacing);
                col.map.remove(chunkY, chunkX, entityIdDead);
            }
        }
    }

    for (auto& components : commandBufferAdd)
    {
        const int entityId = components->entityID;
        mPool.add(entityId);
        components->addToPool(mPool);
    }

    for (const int entityID : commandBufferRemove)
    {
        mPool.remove(entityID);
        mFreeIds.push(entityID);
        if (entityID >= 0 && entityID < static_cast<int>(mPendingRemove.size()))
        {
            mPendingRemove[entityID] = 0;
        }
    }

    if (gameState)
    {
        auto& col = gameState->collision();

        for (auto& entityIdAdd : commandBufferAdd)
        {
            int entityId = entityIdAdd->entityID;

            const auto* mask = mPool.getMask(entityId);
            if (mPool.hasComponent<CBoundCircle>(mask) && mPool.hasComponent<CPosition>(mask))
            {
                auto position = mPool.getComponent<CPosition>(entityId);
                int  chunkX   = static_cast<int>(position->pos.x / col.spacing);
                int  chunkY   = static_cast<int>(position->pos.y / col.spacing);
                col.map.insert(chunkY, chunkX, entityId);
            }
        }
    }

    for (auto* bundle : commandBufferAdd)
    {
        bundle->~IComponentBundle();
    }

    commandBufferAdd.clear();
    commandBufferRemove.clear();
    mFrameAllocator.reset();
}

int EntityManager::maxId() const
{
    return mMaxId;
}

const Stack<int>& EntityManager::freeIds() const
{
    return mFreeIds;
}
