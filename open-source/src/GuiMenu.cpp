#include "GuiMenu.hpp"

#include "Profiler.hpp"

GUIMenu::GUIMenu(sf::Font&)
{
}

bool GUIMenu::input(const sf::Event&)
{
    return false;
}

void GUIMenu::update()
{
    PROFILE_FUNCTION();
}

void GUIMenu::render()
{
    PROFILE_FUNCTION();
}
