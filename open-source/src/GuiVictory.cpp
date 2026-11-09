#include "GuiVictory.hpp"

#include "FloatFixed.hpp"
#include "Game.hpp"
#include "Profiler.hpp"
#include "Scene.hpp"
#include "TextUtil.hpp"

#include <cmath>

GUIVictory::Button::Button(sf::Font& font) : mOriginalSize(250.f, 70.f), mText(TextUtil::make(font, "MENU", 30.f))
{
    mText.setFillColor(sf::Color::White);
    const sf::Vector2f windowSize = {2532.f, 1170.f};
    const sf::Vector2f btnCenter  = {windowSize.x / 2.f, windowSize.y / 2.f + 110.f};
    mText.setPosition(btnCenter);
    auto bounds = mText.getLocalBounds();
    mText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
}

GUIVictory::Button::~Button()
{
    logout("~Button()");
}

void GUIVictory::Button::update()
{
    PROFILE_FUNCTION();
}

bool GUIVictory::Button::input(const sf::Event& event)
{
    auto* guiVictory = dynamic_cast<GUIVictory*>(parent);
    if (!guiVictory)
    {
        return false;
    }

    auto* scene = dynamic_cast<Scene*>(guiVictory->parent);
    if (!scene)
    {
        return false;
    }

    sf::Vector2i touchPos;
    bool         isInput = false;

    if (const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Left)
        {
            touchPos = mouse->position;
            isInput  = true;
        }
    }
    else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
    {
        touchPos = touch->position;
        isInput  = true;
    }

    if (!isInput)
    {
        return false;
    }

    sf::Vector2f uiPos = scene->window().mapPixelToCoords(touchPos, scene->mGame->uiView());

    sf::Vector2f windowSize = {2532.f, 1170.f};
    sf::Vector2f btnCenter  = {windowSize.x / 2.f, windowSize.y / 2.f + 110.f};
    sf::Vector2f btnSize    = {420.f, 95.f};

    sf::FloatRect btnRect({btnCenter.x - btnSize.x / 2.f, btnCenter.y - btnSize.y / 2.f}, btnSize);

    if (btnRect.contains(uiPos))
    {
        Action action(Action::Type::ExitToMenu);
        pushAction(action);
        return true;
    }

    return false;
}

void GUIVictory::Button::render()
{
    PROFILE_FUNCTION();
    auto* guiVictory = dynamic_cast<GUIVictory*>(parent);
    if (!guiVictory)
    {
        return;
    }

    auto* scene = dynamic_cast<Scene*>(guiVictory->parent);
    if (!scene)
    {
        return;
    }

    auto& window = scene->window();
    window.setView(scene->mGame->uiView());
    sf::Vector2f windowSize = {2532.f, 1170.f};

    sf::Vector2f btnCenter = {windowSize.x / 2.f, windowSize.y / 2.f + 110.f};
    sf::Vector2f btnSize   = {420.f, 95.f};

    sf::Vector2f mpos = window.mapPixelToCoords(sf::Mouse::getPosition(window), scene->mGame->uiView());
    bool hovered = sf::FloatRect({btnCenter.x - btnSize.x / 2.f, btnCenter.y - btnSize.y / 2.f}, btnSize).contains(mpos);

    sf::Vector2f renderSize = btnSize;
    if (hovered)
    {
        renderSize *= 1.05f;
    }

    sf::RectangleShape rect(renderSize);
    rect.setOrigin({renderSize.x / 2.f, renderSize.y / 2.f});
    rect.setPosition(btnCenter);

    if (hovered)
    {
        rect.setFillColor(sf::Color(25, 90, 45, 220));
        rect.setOutlineColor(sf::Color(100, 255, 120, 240));
        rect.setOutlineThickness(2.f);
    }
    else
    {
        rect.setFillColor(sf::Color(15, 40, 20, 170));
        rect.setOutlineColor(sf::Color(70, 180, 100, 120));
        rect.setOutlineThickness(1.5f);
    }

    TextUtil::setSize(mText, 30.f * (hovered ? 1.05f : 1.f));

    window.draw(rect);
    window.draw(mText);
}

GUIVictory::GUIVictory(sf::Font& font) : mTitleText(TextUtil::make(font, "VICTORY!", 72.f))
{
    mTitleText.setFillColor(sf::Color(110, 255, 110));
    mTitleText.setOutlineColor(sf::Color(50, 220, 50, 180));
    mTitleText.setOutlineThickness(3.5f);
    auto bounds = mTitleText.getLocalBounds();
    mTitleText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    mTitleText.setPosition({2532.f / 2.f, 1170.f / 2.f - 50.f});

    addChild<Button>(font);
}

GUIVictory::~GUIVictory()
{
    logout("~GUIVictory()");
}

bool GUIVictory::input(const sf::Event& event)
{
    return false;
}

void GUIVictory::update()
{
    PROFILE_FUNCTION();
}

void GUIVictory::render()
{
    PROFILE_FUNCTION();
    auto* scene = dynamic_cast<Scene*>(parent);
    if (!scene)
        return;

    auto& window = scene->window();
    window.setView(scene->mGame->uiView());
    sf::Vector2f windowSize = {2532.f, 1170.f};
    float        time       = scene->mCurrentFrame * 0.1f;

    sf::VertexArray overlay(sf::PrimitiveType::TriangleStrip, 4);
    overlay[0] = sf::Vertex({0.f, 0.f}, sf::Color(0, 45, 15, 160));
    overlay[1] = sf::Vertex({windowSize.x, 0.f}, sf::Color(0, 45, 15, 160));
    overlay[2] = sf::Vertex({0.f, windowSize.y}, sf::Color(0, 15, 5, 220));
    overlay[3] = sf::Vertex({windowSize.x, windowSize.y}, sf::Color(0, 15, 5, 220));
    window.draw(overlay);

    sf::VertexArray particles(sf::PrimitiveType::Triangles, 75);
    for (int j = 0; j < 25; ++j)
    {
        float phase = j * 423.17f;
        float px    = std::fmod(phase * 8.f + time * 15.f, windowSize.x);
        float py    = std::fmod(phase * 11.f - time * 25.f, windowSize.y);
        if (py < 0.f)
            py += windowSize.y;
        float     pSize = 1.2f + std::sin(time * 2.5f + j) * 0.5f + (j % 2) * 0.6f;
        sf::Color pCol(255, 215, 0, static_cast<uint8_t>(80 + std::sin(time * 3.f + j) * 60));

        particles[j * 3 + 0] = sf::Vertex({px, py - pSize}, pCol);
        particles[j * 3 + 1] = sf::Vertex({px + pSize, py + pSize}, pCol);
        particles[j * 3 + 2] = sf::Vertex({px - pSize, py + pSize}, pCol);
    }
    window.draw(particles);

    float              bannerHeight = 150.f;
    sf::RectangleShape banner({windowSize.x, bannerHeight});
    banner.setOrigin({windowSize.x / 2.f, bannerHeight / 2.f});
    banner.setPosition({windowSize.x / 2.f, windowSize.y / 2.f - 50.f});
    banner.setFillColor(sf::Color(5, 25, 10, 200));
    banner.setOutlineColor(sf::Color(100, 255, 120, 90));
    banner.setOutlineThickness(1.5f);
    window.draw(banner);

    float pulse = std::sin(time * 2.f) * 0.05f + 1.f;
    TextUtil::setSize(mTitleText, 72.f * pulse);
    window.draw(mTitleText);
}
