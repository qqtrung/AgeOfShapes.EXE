#ifndef GUIMENU_HPP
#define GUIMENU_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct GUIMenu : Space
{
    GUIMenu(sf::Font& font);
    ~GUIMenu() override = default;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
};

#endif
