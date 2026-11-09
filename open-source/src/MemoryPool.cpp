#include "MemoryPool.hpp"

#include <utility>

#include <cassert>

MemoryPool::MemoryPool() = default;

void MemoryPool::add(int entityID)
{
    if (entityID >= static_cast<int>(mGenerations.size()))
    {
        mGenerations.resize(entityID + 1, 0);
    }
    if (entityID >= static_cast<int>(mMasks.size()))
    {
        mMasks.resize(entityID + 1, Mask{});
    }
    mMasks[entityID].fill(0);
}

void MemoryPool::remove(int entityID)
{
    if (entityID < 0 || entityID >= static_cast<int>(mMasks.size()))
    {
        return;
    }

    auto mask = mMasks[entityID];

    for (int i = 0; std::cmp_less(i, mask.size()); i++)
    {
        while (mask[i])
        {
            removeComponent(i * N_BIT_SIZE_T + __builtin_ctzll(mask[i]), entityID);
            mask[i] &= mask[i] - 1;
        }
    }

    mMasks[entityID].fill(0);

    if (entityID >= 0 && entityID < static_cast<int>(mGenerations.size()))
    {
        mGenerations[entityID]++;
    }
}

namespace
{

template <std::size_t I>
void removeAt(Pool* pool, int entityID)
{
    using T = typename std::tuple_element_t<I, Pool>::Type;
    if constexpr (!std::is_empty_v<T>)
    {
        std::get<I>(*pool).remove(entityID);
    }
}

template <std::size_t... Is>
constexpr auto makeDispatchTable(std::index_sequence<Is...>)
{
    using Fn = void (*)(Pool*, int);
    return std::array<Fn, sizeof...(Is)>{&removeAt<Is>...};
}

} // namespace

static const auto kRemoveDispatch = makeDispatchTable(std::make_index_sequence<componentCount>{});

void MemoryPool::removeComponent(int idx, int entityID)
{
    kRemoveDispatch[idx](&mPool, entityID);
}

bool MemoryPool::has(int entityID) const
{
    if (entityID >= 0 && entityID < static_cast<int>(mMasks.size()))
    {
        for (size_t w : mMasks[entityID])
        {
            if (w != 0)
            {
                return true;
            }
        }
    }
    return false;
}

const Mask* MemoryPool::getMask(int entityID) const
{
    if (entityID >= 0 && entityID < static_cast<int>(mMasks.size()))
    {
        return &mMasks[entityID];
    }
    return nullptr;
}

uint32_t MemoryPool::getGeneration(int entityID) const
{
    if (entityID >= 0 && entityID < static_cast<int>(mGenerations.size()))
    {
        return mGenerations[entityID];
    }
    return 0;
}

bool MemoryPool::isValid(int entityID, uint32_t expectedGen) const
{
    return has(entityID) && getGeneration(entityID) == expectedGen;
}
