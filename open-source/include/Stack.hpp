#ifndef STACK_HPP
#define STACK_HPP

#include <vector>

#include <cassert>

template <typename T>
struct Stack
{
    std::vector<T> st;

    [[nodiscard]]
    const T& top() const
    {
        assert(!st.empty());
        return st.back();
    }

    void push(const T& val)
    {
        st.emplace_back(val);
    }

    void pop()
    {
        assert(!st.empty());
        st.pop_back();
    }

    [[nodiscard]]
    bool empty() const
    {
        return st.empty();
    }

    [[nodiscard]]
    int size() const
    {
        return st.size();
    }

    void reset()
    {
        st.clear();
    }

    bool operator==(const Stack& other) const = default;
};

#endif
