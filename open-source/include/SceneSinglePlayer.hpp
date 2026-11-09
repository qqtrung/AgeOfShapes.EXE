#ifndef SCENESINGLEPLAYER_HPP
#define SCENESINGLEPLAYER_HPP

#include "Scene.hpp"

struct SceneSinglePlayer : Scene
{
    int mSelectedLevel      = 1;
    int mSelectedDifficulty = 1;

    SceneSinglePlayer(Game* game);
    ~SceneSinglePlayer() override = default;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;
    void doAction(const Action& action) override;

private:
    void drawUI(bool backHovered, bool startHovered);
};

#endif
