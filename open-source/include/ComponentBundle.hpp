#ifndef COMPONENTBUNDLE_HPP
#define COMPONENTBUNDLE_HPP

#include "Debug.hpp"
#include "MemoryPool.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

struct IComponentBundle
{
    int          entityID                    = -1;
    virtual void addToPool(MemoryPool& pool) = 0;
    virtual ~IComponentBundle()              = default;
};

template <typename... Components>
struct ComponentBundle : IComponentBundle
{
    static constexpr int childCount  = (0 + ... + (IsComponentChild<Components> ? 1 : 0));
    static constexpr int parentCount = (0 + ... + (IsComponentParent<Components> ? 1 : 0));
    static_assert(childCount == 1, "Entity phai duoc tao boi dung 1 ComponentChild duy nhat!");
    static_assert((childCount + parentCount) == sizeof...(Components),
                  "Moi component trong bundle phai thuoc ComponentParent hoac ComponentChild!");

    std::tuple<Components*...> components;

    explicit ComponentBundle(Components*... comps) : components(comps...)
    {
    }

    ~ComponentBundle() override
    {
        std::apply(
            [](auto&... comp)
            {
                (
                    [&]
                    {
                        if (comp)
                        {
                            using T = std::remove_pointer_t<std::decay_t<decltype(comp)>>;
                            comp->~T();
                            comp = nullptr;
                        }
                    }(),
                    ...);
            },
            components);
    }

    void addToPool(MemoryPool& pool) override
    {
        std::apply(
            [&](auto&... comp)
            {
                (
                    [&]
                    {
                        if (comp)
                        {
                            using T = std::remove_pointer_t<std::decay_t<decltype(comp)>>;
                            pool.addComponent<T>(entityID, std::move(*comp));
                            comp->~T();
                            comp = nullptr;
                        }
                    }(),
                    ...);
            },
            components);
    }
};

#endif
