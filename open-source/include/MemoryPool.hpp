#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP

#include "Components.hpp"
#include "Debug.hpp"
#include "SparseSet.hpp"

#include <array>
#include <bit>
#include <tuple>
#include <type_traits>

using Pool = std::tuple<SparseSet<CPosition>,
                        SparseSet<CVelocity>,
                        SparseSet<CBoundCircle>,
                        SparseSet<CHealth>,
                        SparseSet<CTeam>,
                        SparseSet<CRectDragDrop>,
                        SparseSet<CVision>,
                        SparseSet<CObstacle>,
                        SparseSet<CMatchObjective>,
                        SparseSet<CNavBeacon>,
                        SparseSet<CNavTarget>,
                        SparseSet<CUnit>,
                        SparseSet<CPlayer>,
                        SparseSet<CWall>,
                        SparseSet<CBase>,
                        SparseSet<CProjectile>,
                        SparseSet<CPickup>,
                        SparseSet<CEnvironmentZone>,
                        SparseSet<CVisualEffect>>;

constexpr int componentCount = std::tuple_size_v<Pool>;
using Mask = std::array<size_t, componentCount / N_BIT_SIZE_T + (componentCount % N_BIT_SIZE_T ? 1 : 0)>;

template <typename T, typename Tuple>
struct ComponentIndex;

template <typename T, typename... Sets>
struct ComponentIndex<T, std::tuple<Sets...>>
{
    static constexpr int get()
    {
        int  idx      = 0;
        int  foundIdx = -1;
        auto test     = [&]<typename Set>()
        {
            if constexpr (std::is_same_v<T, typename Set::Type>)
            {
                foundIdx = idx;
            }
            ++idx;
        };
        (test.template operator()<Sets>(), ...);

        return foundIdx;
    }

    static constexpr int value = get();
    static_assert(value != -1, "Component T not found in Pool tuple!");
};

template <typename T>
inline constexpr int ComponentId = ComponentIndex<T, Pool>::value;

struct MemoryPool
{
    Pool mPool;

    std::vector<Mask> mMasks;

    std::vector<uint32_t> mGenerations;

    MemoryPool();

    [[nodiscard]]
    bool has(int entityID) const;

    [[nodiscard]]
    uint32_t getGeneration(int entityID) const;

    [[nodiscard]]
    bool isValid(int entityID, uint32_t expectedGen) const;

    void add(int entityID);
    void remove(int entityID);

    void removeComponent(int idx, int entityID);

    template <typename T>
    [[nodiscard]]
    Ptr<T> getComponent(int entityID);

    template <typename T>
    [[nodiscard]]
    Ptr<const T> getComponent(int entityID) const;

    template <typename T>
    void addComponent(int entityID, T&& component);

    [[nodiscard]]
    const Mask* getMask(int entityID) const;

    template <typename T>
    [[nodiscard]] static constexpr bool hasComponent(const Mask& mask)
    {
        constexpr int index  = ComponentId<T>;
        constexpr int chunk  = index >> (std::bit_width(N_BIT_SIZE_T) - 1);
        constexpr int offset = index & (N_BIT_SIZE_T - 1);
        return (mask[chunk] & BIT(offset)) != 0;
    }

    template <typename T>
    [[nodiscard]] static constexpr bool hasComponent(const Mask* mask)
    {
        return mask && hasComponent<T>(*mask);
    }


    template <typename T>
    [[nodiscard]]
    std::vector<T>& viewComponent();

    template <typename T>
    [[nodiscard]]
    const std::vector<T>& viewComponent() const;

    template <typename T>
    [[nodiscard]]
    std::vector<int>& viewIndex();

    template <typename T>
    [[nodiscard]]
    const std::vector<int>& viewIndex() const;

    template <typename T>
    void removeComponent(int entityID);
};

template <typename T>
inline Ptr<T> MemoryPool::getComponent(int entityID)
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).get(entityID);
}

template <typename T>
inline Ptr<const T> MemoryPool::getComponent(int entityID) const
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).get(entityID);
}


template <typename T>
inline void MemoryPool::addComponent(int entityID, T&& component)
{
    constexpr int index  = ComponentId<T>;
    constexpr int chunk  = index >> (std::bit_width(N_BIT_SIZE_T) - 1);
    constexpr int offset = index & (N_BIT_SIZE_T - 1);

    if (entityID >= static_cast<int>(mMasks.size()))
    {
        mMasks.resize(entityID + 1, Mask{});
    }
    mMasks[entityID][chunk] |= BIT(offset);

    if constexpr (!std::is_empty_v<T>)
    {
        std::get<SparseSet<T>>(mPool).add(entityID, std::forward<T>(component));
    }
}

template <typename T>
inline std::vector<T>& MemoryPool::viewComponent()
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).dense;
}

template <typename T>
inline const std::vector<T>& MemoryPool::viewComponent() const
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).dense;
}

template <typename T>
inline std::vector<int>& MemoryPool::viewIndex()
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).denseIDs;
}

template <typename T>
inline const std::vector<int>& MemoryPool::viewIndex() const
{
    static_assert(!std::is_empty_v<T>);

    return std::get<SparseSet<T>>(mPool).denseIDs;
}

template <typename T>
inline void MemoryPool::removeComponent(int entityID)
{
    constexpr int index  = ComponentId<T>;
    constexpr int chunk  = index >> (std::bit_width(N_BIT_SIZE_T) - 1);
    constexpr int offset = index & (N_BIT_SIZE_T - 1);

    if (entityID >= 0 && entityID < static_cast<int>(mMasks.size()))
    {
        mMasks[entityID][chunk] &= ~BIT(offset);
    }

    if constexpr (!std::is_empty_v<T>)
    {
        std::get<SparseSet<T>>(mPool).remove(entityID);
    }
}

#endif
