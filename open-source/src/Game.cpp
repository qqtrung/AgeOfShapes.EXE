#include "Game.hpp"

#include "Assets.hpp"
#include "FPSMeter.hpp"
#include "GameConfig.hpp"
#include "GuiInstruction.hpp"
#include "GuiMenu.hpp"
#include "GuiSetting.hpp"
#include "Profiler.hpp"
#include "SceneCombat.hpp"
#include "SceneInstruction.hpp"
#include "SceneMenu.hpp"
#include "SceneMultiplayer.hpp"
#include "SceneReplay.hpp"
#include "SceneSetting.hpp"
#include "SceneSinglePlayer.hpp"
#include "ViewState.hpp"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <filesystem>
#include <fstream>

#include <cassert>

Game::Game(const std::string& path, int id, Network* serverNetwork) : mId(id), mNetwork(id, serverNetwork)
{
    init(path);
}

void Game::init(const std::string& path)
{
    std::string  title = "Age of Shapes";
    sf::Vector2i pos{635, 30};

    if (mId == 0)
    {
        pos = {635, 30};
    }
    else if (mId == 1)
    {
        pos = {15, 435};
    }
    else if (mId == 2)
    {
        pos = {635, 435};
    }
    else if (mId == 3)
    {
        pos = {1255, 30};
    }
    else if (mId == 4)
    {
        pos = {1255, 435};
    }
    else
    {
        pos = {15 + (mId % 3) * 620, 30 + (mId / 3) * 405};
    }

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 4;

#ifdef __ANDROID__
    mWindow.create(sf::VideoMode::getDesktopMode(), title, sf::Style::None, sf::State::Fullscreen, settings);
#else
    mWindow.create(sf::VideoMode{sf::Vector2u{1280, 768}},
                   title,
                   sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close,
                   sf::State::Windowed,
                   settings);
#endif

    mUIView = sf::View(sf::FloatRect({0.f, 0.f}, {2532.f, 1170.f}));

    mAssets.loadFromFile(path);

    ViewState::initFogShader();

    mFpsMeter = std::make_unique<FPSMeter>(mAssets.fontMap["Consolas"]);

    mClockTick.restart();
    m_prev = mClockTick.getElapsedTime().asMicroseconds();

    pushScene("Menu");
}

void Game::sendPacket(Packet pkt)
{
    mNetwork.sendPacket(std::move(pkt));
}

void Game::update()
{
    PROFILE_FUNCTION();
    while (m_countPopScenes && !m_scenes.empty())
    {
        m_scenes.pop();
        m_countPopScenes--;
    }

    if (m_pendingScene)
    {
        m_scenes.push(std::move(m_pendingScene));
    }

    if (m_scenes.empty())
        return;

    m_scenes.top()->updateTraverse();

    if (mFpsMeter)
    {
        mFpsMeter->update();
    }
}

void Game::render()
{
    PROFILE_FUNCTION();
    if (m_scenes.empty())
        return;

    m_scenes.top()->renderTraverse();

    if (mNightLightMode > 0)
    {
        const sf::View oldView = mWindow.getView();
        mWindow.setView(sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(mWindow.getSize()))));

        static sf::RectangleShape nightLightOverlay;
        nightLightOverlay.setSize(sf::Vector2f(mWindow.getSize()));
        nightLightOverlay.setFillColor(mNightLightColor);
        mWindow.draw(nightLightOverlay, sf::BlendMultiply);

        mWindow.setView(oldView);
    }

    if (mFpsMeter)
    {
        const bool netActive = mNetwork.isRunning() || mNetwork.isExternalTransport();
        mFpsMeter->setRtt(mNetwork.getRtt(), mNetwork.isConnected(), netActive);
        mFpsMeter->setBufferSize(mLockstepBufferSize);
        mFpsMeter->draw(mWindow, mUIView);
    }
}

void Game::input()
{
    while (const std::optional event = mWindow.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            quit();
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::F9)
            {
                mNightLightMode = (mNightLightMode + 1) % 4;
                if (mNightLightMode == 1)
                {
                    mNightLightColor = sf::Color(255, 226, 212, 255);
                    logout("[Eye Comfort Shield] Mode 1: Windows 10/11 Night Light (Soft Warm Peach)");
                }
                else if (mNightLightMode == 2)
                {
                    mNightLightColor = sf::Color(255, 180, 107, 255);
                    logout("[Eye Comfort Shield] Mode 2: Android AOSP Night Light (2850K Deep Amber)");
                }
                else if (mNightLightMode == 3)
                {
                    mNightLightColor = sf::Color(255, 218, 170, 255);
                    logout("[Eye Comfort Shield] Mode 3: Samsung Eye Comfort Shield (4000K Balanced)");
                }
                else
                {
                    logout("[Eye Comfort Shield] Mode 0: Disabled (Standard 6500K)");
                }
            }
        }

        if (!m_scenes.empty())
        {
            (void)m_scenes.top()->inputTraverse(*event);
        }
    }
}

