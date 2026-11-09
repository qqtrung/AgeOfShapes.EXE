#ifndef SYSTEMMANAGER_HPP
#define SYSTEMMANAGER_HPP

#include <tuple>

template <typename... Systems>
struct SystemManager
{
    std::tuple<Systems...> mSystems;

    template <typename T>
    explicit SystemManager(T* context)
    {
        std::apply([&](auto&... system) { (..., (system.context = context)); }, mSystems);
    }

    template <typename T>
    T& get()
    {
        return std::get<T>(mSystems);
    }

    template <typename T>
    const T& get() const
    {
        return std::get<T>(mSystems);
    }

    void init()
    {
        std::apply([&](auto&... system) { (..., (system.init())); }, mSystems);
    }

    void update()
    {
        std::apply([&](auto&... system) { (..., (system.update())); }, mSystems);
    }

    void exit()
    {
        std::apply([&](auto&... system) { (..., (system.exit())); }, mSystems);
    }
};

#endif
