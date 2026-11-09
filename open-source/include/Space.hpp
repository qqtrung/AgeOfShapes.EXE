#ifndef Space_HPP
#define Space_HPP

#include "Action.hpp"
#include "Debug.hpp"

#include <SFML/Graphics.hpp>

#include <utility>

struct Space
{
    bool   visible = true;
    size_t depth   = 0;

    Space* next       = nullptr;
    Space* prev       = nullptr;
    Space* parent     = nullptr;
    Space* firstChild = nullptr;
    Space* lastChild  = nullptr;

    virtual ~Space();

    void appendSibling(Space* sibling);

    template <typename T, typename... Args>
    T* addChild(Args&&... args)
    {
        T* space = new T(std::forward<Args>(args)...);
        this->appendChild(space);
        return space;
    }

    template <typename T>
    [[nodiscard]] bool hasChild() const
    {
        Space* child = this->firstChild;
        while (child)
        {
            if (dynamic_cast<T*>(child))
            {
                return true;
            }
            child = child->next;
        }
        return false;
    }

    void appendChild(Space* child);
    void erase();

    void updateTraverse();

    [[nodiscard]] bool inputTraverse(const sf::Event& event);

    void renderTraverse();

    virtual void update() = 0;
    virtual void render() = 0;

    [[nodiscard]] virtual bool input(const sf::Event& event) = 0;

    virtual void doAction(const Action& action);

    void pushAction(const Action& action);

    Space* findRoot();
};

#endif
