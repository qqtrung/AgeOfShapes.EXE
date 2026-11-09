#ifndef ISYSTEM_HPP
#define ISYSTEM_HPP

template <typename T>
struct ISystem
{
    virtual ~ISystem() = default;

    T* context = nullptr;

    virtual void init()
    {
    }
    virtual void update()
    {
    }
    virtual void exit()
    {
    }
};

#endif
