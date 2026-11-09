#include "SceneReplay.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "GuiBalo.hpp"
#include "GuiCombat.hpp"
#include "GuiControl.hpp"
#include "GuiMinimap.hpp"
#include "TextUtil.hpp"
#include "ViewState.hpp"

#include <cmath>

SceneReplay::SceneReplay(Game* game, const std::string& path, int level, int difficulty, int numClients) : Scene(game)
{

    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
        mHasReplay        = mGame->mLatestReplay.hasReplay;
        mReplayTotalTicks = mGame->mLatestReplay.totalTicks;
    }

    if (mHasReplay && mGame)
    {
        int rLevel   = mGame->mLatestReplay.level;
        int rDiff    = mGame->mLatestReplay.difficulty;
        int rPlayers = mGame->mLatestReplay.playerCount > 0 ? mGame->mLatestReplay.playerCount : 1;

        client = std::make_unique<
            ClientGame>(game, path, rLevel, rDiff, true, 0, rPlayers, mGame->mLatestReplay.map, mGame->mLatestReplay.gameMode);
        client->addChild<GUIMinimap>(game->mAssets.fontMap["Consolas"]);
        client->addChild<GuiBalo>(game->mAssets.fontMap["Consolas"]);
        client->addChild<GUIControl>(game->mAssets.fontMap["Consolas"]);
        client->addChild<GUICombat>(game->mAssets.fontMap["Consolas"]);

        for (const auto& cmd : mGame->mLatestReplay.commands)
        {
            client->mCommandQueue.push(cmd);
        }

        logout() << "[SceneReplay] Starting replay for latest match (" << mGame->mLatestReplay.commands.size()
                 << " command ticks, total " << mReplayTotalTicks << " ticks, Map " << rLevel << ", Diff " << rDiff
                 << ", Players " << rPlayers << ")";
    }
}

SceneReplay::~SceneReplay()
{
}

void SceneReplay::update()
{
    mCurrentFrame++;

    if (client)
    {
        if ((mReplayTotalTicks > 0 && client->gameState().mTick >= mReplayTotalTicks) || client->isGameOver())
        {
            mGame->changeScene("Menu");
            return;
        }

        client->updateTraverse();
    }
}

