#include "GuiLoss.hpp"

#include "FloatFixed.hpp"
#include "Game.hpp"
#include "Profiler.hpp"
#include "Scene.hpp"
#include "TextUtil.hpp"

#include <cmath>

GUILoss::Button::Button(sf::Font& font) : mOriginalSize(250.f, 70.f), mText(TextUtil::make(font, "MENU", 30.f))
{
    mText.setFillColor(sf::Color::White);
    const sf::Vector2f windowSize = {2532.f, 1170.f};
    const sf::Vector2f btnCenter  = {windowSize.x / 2.f, windowSize.y / 2.f + 110.f};
    mText.setPosition(btnCenter);
    auto bounds = mText.getLocalBounds();
    mText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
}

GUILoss::Button::~Button()
{
    logout("~Button()");
}

void GUILoss::Button::update()
{
    PROFILE_FUNCTION();
}

bool GUILoss::Button::input(const sf::Event& event)
{
    auto* guiLoss = dynamic_cast<GUILoss*>(parent);
    if (!guiLoss)
    {
        return false;
    }

    auto* scene = dynamic_cast<Scene*>(guiLoss->parent);
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

void GUILoss::Button::render()
{
    PROFILE_FUNCTION();
    auto* guiLoss = dynamic_cast<GUILoss*>(parent);
    if (!guiLoss)
    {
        return;
    }

    auto* scene = dynamic_cast<Scene*>(guiLoss->parent);
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
        rect.setFillColor(sf::Color(90, 25, 25, 220));
        rect.setOutlineColor(sf::Color(255, 100, 100, 240));
        rect.setOutlineThickness(2.f);
    }
    else
    {
        rect.setFillColor(sf::Color(40, 15, 15, 170));
        rect.setOutlineColor(sf::Color(180, 70, 70, 120));
        rect.setOutlineThickness(1.5f);
    }

    TextUtil::setSize(mText, 30.f * (hovered ? 1.05f : 1.f));

    window.draw(rect);
    window.draw(mText);
}

GUILoss::GUILoss(sf::Font& font) : mTitleText(TextUtil::make(font, "DEFEAT", 72.f))
{
    mTitleText.setFillColor(sf::Color(255, 90, 90));
    mTitleText.setOutlineColor(sf::Color(255, 30, 30, 180));
    mTitleText.setOutlineThickness(3.5f);
    auto bounds = mTitleText.getLocalBounds();
    mTitleText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    mTitleText.setPosition({2532.f / 2.f, 1170.f / 2.f - 50.f});

    addChild<Button>(font);
}

GUILoss::~GUILoss()
{
    logout("~GUILoss()");
}

bool GUILoss::input(const sf::Event&)
{
    return false;
}

void GUILoss::update()
{
    PROFILE_FUNCTION();
}

void GUILoss::render()
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
    overlay[0] = sf::Vertex({0.f, 0.f}, sf::Color(45, 0, 0, 160));
    overlay[1] = sf::Vertex({windowSize.x, 0.f}, sf::Color(45, 0, 0, 160));
    overlay[2] = sf::Vertex({0.f, windowSize.y}, sf::Color(15, 0, 0, 220));
    overlay[3] = sf::Vertex({windowSize.x, windowSize.y}, sf::Color(15, 0, 0, 220));
    window.draw(overlay);

    sf::VertexArray particles(sf::PrimitiveType::Triangles, 75);
    for (int j = 0; j < 25; ++j)
    {
        float     phase = j * 314.15f;
        float     px    = std::fmod(phase * 7.f, windowSize.x) + std::sin(time * 1.5f + j) * 15.f;
        float     py    = windowSize.y - std::fmod(phase * 13.f + time * 35.f, windowSize.y);
        float     pSize = 1.0f + std::sin(time * 2.f + j) * 0.4f + (j % 3) * 0.8f;
        sf::Color pCol(255, 60 + (j % 5) * 30, 20, static_cast<uint8_t>(70 + std::sin(time * 3.f + j) * 50));

        particles[j * 3 + 0] = sf::Vertex({px, py - pSize}, pCol);
        particles[j * 3 + 1] = sf::Vertex({px + pSize, py + pSize}, pCol);
        particles[j * 3 + 2] = sf::Vertex({px - pSize, py + pSize}, pCol);
    }
    window.draw(particles);

    float              bannerHeight = 150.f;
    sf::RectangleShape banner({windowSize.x, bannerHeight});
    banner.setOrigin({windowSize.x / 2.f, bannerHeight / 2.f});
    banner.setPosition({windowSize.x / 2.f, windowSize.y / 2.f - 50.f});
    banner.setFillColor(sf::Color(25, 5, 5, 200));
    banner.setOutlineColor(sf::Color(255, 50, 50, 90));
    banner.setOutlineThickness(1.5f);
    window.draw(banner);

    float pulse = std::sin(time * 2.f) * 0.05f + 1.f;
    TextUtil::setSize(mTitleText, 72.f * pulse);
    window.draw(mTitleText);
}
