#include "Scene.hpp"

#include "Game.hpp"

Scene::Scene(Game* game)
{
    mGame = game;
}

sf::RenderWindow& Scene::window()
{
    return mGame->mWindow;
}
