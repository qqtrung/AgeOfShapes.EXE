#include "FrameAllocator.hpp"

FrameAllocator::FrameAllocator(size_t blockSize) : mBlockSize(blockSize)
{
    allocateNewBlock(mBlockSize);
}

FrameAllocator::~FrameAllocator()
{
    for (auto* block : mBlocks)
    {
        delete[] block;
    }
}

void* FrameAllocator::allocate(size_t size, size_t alignment)
{
    if (size == 0)
    {
        return nullptr;
    }

    uintptr_t current = reinterpret_cast<uintptr_t>(mBlocks[mCurrentBlockIndex] + mOffset);
    uintptr_t aligned = (current + alignment - 1) & ~(static_cast<uintptr_t>(alignment - 1));
    size_t    padding = aligned - current;

    if (mOffset + padding + size > mBlockCapacities[mCurrentBlockIndex])
    {
        if (mCurrentBlockIndex + 1 < mBlocks.size() && mBlockCapacities[mCurrentBlockIndex + 1] >= size + alignment)
        {
            mCurrentBlockIndex++;
        }
        else
        {
            size_t newCap = std::max(mBlockSize, size + alignment);
            allocateNewBlock(newCap);
            mCurrentBlockIndex = mBlocks.size() - 1;
        }

        mOffset = 0;
        current = reinterpret_cast<uintptr_t>(mBlocks[mCurrentBlockIndex]);
        aligned = (current + alignment - 1) & ~(static_cast<uintptr_t>(alignment - 1));
        padding = aligned - current;
    }

    mOffset += padding + size;
    return reinterpret_cast<void*>(aligned);
}

void FrameAllocator::reset()
{
    mCurrentBlockIndex = 0;
    mOffset            = 0;
}

size_t FrameAllocator::currentAllocated() const
{
    return mOffset;
}

size_t FrameAllocator::blockCount() const
{
    return mBlocks.size();
}

void FrameAllocator::allocateNewBlock(size_t capacity)
{
    auto* block = new std::byte[capacity];
    mBlocks.push_back(block);
    mBlockCapacities.push_back(capacity);
}
