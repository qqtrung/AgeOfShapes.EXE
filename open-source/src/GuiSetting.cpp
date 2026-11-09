#include "GuiSetting.hpp"

#include "FloatFixed.hpp"
#include "Game.hpp"
#include "Profiler.hpp"
#include "SceneSetting.hpp"
#include "TextUtil.hpp"

#include <cmath>

GUISetting::Button::Button(ButtonType type, sf::Font& font) :
    mType(type),
    mOriginalSize(36.f, 36.f),
    mText(font, "", TextUtil::kBaseCharSize)
{
    mText.setFillColor(sf::Color::White);

    const char* label = "";
    switch (mType)
    {
        case ButtonType::MasterVolumeMinus:
        case ButtonType::MusicVolumeMinus:
        case ButtonType::SfxVolumeMinus:
        case ButtonType::GraphicsMinus:
            label = "-";
            break;
        case ButtonType::MasterVolumePlus:
        case ButtonType::MusicVolumePlus:
        case ButtonType::SfxVolumePlus:
        case ButtonType::GraphicsPlus:
            label = "+";
            break;
        case ButtonType::Back:
            label         = "Back";
            mOriginalSize = {180.f, 42.f};
            break;
    }
    mText.setString(label);
}

GUISetting::Button::~Button()
{
    logout("~Button()");
}

void GUISetting::Button::update()
{
    PROFILE_FUNCTION();
}

