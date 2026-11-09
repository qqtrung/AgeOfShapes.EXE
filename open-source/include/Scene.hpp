#ifndef SCENE_HPP
#define SCENE_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct Game;

struct Scene : Space
{
    Game* mGame = nullptr;

    size_t mCurrentFrame = 0;

    Scene(Game* game);

    void update() override = 0;
    void render() override = 0;

    [[nodiscard]]
    bool input(const sf::Event& event) override = 0;

    virtual void network()
    {
    }

    virtual sf::RenderWindow& window();

    ~Scene() override = default;
};

#endif
