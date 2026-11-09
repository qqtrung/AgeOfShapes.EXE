#include "SceneSetting.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "TextUtil.hpp"

#include <algorithm>

SceneSetting::SceneSetting(Game* game) : Scene(game)
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
    mMasterVolume = 1.0f;
    mMusicVolume  = 1.0f;
    mSfxVolume    = 1.0f;
}

bool SceneSetting::input(const sf::Event& event)
{
    sf::Vector2i pixelPos;
    bool         isClick = false;

    if (const auto* mouseButton = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseButton->button == sf::Mouse::Button::Left)
        {
            pixelPos = mouseButton->position;
            isClick  = true;
        }
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
    {
        pixelPos = touch->position;
        isClick  = true;
    }

    if (!isClick)
    {
        return false;
    }

    sf::Vector2f mpos = mGame->mWindow.mapPixelToCoords(pixelPos, mGame->uiView());

    sf::FloatRect backBtn({2180.f, 25.f}, {280.f, 80.f});
    if (backBtn.contains(mpos))
    {
        mGame->changeScene("Menu");
        return true;
    }

    if (sf::FloatRect({750.f, 280.f}, {90.f, 70.f}).contains(mpos))
    {
        mMasterVolume = std::max(0.0f, mMasterVolume - 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({1800.f, 280.f}, {90.f, 70.f}).contains(mpos))
    {
        mMasterVolume = std::min(1.0f, mMasterVolume + 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({870.f, 295.f}, {900.f, 40.f}).contains(mpos))
    {
        mMasterVolume = std::clamp((mpos.x - 870.f) / 900.f, 0.0f, 1.0f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }

    if (sf::FloatRect({750.f, 460.f}, {90.f, 70.f}).contains(mpos))
    {
        mMusicVolume = std::max(0.0f, mMusicVolume - 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({1800.f, 460.f}, {90.f, 70.f}).contains(mpos))
    {
        mMusicVolume = std::min(1.0f, mMusicVolume + 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({870.f, 475.f}, {900.f, 40.f}).contains(mpos))
    {
        mMusicVolume = std::clamp((mpos.x - 870.f) / 900.f, 0.0f, 1.0f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }

    if (sf::FloatRect({750.f, 640.f}, {90.f, 70.f}).contains(mpos))
    {
        mSfxVolume = std::max(0.0f, mSfxVolume - 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({1800.f, 640.f}, {90.f, 70.f}).contains(mpos))
    {
        mSfxVolume = std::min(1.0f, mSfxVolume + 0.1f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }
    if (sf::FloatRect({870.f, 655.f}, {900.f, 40.f}).contains(mpos))
    {
        mSfxVolume = std::clamp((mpos.x - 870.f) / 900.f, 0.0f, 1.0f);
        mGame->mAssets.setAllVolumes(mMasterVolume, mMusicVolume, mSfxVolume);
        return true;
    }

    if (sf::FloatRect({750.f, 820.f}, {320.f, 75.f}).contains(mpos))
    {
        mGame->mGraphicsLevel = 0;
        return true;
    }
    if (sf::FloatRect({1110.f, 820.f}, {320.f, 75.f}).contains(mpos))
    {
        mGame->mGraphicsLevel = 1;
        return true;
    }
    if (sf::FloatRect({1470.f, 820.f}, {320.f, 75.f}).contains(mpos))
    {
        mGame->mGraphicsLevel = 2;
        return true;
    }

    return false;
}

void SceneSetting::update()
{
}

void SceneSetting::network()
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

void SceneSetting::render()
{
    auto& window = mGame->mWindow;
    window.clear(sf::Color(14, 18, 28));
    window.setView(mGame->uiView());
    drawUI();
}

void SceneSetting::drawUI()
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

    sf::Text title = TextUtil::make(font, "SETTINGS", 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({80.f, 38.f});
    target.draw(title);

    sf::RectangleShape backBtn({280.f, 80.f});
    backBtn.setPosition({2180.f, 25.f});
    backBtn.setFillColor(sf::Color(38, 48, 68));
    backBtn.setOutlineColor(sf::Color(80, 110, 150));
    backBtn.setOutlineThickness(2.f);
    target.draw(backBtn);

    sf::Text bText = TextUtil::make(font, "< BACK", 30);
    bText.setFillColor(sf::Color::White);
    auto bb = TextUtil::bounds(bText);
    bText.setPosition({2180.f + (280.f - bb.size.x) / 2.f - bb.position.x, 25.f + (80.f - bb.size.y) / 2.f - bb.position.y});
    target.draw(bText);

    sf::RectangleShape panel({2132.f, 870.f});
    panel.setPosition({200.f, 200.f});
    panel.setFillColor(sf::Color(20, 26, 42));
    panel.setOutlineColor(sf::Color(45, 60, 85));
    panel.setOutlineThickness(2.f);
    target.draw(panel);

    auto drawSlider = [&](const std::string& labelText, float y, float val, sf::Color barCol)
    {
        sf::Text lbl = TextUtil::make(font, labelText, 32);
        lbl.setFillColor(barCol);
        lbl.setPosition({280.f, y + 15.f});
        target.draw(lbl);

        sf::RectangleShape btnMinus({90.f, 70.f});
        btnMinus.setPosition({750.f, y});
        btnMinus.setFillColor(sf::Color(32, 42, 60));
        btnMinus.setOutlineColor(sf::Color(65, 85, 115));
        btnMinus.setOutlineThickness(2.f);
        target.draw(btnMinus);

        sf::Text mText = TextUtil::make(font, "-", 40);
        mText.setFillColor(sf::Color::White);
        auto mb = TextUtil::bounds(mText);
        mText.setPosition({750.f + (90.f - mb.size.x) / 2.f - mb.position.x, y + (70.f - mb.size.y) / 2.f - mb.position.y});
        target.draw(mText);

        sf::RectangleShape track({900.f, 40.f});
        track.setPosition({870.f, y + 15.f});
        track.setFillColor(sf::Color(12, 16, 26));
        track.setOutlineColor(sf::Color(50, 65, 90));
        track.setOutlineThickness(2.f);
        target.draw(track);

        if (val > 0.001f)
        {
            sf::RectangleShape fill({900.f * val, 40.f});
            fill.setPosition({870.f, y + 15.f});
            fill.setFillColor(barCol);
            target.draw(fill);
        }

        sf::RectangleShape btnPlus({90.f, 70.f});
        btnPlus.setPosition({1800.f, y});
        btnPlus.setFillColor(sf::Color(32, 42, 60));
        btnPlus.setOutlineColor(sf::Color(65, 85, 115));
        btnPlus.setOutlineThickness(2.f);
        target.draw(btnPlus);

        sf::Text pText = TextUtil::make(font, "+", 40);
        pText.setFillColor(sf::Color::White);
        auto pb = TextUtil::bounds(pText);
        pText.setPosition({1800.f + (90.f - pb.size.x) / 2.f - pb.position.x, y + (70.f - pb.size.y) / 2.f - pb.position.y});
        target.draw(pText);

        std::string pctStr  = std::to_string(static_cast<int>(val * 100.f + 0.5f)) + "%";
        sf::Text    valText = TextUtil::make(font, pctStr, 32);
        valText.setFillColor(sf::Color(210, 225, 240));
        valText.setPosition({1920.f, y + 15.f});
        target.draw(valText);
    };

    drawSlider("MASTER VOLUME", 280.f, mMasterVolume, sf::Color(0, 220, 255));
    drawSlider("MUSIC VOLUME", 460.f, mMusicVolume, sf::Color(190, 110, 255));
    drawSlider("SFX VOLUME", 640.f, mSfxVolume, sf::Color(255, 195, 50));

    sf::Text gLbl = TextUtil::make(font, "GRAPHICS", 32);
    gLbl.setFillColor(sf::Color(50, 235, 140));
    gLbl.setPosition({280.f, 835.f});
    target.draw(gLbl);

    struct GraphBtn
    {
        std::string  name;
        sf::Vector2f pos;
        sf::Vector2f size;
        int          level;
    };

    GraphBtn gBtns[3] = {{"LOW", {750.f, 820.f}, {320.f, 75.f}, 0},
                         {"MEDIUM", {1110.f, 820.f}, {320.f, 75.f}, 1},
                         {"HIGH", {1470.f, 820.f}, {320.f, 75.f}, 2}};

    for (int i = 0; i < 3; ++i)
    {
        bool isSel = (mGame->mGraphicsLevel == gBtns[i].level);

        sf::RectangleShape b(gBtns[i].size);
        b.setPosition(gBtns[i].pos);
        b.setFillColor(isSel ? sf::Color(0, 140, 90) : sf::Color(28, 36, 52));
        b.setOutlineColor(isSel ? sf::Color(0, 255, 160) : sf::Color(55, 75, 105));
        b.setOutlineThickness(isSel ? 3.f : 2.f);
        target.draw(b);

        sf::Text t = TextUtil::make(font, gBtns[i].name, 30);
        t.setFillColor(isSel ? sf::Color::White : sf::Color(170, 185, 205));
        auto tb = TextUtil::bounds(t);
        t.setPosition({gBtns[i].pos.x + (gBtns[i].size.x - tb.size.x) / 2.f - tb.position.x,
                       gBtns[i].pos.y + (gBtns[i].size.y - tb.size.y) / 2.f - tb.position.y});
        target.draw(t);
    }
}

void SceneSetting::doAction(const Action& action)
{
    if (action.type == Action::Type::Back)
    {
        mGame->popScene();
    }
    else
    {
        Scene::doAction(action);
    }
}