void Game::pushScene(std::unique_ptr<Scene> scene)
{
    assert(scene);
    if (!m_pendingScene)
    {
        m_pendingScene = std::move(scene);
    }
}

void Game::changeScene(std::unique_ptr<Scene> scene)
{
    assert(scene);
    if (!m_pendingScene)
    {
        m_pendingScene   = std::move(scene);
        m_countPopScenes = m_scenes.size();
    }
}

void Game::popScene()
{
    if (!m_pendingScene)
    {
        if (m_scenes.size() > 1)
        {
            m_countPopScenes = 1;
        }
        else
        {
            changeScene("Menu");
        }
    }
}

void Game::pushScene(const std::string& name, int level, int difficulty, int numClients, int slotIndex, int map, int gameMode)
{
    if (auto scene = createScene(name, level, difficulty, numClients, slotIndex, map, gameMode))
    {
        pushScene(std::move(scene));
    }
}

void Game::changeScene(const std::string& name, int level, int difficulty, int numClients, int slotIndex, int map, int gameMode)
{
    if (auto scene = createScene(name, level, difficulty, numClients, slotIndex, map, gameMode))
    {
        changeScene(std::move(scene));
    }
}

std::unique_ptr<Scene> Game::createScene(
    const std::string& name,
    int                level,
    int                difficulty,
    int                numClients,
    int                slotIndex,
    int                map,
    int                gameMode)
{
    if (name == "Combat AI" || name == "Combat")
    {
        return std::make_unique<SceneCombat>(this, "", level, difficulty, numClients, slotIndex, map, gameMode);
    }
    if (name == "SinglePlayer" || name == "Single Player")
    {
        return std::make_unique<SceneSinglePlayer>(this);
    }
    if (name == "Multiplayer" || name == "Lobby")
    {
        return std::make_unique<SceneMultiplayer>(this);
    }
    if (name == "Replay")
    {
        return std::make_unique<SceneReplay>(this, "", level, difficulty, numClients);
    }
    if (name == "Menu")
    {
        return std::make_unique<SceneMenu>(this);
    }
    if (name == "Setting" || name == "Settings")
    {
        return std::make_unique<SceneSetting>(this);
    }
    if (name == "Instruction" || name == "Help")
    {
        return std::make_unique<SceneInstruction>(this);
    }

    return nullptr;
}

void Game::quit()
{
    PROFILE_END_SESSION();
    mNetwork.sendDisconnect("Client window closed");

    if (mWindow.isOpen())
    {
        mWindow.close();
    }
}

void Game::run()
{
    if (!mWindow.isOpen())
    {
        mNetwork.sendDisconnect("Client window closed");
        return;
    }

    uint64_t cur = mClockTick.getElapsedTime().asMicroseconds();
    m_lag += cur - m_prev;
    m_prev = cur;

    m_lag = std::min(m_lag, GameConfig::Game::kTickUs * 10);

    input();
    network();

    if (m_lag >= GameConfig::Game::kTickUs)
    {
        while (m_lag >= GameConfig::Game::kTickUs)
        {
            m_lag -= GameConfig::Game::kTickUs;
            update();
            network();
        }
    }

    render();
    {
        PROFILE_SCOPE("Window Display (GPU Wait & VSync)");
        mWindow.display();
    }
}

void Game::startNetwork(const std::string& host, uint16_t port)
{
    const std::string& targetHost = host.empty() ? mServerHost : host;
    const uint16_t     targetPort = (port == 0) ? mServerPort : port;
    mNetwork.start(targetHost, targetPort, 256);
}

void Game::stopNetwork()
{
    mLockstepBufferSize = -1;
    mNetwork.stop();
}

void Game::network()
{
    PROFILE_FUNCTION();
    mNetwork.sync();

    while (m_countPopScenes && !m_scenes.empty())
    {
        m_scenes.pop();
        m_countPopScenes--;
    }
    if (m_pendingScene)
    {
        m_scenes.push(std::move(m_pendingScene));
    }

    if (!m_scenes.empty())
    {
        m_scenes.top()->network();
    }

    mNetwork.sync();
}

Game::~Game()
{
    mNetwork.sendDisconnect("Client window closed");
    mNetwork.stop();
}
