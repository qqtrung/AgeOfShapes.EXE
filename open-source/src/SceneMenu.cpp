#include "SceneMenu.hpp"

#include "Action.hpp"
#include "Game.hpp"
#include "TextUtil.hpp"

SceneMenu::SceneMenu(Game* game) : Scene(game)
{

    if (mGame)
    {
        mGame->mNetwork.ingoingPacket.clear();

        if (mGame->isConnected())
        {
            Packet discPkt;
            discPkt.type = PacketType::Disconnect;
            discPkt.data = PacketDisconnect{mGame->mId, "Player is in Main Menu"};
            mGame->sendPacket(std::move(discPkt));
        }

        mGame->stopNetwork();
    }
}

bool SceneMenu::input(const sf::Event& event)
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

    if (sf::FloatRect({2180.f, 25.f}, {280.f, 80.f}).contains(mpos))
    {
        mGame->quit();
        return true;
    }

    if (sf::FloatRect({180.f, 230.f}, {1040.f, 230.f}).contains(mpos))
    {
        mGame->pushScene("SinglePlayer");
        return true;
    }

    if (sf::FloatRect({180.f, 520.f}, {1040.f, 230.f}).contains(mpos))
    {
        mGame->pushScene("Multiplayer");
        return true;
    }

    if (sf::FloatRect({180.f, 810.f}, {1040.f, 230.f}).contains(mpos))
    {
        if (mGame->mLatestReplay.hasReplay)
        {
            mGame->pushScene("Replay",
                             mGame->mLatestReplay.level,
                             mGame->mLatestReplay.difficulty,
                             mGame->mLatestReplay.playerCount);
        }
        else
        {
            mGame->pushScene("Replay", 1, 1, 1);
        }
        return true;
    }

    if (sf::FloatRect({1312.f, 230.f}, {1040.f, 230.f}).contains(mpos))
    {
        mGame->pushScene("Setting");
        return true;
    }

    if (sf::FloatRect({1312.f, 520.f}, {1040.f, 230.f}).contains(mpos))
    {
        mGame->pushScene("Instruction");
        return true;
    }

    if (sf::FloatRect({1312.f, 810.f}, {1040.f, 230.f}).contains(mpos))
    {
        return true;
    }

    return false;
}

void SceneMenu::update()
{
}

void SceneMenu::network()
{
    if (mGame && !mGame->m_pendingScene)
    {
        mGame->mNetwork.ingoingPacket.clear();
    }
}

void SceneMenu::render()
{
    auto&        window = mGame->mWindow;
    sf::Vector2f mpos   = window.mapPixelToCoords(sf::Mouse::getPosition(window), mGame->uiView());

    int hoverIndex = -1;
    if (sf::FloatRect({2180.f, 25.f}, {280.f, 80.f}).contains(mpos))
    {
        hoverIndex = 0;
    }
    else
    {
        const sf::Vector2f positions[6] =
            {{180.f, 230.f}, {180.f, 520.f}, {180.f, 810.f}, {1312.f, 230.f}, {1312.f, 520.f}, {1312.f, 810.f}};
        const sf::Vector2f size{1040.f, 230.f};
        for (int i = 0; i < 6; ++i)
        {
            if (sf::FloatRect(positions[i], size).contains(mpos))
            {
                hoverIndex = i + 1;
                break;
            }
        }
    }

    window.clear(sf::Color(14, 18, 28));
    window.setView(mGame->uiView());
    drawUI(hoverIndex);
}

void SceneMenu::drawUI(int hoverIndex)
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

    sf::Text title = TextUtil::make(font, "MENU", 42);
    title.setFillColor(sf::Color(0, 230, 255));
    title.setPosition({80.f, 38.f});
    target.draw(title);

    bool               exitHovered = (hoverIndex == 0);
    sf::RectangleShape exitBtn({280.f, 80.f});
    exitBtn.setPosition({2180.f, 25.f});
    exitBtn.setFillColor(exitHovered ? sf::Color(55, 70, 95) : sf::Color(38, 48, 68));
    exitBtn.setOutlineColor(exitHovered ? sf::Color(255, 85, 95) : sf::Color(80, 110, 150));
    exitBtn.setOutlineThickness(2.f);
    target.draw(exitBtn);

    sf::Text eText = TextUtil::make(font, "EXIT", 30);
    eText.setFillColor(exitHovered ? sf::Color(255, 100, 110) : sf::Color::White);
    auto eb = TextUtil::bounds(eText);
    eText.setPosition({2180.f + (280.f - eb.size.x) / 2.f - eb.position.x, 25.f + (80.f - eb.size.y) / 2.f - eb.position.y});
    target.draw(eText);

    struct MenuItem
    {
        sf::Vector2f pos;
        sf::Vector2f size;
        std::string  title;
        sf::Color    color;
    };

    MenuItem items[6] = {{{180.f, 230.f}, {1040.f, 230.f}, "SINGLE PLAYER", sf::Color(255, 195, 50)},
                         {{180.f, 520.f}, {1040.f, 230.f}, "MULTIPLAYER", sf::Color(0, 220, 255)},
                         {{180.f, 810.f}, {1040.f, 230.f}, "REPLAY", sf::Color(50, 235, 140)},
                         {{1312.f, 230.f}, {1040.f, 230.f}, "SETTINGS", sf::Color(140, 190, 255)},
                         {{1312.f, 520.f}, {1040.f, 230.f}, "HELP", sf::Color(210, 150, 255)},
                         {{1312.f, 810.f}, {1040.f, 230.f}, "LANGUAGE", sf::Color(255, 140, 60)}};

    for (int i = 0; i < 6; ++i)
    {
        const auto& it      = items[i];
        bool        hovered = (hoverIndex == i + 1);

        sf::RectangleShape card(it.size);
        card.setPosition(it.pos);
        card.setFillColor(hovered ? sf::Color(32, 44, 68) : sf::Color(20, 26, 42));
        card.setOutlineColor(hovered ? it.color : sf::Color(45, 60, 85));
        card.setOutlineThickness(hovered ? 3.f : 2.f);
        target.draw(card);

        sf::RectangleShape bar({16.f, it.size.y});
        bar.setPosition(it.pos);
        bar.setFillColor(it.color);
        target.draw(bar);

        sf::Text tTitle = TextUtil::make(font, it.title, 38);
        tTitle.setFillColor(hovered ? sf::Color::White : it.color);
        tTitle.setPosition({it.pos.x + 60.f, it.pos.y + 85.f});
        target.draw(tTitle);
    }
}

void SceneMenu::doAction(const Action& action)
{
    switch (action.type)
    {
        case Action::Type::SinglePlayer:
        case Action::Type::Start:
            mGame->pushScene("SinglePlayer");
            break;
        case Action::Type::Multiplayer:
            mGame->pushScene("Multiplayer");
            break;
        case Action::Type::Settings:
            mGame->pushScene("Setting");
            break;
        case Action::Type::Instruction:
            mGame->pushScene("Instruction");
            break;
        case Action::Type::Replay:
            if (mGame->mLatestReplay.hasReplay)
            {
                mGame->pushScene("Replay",
                                 mGame->mLatestReplay.level,
                                 mGame->mLatestReplay.difficulty,
                                 mGame->mLatestReplay.playerCount);
            }
            else
            {
                mGame->pushScene("Replay", 1, 1, 1);
            }
            break;
        case Action::Type::Exit:
            mGame->quit();
            break;
        default:
            Scene::doAction(action);
            break;
    }
}
