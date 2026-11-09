#ifndef FRAMEALLOCATOR_HPP
#define FRAMEALLOCATOR_HPP

#include <algorithm>
#include <new>
#include <utility>
#include <vector>

#include <cstddef>
#include <cstdint>

class FrameAllocator
{
public:
    static constexpr size_t kDefaultBlockSize = 128 * 1024;

    explicit FrameAllocator(size_t blockSize = kDefaultBlockSize);
    ~FrameAllocator();

    FrameAllocator(const FrameAllocator&)            = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;
    FrameAllocator(FrameAllocator&&)                 = delete;
    FrameAllocator& operator=(FrameAllocator&&)      = delete;

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    T* create(Args&&... args)
    {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    void reset();

    [[nodiscard]] size_t currentAllocated() const;
    [[nodiscard]] size_t blockCount() const;

private:
    size_t                  mBlockSize;
    size_t                  mCurrentBlockIndex = 0;
    size_t                  mOffset            = 0;
    std::vector<std::byte*> mBlocks;
    std::vector<size_t>     mBlockCapacities;

    void allocateNewBlock(size_t capacity);
};

#endif
