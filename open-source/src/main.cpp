#include "Debug.hpp"
#include "ENetTransport.hpp"
#include "Game.hpp"
#include "Network.hpp"
#include "Profiler.hpp"
#include "Server.hpp"

#if defined(ANDROID) || defined(__ANDROID__)
#include "WifiPerLock.hpp"
#endif

#ifdef __WIN32
#include <windows.h>
#endif

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ctime>

enum class Mode
{
    Client,
    Server,
    DevMulti,
    Simulated
};

constexpr Mode CURRENT_MODE = Mode::Client;

constexpr uint16_t DEFAULT_PORT = 7777;
const std::string  SERVER_HOST  = "geometrylegion.ddns.net";
#define MAX_CLIENT 5

int main(int argc, char* argv[])
{
#if defined(ANDROID) || defined(__ANDROID__)
    WifiPerfLock::acquire();
#endif

#ifdef _WIN32
    wchar_t exePath[MAX_PATH];
    DWORD   length = GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    if (length > 0 && length < MAX_PATH)
    {
        std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
    }
#endif

    srand(static_cast<unsigned>(time(nullptr)));

    if constexpr (CURRENT_MODE == Mode::Server)
    {
        bool enableGui = true;
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--no-gui" || arg == "--headless" || arg == "-nogui")
            {
                enableGui = false;
            }
        }

        logout("====================================================");
        logout(" [SERVER] Starting Dedicated Server on UDP port " + std::to_string(DEFAULT_PORT) + " (IP: 9.9.9.2)...");
        logout(" [SERVER] Forwarded via " + SERVER_HOST + ":" + std::to_string(DEFAULT_PORT));
        logout(" [SERVER] FPS Limit: 256 FPS | Network Thread: Independent (256 Hz) | GUI: " +
               std::string(enableGui ? "Enabled" : "Disabled"));
        logout("====================================================");

        if (!ENetSystem::initialize())
        {
            return 1;
        }

        auto server = std::make_shared<Server>(MAX_CLIENT, enableGui);

        server->setFramerateLimit(256);

        if (!server->startNetwork(DEFAULT_PORT, 64, 256))
        {
            return 1;
        }

        while (server->isRunning())
        {
            server->run();
        }

        server->stopNetwork();
        ENetSystem::deinitialize();
        return 0;
    }

    if constexpr (CURRENT_MODE == Mode::Client)
    {
        logout("====================================================");
        logout(" [CLIENT] Starting Game Client...");
        logout(" [CLIENT] Target Server: " + SERVER_HOST + ":" + std::to_string(DEFAULT_PORT));
        logout(" [CLIENT] FPS Limit: 60 FPS | On-Demand Network (Multiplayer only)");
        logout("====================================================");

        static Network sRemoteServerAddress;
        auto           client = std::make_shared<Game>("assets/assets.txt", 0, &sRemoteServerAddress);
        client->setServerAddress(SERVER_HOST, DEFAULT_PORT);

        client->setFramerateLimit(60);

        while (client->window().isOpen())
        {
            client->run();
        }

        client->stopNetwork();
        PROFILE_END_SESSION();

#if defined(ANDROID) || defined(__ANDROID__)
        WifiPerfLock::release();
#endif
        return 0;
    }

    if constexpr (CURRENT_MODE == Mode::Simulated)
    {
        logout("Starting multiplayer game system in simulated memory route mode...");
        auto server = std::make_shared<Server>(MAX_CLIENT);
        server->setExternalTransport(true);

        std::vector<std::shared_ptr<Game>> clients;
        clients.reserve(MAX_CLIENT);
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            clients.emplace_back(std::make_shared<Game>("assets/assets.txt", i, &server->mNetwork));
            clients.back()->setExternalTransport(true);
        }

        while (true)
        {
            for (size_t i = 0; i < clients.size();)
            {
                if (!clients[i]->window().isOpen())
                {
                    clients.erase(clients.begin() + i);
                }
                else
                {
                    ++i;
                }
            }

            if (server && !server->isRunning())
            {
                server.reset();
            }

            if (clients.empty() && !server)
            {
                break;
            }

            uint64_t now = SimulationClock::nowUs();
            if (server)
            {
                NetworkRouter::route(server->mNetwork, now);
            }
            for (auto& client : clients)
            {
                NetworkRouter::route(client->mNetwork, now);
            }

            if (server)
            {
                server->run();
            }
            for (size_t i = 0; i < clients.size(); i++)
            {
                clients[i]->run();
            }
        }
        return 0;
    }

    if constexpr (CURRENT_MODE == Mode::DevMulti)
    {
        logout("Starting Dev Mode: 1 Server + " + std::to_string(MAX_CLIENT) + " Clients over ENet UDP on port " +
               std::to_string(DEFAULT_PORT) + "...");

        if (!ENetSystem::initialize())
        {
            logout("[Main] Warning: ENet system initialization failed. Running in offline/fallback mode.", true);
        }

        auto server = std::make_shared<Server>(MAX_CLIENT, true);
        if (server->mWindow)
        {
            server->mWindow->setSize(sf::Vector2u{600, 380});
            server->mWindow->setPosition(sf::Vector2i{15, 30});
        }
        if (!server->startNetwork(DEFAULT_PORT, MAX_CLIENT * 3, 256))
        {
            logout("[Main] Warning: Failed to start ENet server on port " + std::to_string(DEFAULT_PORT), true);
        }

        static Network                     sRemoteServerAddress;
        std::vector<std::shared_ptr<Game>> clients;
        clients.reserve(MAX_CLIENT);

        const std::vector<sf::Vector2i> clientPositions = {
            {635, 30},
            {15, 435},
            {635, 435},
            {1255, 30},
            {1255, 435},
        };

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            auto c = std::make_shared<Game>("assets/assets.txt", i, &sRemoteServerAddress);
            c->setServerAddress(SERVER_HOST, DEFAULT_PORT);
            c->window().setSize(sf::Vector2u{600, 380});
            if (i < static_cast<int>(clientPositions.size()))
            {
                c->window().setPosition(clientPositions[i]);
            }
            else
            {
                c->window().setPosition(sf::Vector2i{15 + ((i + 1) % 3) * 620, 30 + ((i + 1) / 3) * 405});
            }
            clients.emplace_back(std::move(c));
        }

        while (true)
        {
            for (size_t i = 0; i < clients.size();)
            {
                if (!clients[i]->window().isOpen())
                {
                    clients.erase(clients.begin() + i);
                }
                else
                {
                    ++i;
                }
            }

            if (server && !server->isRunning())
            {
                server.reset();
            }

            if (clients.empty() && !server)
            {
                break;
            }

            if (server)
            {
                server->run();
            }

            for (auto& client : clients)
            {
                client->run();
            }
        }

        ENetSystem::deinitialize();
        logout("Exiting multiplayer game system...");
        return 0;
    }

#if defined(ANDROID) || defined(__ANDROID__)
    WifiPerfLock::release();
#endif
    return 0;
}
