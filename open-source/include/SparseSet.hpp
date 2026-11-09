#ifndef SPARSESET_HPP
#define SPARSESET_HPP

#include <bit>
#include <utility>
#include <vector>

#include <cassert>
#include <cstddef>

template <typename T>
class Ptr
{
    T* m_ptr;

    const uint64_t* m_generations;
    const uint64_t* m_storageGeneration;
    uint64_t        m_expectedGeneration;
    uint64_t        m_expectedStorageGeneration;

public:
    Ptr(T* ptr, const uint64_t* generations, const uint64_t* storageGeneration) :
        m_ptr(ptr),
        m_generations(generations),
        m_storageGeneration(storageGeneration),
        m_expectedGeneration(*generations),
        m_expectedStorageGeneration(*storageGeneration)
    {
    }

    [[nodiscard]]
    T* get() const
    {
        return m_ptr;
    }

    T* operator->() const
    {
        return get();
    }

    T& operator*() const
    {
        return *get();
    }
};

template <typename T, int PageSize = 2048>
struct SparseSet
{
    static_assert(PageSize > 0, "PageSize must be greater than 0");
    static_assert((PageSize & (PageSize - 1)) == 0, "PageSize must be a power of two");

    static constexpr int invalid = -1;

    static constexpr int pageBits = std::countr_zero(static_cast<unsigned>(PageSize));
    static constexpr int pageMask = PageSize - 1;

    using Type = T;

    std::vector<std::vector<int>> sparse;
    std::vector<T>                dense;
    std::vector<int>              denseIDs;


    std::vector<uint64_t> generations;
    uint64_t              storageGeneration = 0;


    SparseSet()                            = default;
    SparseSet(const SparseSet&)            = delete;
    SparseSet& operator=(const SparseSet&) = delete;
    SparseSet(SparseSet&&)                 = delete;
    SparseSet& operator=(SparseSet&&)      = delete;

    ~SparseSet() = default;

    [[nodiscard]]
    static constexpr int pageOf(int id) noexcept
    {
        return id >> pageBits;
    }

    [[nodiscard]]
    static constexpr int offsetOf(int id) noexcept
    {
        return id & pageMask;
    }

    void ensurePage(int page)
    {
        if ((int)sparse.size() <= page)
            sparse.resize(page + 1);

        if (sparse[page].empty())
            sparse[page].resize(PageSize, invalid);
    }

    [[nodiscard]]
    bool validID(int id) const noexcept
    {
        return id >= 0;
    }

    [[nodiscard]]
    bool has(int id) const noexcept
    {
        assert(validID(id) && "SparseSet::has -> invalid ID");

        const int page = pageOf(id);

        if (page >= (int)sparse.size())
            return false;

        if (sparse[page].empty())
            return false;

        return sparse[page][offsetOf(id)] != invalid;
    }

    void add(int id, T&& component)
    {
        assert(validID(id) && "SparseSet::add -> invalid ID");
        assert(!has(id) && "SparseSet::add -> ID already exists");

        const int page   = pageOf(id);
        const int offset = offsetOf(id);

        ensurePage(page);

        sparse[page][offset] = (int)dense.size();


        if (dense.size() == dense.capacity() || generations.size() == generations.capacity())
            storageGeneration++;


        dense.emplace_back(std::forward<T>(component));
        denseIDs.push_back(id);


        generations.push_back(0);
    }

    void remove(int id)
    {
        assert(has(id) && "SparseSet::remove -> ID does not exist");

        const int page   = pageOf(id);
        const int offset = offsetOf(id);

        const int removeIndex = sparse[page][offset];
        const int lastIndex   = (int)dense.size() - 1;
        const int lastID      = denseIDs[lastIndex];

        if (removeIndex != lastIndex)
        {
            dense[removeIndex]    = std::move(dense[lastIndex]);
            denseIDs[removeIndex] = lastID;

            sparse[pageOf(lastID)][offsetOf(lastID)] = removeIndex;
        }

        dense.pop_back();
        denseIDs.pop_back();
        sparse[page][offset] = invalid;


        generations[removeIndex] += 1;
        generations[lastIndex] = -1;
        generations.pop_back();
    }

    [[nodiscard]]
    Ptr<T> get(int id)
    {
        assert(has(id) && "SparseSet::get -> ID does not exist");

        auto  index = sparse[pageOf(id)][offsetOf(id)];
        auto& comp  = dense[index];

        return Ptr<T>(&comp, &generations[index], &storageGeneration);
    }

    [[nodiscard]]
    Ptr<const T> get(int id) const
    {
        assert(has(id) && "SparseSet::get -> ID does not exist");

        auto  index = sparse[pageOf(id)][offsetOf(id)];
        auto& comp  = dense[index];

        return Ptr<const T>(&comp, &generations[index], &storageGeneration);
    }

    [[nodiscard]]
    std::vector<T>& view() noexcept
    {
        return dense;
    }

    [[nodiscard]]
    const std::vector<T>& view() const noexcept
    {
        return dense;
    }

    [[nodiscard]]
    std::vector<int>& viewIndex() noexcept
    {
        return denseIDs;
    }

    [[nodiscard]]
    const std::vector<int>& viewIndex() const noexcept
    {
        return denseIDs;
    }
};

#endif
