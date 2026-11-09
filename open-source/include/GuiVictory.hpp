#ifndef GUIVICTORY_HPP
#define GUIVICTORY_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct GUIVictory : Space
{
    struct Button : Space
    {
        sf::Vector2f mOriginalSize;
        sf::Text     mText;

        Button(sf::Font& font);
        ~Button() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;
    };

    sf::Text mTitleText;

    GUIVictory(sf::Font& font);
    ~GUIVictory() override;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
};

#endif
