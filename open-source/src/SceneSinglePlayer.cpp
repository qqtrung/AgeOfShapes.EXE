#include "SceneSinglePlayer.hpp"

#include "Game.hpp"
#include "TextUtil.hpp"

SceneSinglePlayer::SceneSinglePlayer(Game* game) : Scene(game)
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

void SceneSinglePlayer::network()
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

void SceneSinglePlayer::doAction(const Action& action)
{
    if (action.type == Action::Type::Back)
    {
        mGame->changeScene("Menu");
        return;
    }
    Scene::doAction(action);
}

bool SceneSinglePlayer::input(const sf::Event& event)
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

    if (sf::FloatRect({120.f, 250.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedLevel != 1)
        {
            mSelectedLevel = 1;
        }
        return true;
    }
    if (sf::FloatRect({906.f, 250.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedLevel != 2)
        {
            mSelectedLevel = 2;
        }
        return true;
    }
    if (sf::FloatRect({1692.f, 250.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedLevel != 3)
        {
            mSelectedLevel = 3;
        }
        return true;
    }

    if (sf::FloatRect({120.f, 590.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedDifficulty != 0)
        {
            mSelectedDifficulty = 0;
        }
        return true;
    }
    if (sf::FloatRect({906.f, 590.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedDifficulty != 1)
        {
            mSelectedDifficulty = 1;
        }
        return true;
    }
    if (sf::FloatRect({1692.f, 590.f}, {720.f, 220.f}).contains(mpos))
    {
        if (mSelectedDifficulty != 2)
        {
            mSelectedDifficulty = 2;
        }
        return true;
    }

    sf::FloatRect startBtn({866.f, 950.f}, {800.f, 120.f});
    if (startBtn.contains(mpos))
    {
        if (mGame)
        {
            mGame->mNetwork.ingoingPacket.clear();
        }
        mGame->changeScene("Combat AI", mSelectedLevel, mSelectedDifficulty, 1, 0, mSelectedLevel, 0);
        return true;
    }

    return false;
}

void SceneSinglePlayer::update()
{
}

void SceneSinglePlayer::render()
{
    auto&        window = mGame->mWindow;
    sf::Vector2f mpos   = window.mapPixelToCoords(sf::Mouse::getPosition(window), mGame->uiView());

    bool backHovered  = sf::FloatRect({2180.f, 25.f}, {280.f, 80.f}).contains(mpos);
    bool startHovered = sf::FloatRect({866.f, 950.f}, {800.f, 120.f}).contains(mpos);

    window.clear(sf::Color(14, 18, 28));
    window.setView(mGame->uiView());
    drawUI(backHovered, startHovered);
}

void SceneSinglePlayer::drawUI(bool backHovered, bool startHovered)
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

    sf::Text title = TextUtil::make(font, "SINGLE PLAYER", 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({80.f, 38.f});
    target.draw(title);

    sf::RectangleShape backBtn({280.f, 80.f});
    backBtn.setPosition({2180.f, 25.f});
    backBtn.setFillColor(backHovered ? sf::Color(55, 70, 95) : sf::Color(38, 48, 68));
    backBtn.setOutlineColor(backHovered ? sf::Color(0, 220, 255) : sf::Color(80, 110, 150));
    backBtn.setOutlineThickness(2.f);
    target.draw(backBtn);

    sf::Text bText = TextUtil::make(font, "< BACK", 30);
    bText.setFillColor(backHovered ? sf::Color(0, 230, 255) : sf::Color::White);
    auto bb = TextUtil::bounds(bText);
    bText.setPosition({2180.f + (280.f - bb.size.x) / 2.f - bb.position.x, 25.f + (80.f - bb.size.y) / 2.f - bb.position.y});
    target.draw(bText);

    const char* levelNames[3] = {"MAP 1", "MAP 2", "MAP 3"};
    float       lx            = 120.f;
    for (int i = 0; i < 3; ++i)
    {
        bool isSel = (mSelectedLevel == i + 1);

        sf::RectangleShape card({720.f, 220.f});
        card.setPosition({lx, 250.f});
        card.setFillColor(isSel ? sf::Color(15, 75, 65) : sf::Color(20, 26, 42));
        card.setOutlineColor(isSel ? sf::Color(0, 255, 180) : sf::Color(45, 60, 85));
        card.setOutlineThickness(isSel ? 3.f : 2.f);
        target.draw(card);

        sf::Text tName = TextUtil::make(font, levelNames[i], 42);
        tName.setFillColor(isSel ? sf::Color(0, 255, 180) : sf::Color(210, 225, 240));
        auto b = TextUtil::bounds(tName);
        tName.setPosition({lx + (720.f - b.size.x) / 2.f - b.position.x, 250.f + (220.f - b.size.y) / 2.f - b.position.y});
        target.draw(tName);

        lx += 786.f;
    }

    struct DiffInfo
    {
        std::string name;
        sf::Color   color;
    };
    DiffInfo diffs[3] = {{"EASY", sf::Color(50, 235, 130)},
                         {"NORMAL", sf::Color(255, 200, 50)},
                         {"HARD", sf::Color(255, 80, 90)}};

    float dx = 120.f;
    for (int i = 0; i < 3; ++i)
    {
        bool isSel = (mSelectedDifficulty == i);

        sf::RectangleShape card({720.f, 220.f});
        card.setPosition({dx, 590.f});
        card.setFillColor(isSel ? sf::Color(40, 48, 70) : sf::Color(20, 26, 42));
        card.setOutlineColor(isSel ? diffs[i].color : sf::Color(45, 60, 85));
        card.setOutlineThickness(isSel ? 3.f : 2.f);
        target.draw(card);

        sf::Text tName = TextUtil::make(font, diffs[i].name, 42);
        tName.setFillColor(isSel ? diffs[i].color : sf::Color(210, 225, 240));
        auto b = TextUtil::bounds(tName);
        tName.setPosition({dx + (720.f - b.size.x) / 2.f - b.position.x, 590.f + (220.f - b.size.y) / 2.f - b.position.y});
        target.draw(tName);

        dx += 786.f;
    }

    sf::RectangleShape startBtn({800.f, 120.f});
    startBtn.setPosition({866.f, 950.f});
    startBtn.setFillColor(startHovered ? sf::Color(0, 190, 110) : sf::Color(0, 160, 90));
    startBtn.setOutlineColor(startHovered ? sf::Color(80, 255, 200) : sf::Color(0, 255, 160));
    startBtn.setOutlineThickness(startHovered ? 4.f : 3.f);
    target.draw(startBtn);

    sf::Text sText = TextUtil::make(font, "START", 44);
    sText.setFillColor(sf::Color::White);
    auto b = TextUtil::bounds(sText);
    sText.setPosition({866.f + (800.f - b.size.x) / 2.f - b.position.x, 950.f + (120.f - b.size.y) / 2.f - b.position.y});
    target.draw(sText);
}
