#ifndef ENTITYMANAGER_HPP
#define ENTITYMANAGER_HPP

#include "ComponentBundle.hpp"
#include "FrameAllocator.hpp"
#include "MemoryPool.hpp"
#include "Stack.hpp"

#include <memory>
#include <utility>
#include <vector>

class ClientGame;
class GameState;

struct EntityManager
{
    GameState* gameState = nullptr;

    MemoryPool mPool;

    std::vector<int>               commandBufferRemove;
    std::vector<IComponentBundle*> commandBufferAdd;
    std::vector<uint8_t>           mPendingRemove;
    FrameAllocator                 mFrameAllocator;

    void add(IComponentBundle* components);
    void remove(int entityID);
    void update();

    [[nodiscard]] bool isPendingRemove(int entityID) const
    {
        if (entityID >= 0 && entityID < static_cast<int>(mPendingRemove.size()))
        {
            return mPendingRemove[entityID] != 0;
        }
        return false;
    }

    template <typename T, typename... Args>
    T* alloc(Args&&... args)
    {
        return mFrameAllocator.create<T>(std::forward<Args>(args)...);
    }

    template <typename... Comps>
    ComponentBundle<Comps...>* createBundle(Comps*... comps)
    {
        return mFrameAllocator.create<ComponentBundle<Comps...>>(comps...);
    }

    [[nodiscard]] int               maxId() const;
    [[nodiscard]] const Stack<int>& freeIds() const;

private:
    int        mMaxId = 1;
    Stack<int> mFreeIds;

    int getID();
};

#endif
