#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vector>

#include <cassert>

template <typename T>
struct Queue
{
    int frontIndex = 0;
    int backIndex  = 0;

    std::vector<T> q;

    [[nodiscard]]
    const T& front() const
    {
        return q[frontIndex];
    }

    void push(const T& val)
    {
        q.emplace_back(val);
        backIndex++;
    }

    void pop()
    {
        frontIndex++;
        assert(frontIndex <= backIndex);

        if (empty())
            reset();
    }

    [[nodiscard]]
    bool empty() const
    {
        return frontIndex == backIndex;
    }

    void reserve(size_t capacity)
    {
        q.reserve(capacity);
    }

    void reset()
    {
        frontIndex = 0;
        backIndex  = 0;
        q.clear();
    }
};

#endif
