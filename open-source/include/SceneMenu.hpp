#ifndef SCENEMENU_HPP
#define SCENEMENU_HPP

#include "Action.hpp"
#include "Scene.hpp"

struct SceneMenu : Scene
{

    SceneMenu(Game* game);
    ~SceneMenu() override = default;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
    void network() override;
    void doAction(const Action& action) override;

private:
    void drawUI(int hoverIndex);
};

#endif
