#ifndef SCENEINSTRUCTION_HPP
#define SCENEINSTRUCTION_HPP

#include "Action.hpp"
#include "Scene.hpp"

struct SceneInstruction : Scene
{
    bool mShowVietnamese = false;

    SceneInstruction(Game* game);

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;
    void doAction(const Action& action) override;

private:
    void drawUI();
};

#endif