void SceneReplay::render()
{
    auto& font   = mGame->mAssets.fontMap.at("Consolas");
    auto& window = mGame->mWindow;

    if (!mHasReplay || !client)
    {
        window.clear(sf::Color(14, 18, 28));
        window.setView(mGame->uiView());
        drawNoReplayUI();
        return;
    }

    client->renderTraverse();

    window.setView(mGame->uiView());

    sf::RectangleShape headerBar({2532.f, 130.f});
    headerBar.setPosition({0.f, 0.f});
    headerBar.setFillColor(sf::Color(16, 22, 34, 165));
    window.draw(headerBar);

    sf::Text title = TextUtil::make(font, "REPLAY", 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({80.f, 38.f});
    window.draw(title);

    const float progress    = (mReplayTotalTicks > 0)
                                  ? std::clamp((float)client->gameState().mTick / (float)mReplayTotalTicks, 0.f, 1.f)
                                  : 0.f;
    const float playedWidth = 2532.f * progress;

    sf::RectangleShape lineBg({2532.f, 5.f});
    lineBg.setPosition({0.f, 130.f});
    lineBg.setFillColor(sf::Color(35, 45, 65, 180));
    window.draw(lineBg);

    if (playedWidth > 0.f)
    {
        sf::RectangleShape linePlayed({playedWidth, 5.f});
        linePlayed.setPosition({0.f, 130.f});
        linePlayed.setFillColor(sf::Color(0, 200, 255));
        window.draw(linePlayed);
    }

    if (playedWidth > 0.f && playedWidth < 2532.f)
    {
        float           pulse = std::sin((float)mCurrentFrame * 0.15f) * 1.5f;
        sf::CircleShape glowRing(8.f + pulse);
        glowRing.setOrigin({8.f + pulse, 8.f + pulse});
        glowRing.setPosition({playedWidth, 132.5f});
        glowRing.setFillColor(sf::Color(0, 200, 255, 75));
        window.draw(glowRing);

        sf::CircleShape playhead(5.5f);
        playhead.setOrigin({5.5f, 5.5f});
        playhead.setPosition({playedWidth, 132.5f});
        playhead.setFillColor(sf::Color::White);
        playhead.setOutlineColor(sf::Color(0, 230, 255));
        playhead.setOutlineThickness(1.5f);
        window.draw(playhead);
    }

    sf::Vector2f mpos        = window.mapPixelToCoords(sf::Mouse::getPosition(window), mGame->uiView());
    bool         backHovered = sf::FloatRect({2180.f, 25.f}, {280.f, 80.f}).contains(mpos);

    sf::RectangleShape backBtn({280.f, 80.f});
    backBtn.setPosition({2180.f, 25.f});
    backBtn.setFillColor(backHovered ? sf::Color(55, 70, 95, 210) : sf::Color(38, 48, 68, 190));
    backBtn.setOutlineColor(backHovered ? sf::Color(0, 230, 255) : sf::Color(80, 110, 150));
    backBtn.setOutlineThickness(2.f);
    window.draw(backBtn);

    sf::Text bText = TextUtil::make(font, "< BACK", 30);
    bText.setFillColor(sf::Color::White);
    auto bb = TextUtil::bounds(bText);
    bText.setPosition(
        {2180.f + (280.f - bb.size.x) * 0.5f - bb.position.x, 25.f + (80.f - bb.size.y) * 0.5f - bb.position.y});
    window.draw(bText);
}

void SceneReplay::network()
{
}

bool SceneReplay::input(const sf::Event& event)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape)
        {
            mGame->changeScene("Menu");
            return true;
        }
    }

    sf::Vector2i pixelPos;
    bool         isClick = false;
    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Left)
        {
            pixelPos = mouse->position;
            isClick  = true;
        }
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
    {
        pixelPos = touch->position;
        isClick  = true;
    }

    if (!mHasReplay || !client)
    {
        if (isClick)
        {
            sf::Vector2f mpos = mGame->mWindow.mapPixelToCoords(pixelPos, mGame->uiView());
            if (sf::FloatRect({1056.f, 650.f}, {420.f, 90.f}).contains(mpos))
            {
                mGame->changeScene("Menu");
                return true;
            }
        }
        return false;
    }

    if (isClick)
    {
        sf::Vector2f mpos = mGame->mWindow.mapPixelToCoords(pixelPos, mGame->uiView());
        if (sf::FloatRect({2180.f, 25.f}, {280.f, 80.f}).contains(mpos))
        {
            mGame->changeScene("Menu");
            return true;
        }
    }

    return true;
}

void SceneReplay::doAction(const Action& action)
{
    if (action.type == Action::Type::Back || action.type == Action::Type::ExitToMenu)
    {
        mGame->changeScene("Menu");
        return;
    }
}

void SceneReplay::drawNoReplayUI()
{
    auto& font = mGame->mAssets.fontMap.at("Consolas");

    auto& target = mGame->mWindow;

    sf::RectangleShape headerBar({2532.f, 130.f});
    headerBar.setFillColor(sf::Color(20, 26, 42));
    target.draw(headerBar);

    sf::RectangleShape lineSep({2532.f, 4.f});
    lineSep.setPosition({0.f, 130.f});
    lineSep.setFillColor(sf::Color(0, 200, 255));
    target.draw(lineSep);

    sf::Text title = TextUtil::make(font, "REPLAY", 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({80.f, 38.f});
    target.draw(title);

    sf::Text msg = TextUtil::make(font, "NO REPLAY RECORDED!\n\nPlease play a match first.", 36);
    msg.setFillColor(sf::Color(200, 215, 235));
    auto mb = TextUtil::bounds(msg);
    msg.setPosition({(2532.f - mb.size.x) / 2.f - mb.position.x, 450.f});
    target.draw(msg);

    sf::RectangleShape backBtn({420.f, 90.f});
    backBtn.setPosition({1056.f, 650.f});
    backBtn.setFillColor(sf::Color(0, 150, 190));
    backBtn.setOutlineColor(sf::Color(0, 230, 255));
    backBtn.setOutlineThickness(2.f);
    target.draw(backBtn);

    sf::Text bText = TextUtil::make(font, "< BACK", 34);
    bText.setFillColor(sf::Color::White);
    auto bb = TextUtil::bounds(bText);
    bText.setPosition(
        {1056.f + (420.f - bb.size.x) / 2.f - bb.position.x, 650.f + (90.f - bb.size.y) / 2.f - bb.position.y});
    target.draw(bText);
}