bool GUISetting::Button::input(const sf::Event& event)
{
    auto* guiSetting = dynamic_cast<GUISetting*>(parent);
    if (!guiSetting)
    {
        return false;
    }

    auto* sceneSetting = dynamic_cast<SceneSetting*>(guiSetting->parent);
    if (!sceneSetting)
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

    sf::Vector2f mpos = sceneSetting->mGame->mWindow.mapPixelToCoords(sf::Vector2i((int)touchPos.x, (int)touchPos.y),
                                                                      sceneSetting->mGame->uiView());

    sf::Vector2f viewSize = {2532.f, 1170.f};
    float        cx       = viewSize.x / 2.f;
    float        cy       = viewSize.y;
    float        scale    = cy / 720.f;

    auto contains = [&](sf::Vector2f centerPos, sf::Vector2f size)
    { return sf::FloatRect({centerPos.x - size.x / 2.f, centerPos.y - size.y / 2.f}, size).contains(mpos); };

    sf::Vector2f clickBtnSize(54.f * scale, 54.f * scale);
    sf::Vector2f btnPos;
    sf::Vector2f btnSize;

    if (mType == ButtonType::MasterVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.28f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::MasterVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.28f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::MusicVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.40f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::MusicVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.40f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::SfxVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.52f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::SfxVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.52f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::GraphicsMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.64f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::GraphicsPlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.64f};
        btnSize = clickBtnSize;
    }
    else if (mType == ButtonType::Back)
    {
        btnPos  = {cx, cy * 0.84f};
        btnSize = {240.f * scale, 56.f * scale};
    }

    if (!contains(btnPos, btnSize))
    {
        return false;
    }

    if (mType == ButtonType::MasterVolumeMinus)
    {
        Action action(Action::Type::MasterVolumeMinus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::MasterVolumePlus)
    {
        Action action(Action::Type::MasterVolumePlus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::MusicVolumeMinus)
    {
        Action action(Action::Type::MusicVolumeMinus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::MusicVolumePlus)
    {
        Action action(Action::Type::MusicVolumePlus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::SfxVolumeMinus)
    {
        Action action(Action::Type::SfxVolumeMinus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::SfxVolumePlus)
    {
        Action action(Action::Type::SfxVolumePlus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::GraphicsMinus)
    {
        Action action(Action::Type::GraphicsMinus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::GraphicsPlus)
    {
        Action action(Action::Type::GraphicsPlus);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::Back)
    {
        Action action(Action::Type::Back);
        pushAction(action);
        return true;
    }

    return false;
}

void GUISetting::Button::render()
{
    PROFILE_FUNCTION();
    auto* guiSetting = dynamic_cast<GUISetting*>(parent);
    if (!guiSetting)
    {
        return;
    }

    auto* sceneSetting = dynamic_cast<SceneSetting*>(guiSetting->parent);
    if (!sceneSetting)
    {
        return;
    }

    auto& window = sceneSetting->mGame->mWindow;
    window.setView(sceneSetting->mGame->uiView());
    sf::Vector2f viewSize = {2532.f, 1170.f};

    float cx    = viewSize.x / 2.f;
    float cy    = viewSize.y;
    float scale = cy / 720.f;

    sf::Vector2f btnPos;
    sf::Vector2f btnSize;

    if (mType == ButtonType::MasterVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.28f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::MasterVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.28f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::MusicVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.40f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::MusicVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.40f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::SfxVolumeMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.52f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::SfxVolumePlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.52f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::GraphicsMinus)
    {
        btnPos  = {cx - 160.f * scale, cy * 0.64f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::GraphicsPlus)
    {
        btnPos  = {cx + 160.f * scale, cy * 0.64f};
        btnSize = {46.f * scale, 46.f * scale};
    }
    else if (mType == ButtonType::Back)
    {
        btnPos  = {cx, cy * 0.84f};
        btnSize = {240.f * scale, 56.f * scale};
    }

    sf::Vector2f mpos = window.mapPixelToCoords(sf::Mouse::getPosition(window), sceneSetting->mGame->uiView());
    bool hovered      = sf::FloatRect({btnPos.x - btnSize.x / 2.f, btnPos.y - btnSize.y / 2.f}, btnSize).contains(mpos);

    sf::Vector2f renderSize = btnSize;
    if (hovered)
    {
        renderSize *= 1.05f;
    }

    sf::RectangleShape rect(renderSize);
    rect.setOrigin({renderSize.x / 2.f, renderSize.y / 2.f});
    rect.setPosition(btnPos);

    if (hovered)
    {
        rect.setFillColor(sf::Color(55, 75, 105, 190));
        rect.setOutlineColor(sf::Color(100, 220, 255, 230));
        rect.setOutlineThickness(2.f * scale);
        mText.setFillColor(sf::Color::White);
    }
    else
    {
        rect.setFillColor(sf::Color(35, 45, 65, 140));
        rect.setOutlineColor(sf::Color(110, 140, 180, 120));
        rect.setOutlineThickness(1.5f * scale);
        mText.setFillColor(sf::Color(220, 230, 255));
    }

    TextUtil::setSize(mText, 20.f * scale);
    auto bounds = mText.getLocalBounds();
    mText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    mText.setPosition(btnPos);

    window.draw(rect);
    window.draw(mText);
}

GUISetting::LabelData::LabelData(sf::Font& font) : text(font, "", TextUtil::kBaseCharSize)
{
    text.setFillColor(sf::Color::White);
}

GUISetting::GUISetting(sf::Font& font) :
    mBtnText(font, "", TextUtil::kBaseCharSize),
    mLabelText(font, "", TextUtil::kBaseCharSize),
    mLabels{LabelData(font), LabelData(font), LabelData(font), LabelData(font)}
{
    mBtnText.setFillColor(sf::Color::White);
    mLabelText.setFillColor(sf::Color::White);

    addChild<Button>(Button::ButtonType::MasterVolumeMinus, font);
    addChild<Button>(Button::ButtonType::MasterVolumePlus, font);
    addChild<Button>(Button::ButtonType::MusicVolumeMinus, font);
    addChild<Button>(Button::ButtonType::MusicVolumePlus, font);
    addChild<Button>(Button::ButtonType::SfxVolumeMinus, font);
    addChild<Button>(Button::ButtonType::SfxVolumePlus, font);
    addChild<Button>(Button::ButtonType::GraphicsMinus, font);
    addChild<Button>(Button::ButtonType::GraphicsPlus, font);
    addChild<Button>(Button::ButtonType::Back, font);
}

GUISetting::~GUISetting()
{
    logout("~GUISetting()");
}

bool GUISetting::input(const sf::Event& event)
{
    if (auto* sceneSetting = dynamic_cast<SceneSetting*>(parent))
    {
        std::optional<sf::Vector2f> clickPos;

        if (const auto* mouseButton = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (mouseButton->button == sf::Mouse::Button::Left)
            {
                clickPos = sf::Vector2f((float)mouseButton->position.x, (float)mouseButton->position.y);
            }
        }
        else if (const auto* touch = event.getIf<sf::Event::TouchBegan>())
        {
            clickPos = sf::Vector2f((float)touch->position.x, (float)touch->position.y);
        }

        if (clickPos)
        {
            sf::Vector2f mpos = sceneSetting->mGame->mWindow.mapPixelToCoords(sf::Vector2i((int)clickPos->x,
                                                                                           (int)clickPos->y),
                                                                              sceneSetting->mGame->uiView());

            sf::Vector2f viewSize = {2532.f, 1170.f};
            float        cx       = viewSize.x / 2.f;
            float        cy       = viewSize.y;
            float        scale    = cy / 720.f;

            auto contains = [&](sf::Vector2f centerPos, sf::Vector2f size)
            { return sf::FloatRect({centerPos.x - size.x / 2.f, centerPos.y - size.y / 2.f}, size).contains(mpos); };

            float graphicsY = cy * 0.64f;
            float spacing   = 80.f * scale;
            if (contains({cx - spacing, graphicsY}, {75.f * scale, 32.f * scale}))
            {
                Action action(Action::Type::GraphicsQuality, 0);
                pushAction(action);
                return true;
            }

            if (contains({cx, graphicsY}, {75.f * scale, 32.f * scale}))
            {
                Action action(Action::Type::GraphicsQuality, 1);
                pushAction(action);
                return true;
            }

            if (contains({cx + spacing, graphicsY}, {75.f * scale, 32.f * scale}))
            {
                Action action(Action::Type::GraphicsQuality, 2);
                pushAction(action);
                return true;
            }
        }
    }

    return false;
}

void GUISetting::update()
{
    PROFILE_FUNCTION();
}

void GUISetting::drawLabel(sf::RenderTarget& target, LabelData& label, const std::string& str, sf::Vector2f pos, unsigned int charSize)
{
    label.text.setString(str);
    TextUtil::setSize(label.text, static_cast<float>(charSize));
    label.text.setFillColor(sf::Color(220, 230, 255));

    auto bounds = label.text.getLocalBounds();
    label.text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    label.text.setPosition(pos);

    target.draw(label.text);
}

void GUISetting::drawProgressBar(sf::RenderTarget& target, sf::Vector2f centerPos, float value, sf::Color fillColor, float scale)
{
    float barWidth  = 220.f * scale;
    float barHeight = 8.f * scale;

    sf::RectangleShape track({barWidth, barHeight});
    track.setOrigin({barWidth / 2.f, barHeight / 2.f});
    track.setPosition(centerPos);
    track.setFillColor(sf::Color(40, 50, 70, 130));
    track.setOutlineColor(sf::Color(100, 120, 150, 80));
    track.setOutlineThickness(1.f * scale);
    target.draw(track);

    if (value > 0.0f)
    {
        float              fillWidth = barWidth * value;
        sf::RectangleShape fill({fillWidth, barHeight});
        fill.setOrigin({0.f, barHeight / 2.f});
        fill.setPosition({centerPos.x - barWidth / 2.f, centerPos.y});
        fill.setFillColor(fillColor);

        fill.setOutlineColor(sf::Color(255, 255, 255, 120));
        fill.setOutlineThickness(0.5f * scale);
        target.draw(fill);
    }
}

void GUISetting::drawSegmentSelector(
    sf::RenderTarget& target,
    sf::Vector2f      centerPos,
    int               selectedIndex,
    sf::Font&         font,
    sf::Vector2f      mousePos,
    float             scale)
{
    std::array<std::string, 3> options     = {"Low", "Medium", "High"};
    float                      blockWidth  = 75.f * scale;
    float                      blockHeight = 32.f * scale;
    float                      spacing     = 80.f * scale;

    for (int i = 0; i < 3; ++i)
    {
        sf::Vector2f  pos = {centerPos.x + (i - 1) * spacing, centerPos.y};
        sf::FloatRect rect({pos.x - blockWidth / 2.f, pos.y - blockHeight / 2.f}, {blockWidth, blockHeight});
        bool          isHovered  = rect.contains(mousePos);
        bool          isSelected = (i == selectedIndex);

        sf::RectangleShape box({blockWidth, blockHeight});
        box.setOrigin({blockWidth / 2.f, blockHeight / 2.f});
        box.setPosition(pos);

        if (isSelected)
        {
            box.setFillColor(sf::Color(0, 205, 120, 180));
            box.setOutlineColor(sf::Color(200, 255, 220, 220));
            box.setOutlineThickness(1.5f * scale);
        }
        else if (isHovered)
        {
            box.setFillColor(sf::Color(55, 75, 105, 190));
            box.setOutlineColor(sf::Color(100, 220, 255, 230));
            box.setOutlineThickness(1.5f * scale);
        }
        else
        {
            box.setFillColor(sf::Color(35, 45, 65, 120));
            box.setOutlineColor(sf::Color(110, 140, 180, 80));
            box.setOutlineThickness(1.f * scale);
        }

        target.draw(box);

        sf::Text text = TextUtil::make(font, options[i], 14.f * scale);
        text.setFillColor(sf::Color::White);
        auto bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        text.setPosition(pos);
        target.draw(text);
    }
}

void GUISetting::render()
{
    PROFILE_FUNCTION();
    if (auto* sceneSetting = dynamic_cast<SceneSetting*>(parent))
    {
        auto& mGame = sceneSetting->mGame;
        mGame->mWindow.setView(mGame->uiView());
        sf::Vector2f viewSize = {2532.f, 1170.f};
        float        cx       = viewSize.x / 2.f;
        float        cy       = viewSize.y;
        float        scale    = cy / 720.f;
        float        time     = sceneSetting->mCurrentFrame * 0.1f;

        sf::VertexArray background(sf::PrimitiveType::TriangleStrip, 4);
        background[0] = sf::Vertex({0.f, 0.f}, sf::Color(10, 15, 28));
        background[1] = sf::Vertex({viewSize.x, 0.f}, sf::Color(10, 15, 28));
        background[2] = sf::Vertex({0.f, viewSize.y}, sf::Color(24, 16, 36));
        background[3] = sf::Vertex({viewSize.x, viewSize.y}, sf::Color(24, 16, 36));
        mGame->mWindow.draw(background);

        sf::VertexArray lines(sf::PrimitiveType::Lines);
        sf::Color       lineCol(0, 200, 255, 10);
        float           gridSpacing = 160.f;
        for (float offset = -viewSize.x; offset < viewSize.x; offset += gridSpacing)
        {
            lines.append(sf::Vertex({offset + std::sin(time * 0.12f) * 12.f, 0.f}, lineCol));
            lines.append(sf::Vertex({offset + viewSize.y + std::sin(time * 0.12f) * 12.f, viewSize.y}, lineCol));
        }
        mGame->mWindow.draw(lines);

        sf::VertexArray particles(sf::PrimitiveType::Triangles);
        for (int j = 0; j < 24; ++j)
        {
            float     phase = j * 153.25f;
            float     px    = std::fmod(phase * 15.f + time * 15.f, viewSize.x);
            float     py    = viewSize.y - std::fmod(phase * 11.f + time * 25.f, viewSize.y);
            float     pSize = 2.8f + std::sin(time * 1.8f + j) * 0.8f + (j % 3) * 0.5f;
            sf::Color pCol(0, 240, 255, (uint8_t)(80 + std::sin(time * 2.2f + j) * 55));

            particles.append(sf::Vertex({px, py - pSize}, pCol));
            particles.append(sf::Vertex({px + pSize, py + pSize}, pCol));
            particles.append(sf::Vertex({px - pSize, py + pSize}, pCol));
        }
        mGame->mWindow.draw(particles);

        static sf::Text titleText{mGame->mAssets.fontMap.at("Consolas"), "", TextUtil::kBaseCharSize};
        titleText.setString("SETTINGS");
        float titleSize  = std::clamp(viewSize.y * 0.055f, 24.f, 44.f);
        float titlePulse = std::sin(time * 2.f) * 0.02f + 1.f;
        TextUtil::setSize(titleText, titleSize * titlePulse);
        auto titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(
            {titleBounds.position.x + titleBounds.size.x / 2.f, titleBounds.position.y + titleBounds.size.y / 2.f});
        titleText.setPosition({cx, viewSize.y * 0.12f});

        titleText.setFillColor(sf::Color(0, 200, 255, 60));
        titleText.setOutlineThickness(2.5f * scale);
        titleText.setOutlineColor(sf::Color(0, 180, 255, 70));
        mGame->mWindow.draw(titleText);

        titleText.setFillColor(sf::Color::White);
        titleText.setOutlineThickness(0.f);
        mGame->mWindow.draw(titleText);

        sf::Vector2f mpos = mGame->mWindow.mapPixelToCoords(sf::Mouse::getPosition(mGame->mWindow), mGame->uiView());

        auto labelTextSize = static_cast<unsigned int>(17.f * scale);

        float masterY = cy * 0.28f;
        drawLabel(mGame->mWindow,
                  mLabels[0],
                  "Master Volume: " + std::to_string((int)(sceneSetting->mMasterVolume * 100)) + "%",
                  {cx, masterY - 26.f * scale},
                  labelTextSize);
        drawProgressBar(mGame->mWindow, {cx, masterY}, sceneSetting->mMasterVolume, sf::Color(0, 200, 255, 230), scale);

        float musicY = cy * 0.40f;
        drawLabel(mGame->mWindow,
                  mLabels[1],
                  "Music Volume: " + std::to_string((int)(sceneSetting->mMusicVolume * 100)) + "%",
                  {cx, musicY - 26.f * scale},
                  labelTextSize);
        drawProgressBar(mGame->mWindow, {cx, musicY}, sceneSetting->mMusicVolume, sf::Color(180, 100, 255, 230), scale);

        float sfxY = cy * 0.52f;
        drawLabel(mGame->mWindow,
                  mLabels[2],
                  "SFX Volume: " + std::to_string((int)(sceneSetting->mSfxVolume * 100)) + "%",
                  {cx, sfxY - 26.f * scale},
                  labelTextSize);
        drawProgressBar(mGame->mWindow, {cx, sfxY}, sceneSetting->mSfxVolume, sf::Color(255, 180, 50, 230), scale);

        float graphicsY = cy * 0.64f;
        drawLabel(mGame->mWindow, mLabels[3], "Graphics Quality", {cx, graphicsY - 26.f * scale}, labelTextSize);
        drawSegmentSelector(mGame->mWindow, {cx, graphicsY}, mGame->mGraphicsLevel, mGame->mAssets.fontMap.at("Consolas"), mpos, scale);
    }
}
