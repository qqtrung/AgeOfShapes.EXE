#ifndef SCENESETTING_HPP
#define SCENESETTING_HPP

#include "Action.hpp"
#include "Scene.hpp"

struct SceneSetting : Scene
{
    float mMasterVolume = 1.0f;
    float mMusicVolume  = 1.0f;
    float mSfxVolume    = 1.0f;

    SceneSetting(Game* game);

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;
    void doAction(const Action& action) override;

private:
    void drawUI();
};

#endif
