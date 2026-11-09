#include "Space.hpp"

Space::~Space()
{
    while (firstChild)
    {
        firstChild->erase();
    }
}

void Space::appendSibling(Space* sibling)
{
    sibling->prev   = this;
    sibling->next   = this->next;
    sibling->parent = this->parent;
    sibling->depth  = this->depth;

    if (this->next)
    {
        this->next->prev = sibling;
    }

    this->next = sibling;
}

void Space::appendChild(Space* child)
{
    if (!this->firstChild)
    {
        child->parent    = this;
        child->depth     = this->depth + 1;
        this->firstChild = child;
        this->lastChild  = child;
    }
    else
    {
        this->lastChild->appendSibling(child);
        this->lastChild = child;
    }
}

void Space::erase()
{
    if (this->parent)
    {
        if (this->parent->firstChild == this)
        {
            this->parent->firstChild = this->next;
        }

        if (this->parent->lastChild == this)
        {
            this->parent->lastChild = this->prev;
        }
    }

    if (prev)
    {
        prev->next = this->next;
    }

    if (next)
    {
        next->prev = this->prev;
    }

    while (this->firstChild)
    {
        this->firstChild->erase();
    }

    delete this;
}

void Space::updateTraverse()
{
    if (this->visible)
    {
        this->update();

        if (this->firstChild)
        {
            this->firstChild->updateTraverse();
        }
    }

    if (this->next)
    {
        this->next->updateTraverse();
    }
}

bool Space::inputTraverse(const sf::Event& event)
{
    if (this->visible)
    {
        if (this->lastChild)
        {
            if (this->lastChild->inputTraverse(event))
            {
                return true;
            }
        }

        if (this->input(event))
        {
            return true;
        }
    }

    if (this->prev)
    {
        if (this->prev->inputTraverse(event))
        {
            return true;
        }
    }

    return false;
}

void Space::renderTraverse()
{
    if (this->visible)
    {
        this->render();

        if (this->firstChild)
        {
            this->firstChild->renderTraverse();
        }
    }

    if (this->next)
    {
        this->next->renderTraverse();
    }
}

void Space::doAction(const Action& action)
{
    std::string actionStr = action.toString();
    logout("Received action: " + actionStr);
}

void Space::pushAction(const Action& action)
{
    Space* root = findRoot();
    if (root)
    {
        root->doAction(action);
    }
}

Space* Space::findRoot()
{
    Space* current = this;
    while (current->parent)
    {
        current = current->parent;
    }
    return current;
}
