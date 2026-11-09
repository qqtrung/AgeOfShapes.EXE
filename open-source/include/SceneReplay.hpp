#ifndef SCENEREPLAY_HPP
#define SCENEREPLAY_HPP

#include "Action.hpp"
#include "ClientGame.hpp"
#include "Command.hpp"

#include <memory>

struct SceneReplay : Scene
{
    std::unique_ptr<ClientGame> client;
    bool                        mHasReplay        = false;
    uint64_t                    mReplayTotalTicks = 0;

    SceneReplay(Game* game, const std::string& path = "", int level = 1, int difficulty = 1, int numClients = 1);

    ~SceneReplay() override;
    void update() override;
    void render() override;
    void network() override;
    bool input(const sf::Event& event) override;
    void doAction(const Action& action) override;

private:
    void drawNoReplayUI();
};

#endif
