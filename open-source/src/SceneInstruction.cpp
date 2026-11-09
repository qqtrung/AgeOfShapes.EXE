#include "SceneInstruction.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "TextUtil.hpp"

SceneInstruction::SceneInstruction(Game* game) : Scene(game)
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

bool SceneInstruction::input(const sf::Event& event)
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

    return false;
}

void SceneInstruction::update()
{
}

void SceneInstruction::network()
{
    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

void SceneInstruction::render()
{
    auto& window = mGame->mWindow;
    window.clear(sf::Color(14, 18, 28));
    window.setView(mGame->uiView());
    drawUI();
}

void SceneInstruction::drawUI()
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

    sf::Text title = TextUtil::make(font, "HELP", 42);
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

    sf::RectangleShape leftCard({1110.f, 890.f});
    leftCard.setPosition({120.f, 190.f});
    leftCard.setFillColor(sf::Color(20, 26, 42));
    leftCard.setOutlineColor(sf::Color(45, 60, 85));
    leftCard.setOutlineThickness(2.f);
    target.draw(leftCard);

    struct RuleItem
    {
        std::string tag;
        sf::Color   color;
    };

    RuleItem rules[4] = {{"WIN CONDITION", sf::Color(50, 255, 140)},
                         {"LOSE CONDITION", sf::Color(255, 90, 90)},
                         {"COMMAND & MOVEMENT", sf::Color(0, 220, 255)},
                         {"COMBAT TACTICS", sf::Color(255, 195, 50)}};

    float ry = 280.f;
    for (int i = 0; i < 4; ++i)
    {
        sf::Text tag = TextUtil::make(font, "> " + rules[i].tag, 36);
        tag.setFillColor(rules[i].color);
        tag.setPosition({180.f, ry});
        target.draw(tag);

        ry += 180.f;
    }

    sf::RectangleShape rightCard({1110.f, 890.f});
    rightCard.setPosition({1302.f, 190.f});
    rightCard.setFillColor(sf::Color(20, 26, 42));
    rightCard.setOutlineColor(sf::Color(45, 60, 85));
    rightCard.setOutlineThickness(2.f);
    target.draw(rightCard);

    struct WallInfo
    {
        sf::Color   color;
        std::string name;
    };

    WallInfo walls[4] = {{sf::Color(0, 220, 255), "DEFENSE WALL"},
                         {sf::Color(255, 210, 50), "COMMAND WALL"},
                         {sf::Color(190, 90, 255), "PORTAL WALL"},
                         {sf::Color(255, 120, 40), "BOOST WALL"}};

    float wy = 280.f;
    for (int i = 0; i < 4; ++i)
    {
        sf::RectangleShape badge({32.f, 32.f});
        badge.setPosition({1362.f, wy + 4.f});
        badge.setFillColor(walls[i].color);
        badge.setOutlineColor(sf::Color::White);
        badge.setOutlineThickness(2.f);
        target.draw(badge);

        sf::Text wName = TextUtil::make(font, walls[i].name, 36);
        wName.setFillColor(walls[i].color);
        wName.setPosition({1416.f, wy});
        target.draw(wName);

        wy += 180.f;
    }
}

void SceneInstruction::doAction(const Action& action)
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
