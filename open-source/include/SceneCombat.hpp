#ifndef SCENECOMBAT_HPP
#define SCENECOMBAT_HPP

#include "Action.hpp"
#include "ClientGame.hpp"
#include "Debug.hpp"
#include "Scene.hpp"

#include <memory>
#include <string>

struct SceneCombat : Scene
{
    std::unique_ptr<ClientGame> client;
    bool                        mReturningToMultiplayer = false;

    SceneCombat(Game*              game,
                const std::string& path,
                int                level      = 1,
                int                difficulty = 1,
                int                numClients = 1,
                int                slotIndex  = 0,
                int                map        = 1,
                int                gameMode   = 0);
    ~SceneCombat() override;

    void update() override;
    void render() override;
    void network() override;
    bool input(const sf::Event& event) override;
    void doAction(const Action& action) override;

    static void      logout(const std::string& message, bool isError = false);
    static LogStream logout(bool isError = false);
    static void      logOut(const std::string& message, bool isError = false);
    static LogStream logOut(bool isError = false);
};

#endif
