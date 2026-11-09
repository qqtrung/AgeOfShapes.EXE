#include "GuiInstruction.hpp"

#include "FloatFixed.hpp"
#include "Game.hpp"
#include "Profiler.hpp"
#include "SceneInstruction.hpp"
#include "TextUtil.hpp"

#include <cmath>

GUIInstruction::Button::Button(ButtonType type, sf::Font& font) :
    mType(type),
    mOriginalSize(180.f, 45.f),
    mText(font, "", TextUtil::kBaseCharSize)
{
    mText.setFillColor(sf::Color::White);

    const char* label = "";
    switch (mType)
    {
        case ButtonType::Vietnamese:
            label = "Vietnamese";
            break;
        case ButtonType::English:
            label = "English";
            break;
        case ButtonType::Back:
            label         = "Back";
            mOriginalSize = {180.f, 42.f};
            break;
    }
    mText.setString(label);
}

GUIInstruction::Button::~Button()
{
    logout("~Button()");
}

void GUIInstruction::Button::update()
{
    PROFILE_FUNCTION();
}

bool GUIInstruction::Button::input(const sf::Event& event)
{
    auto* guiInstruction = dynamic_cast<GUIInstruction*>(parent);
    if (!guiInstruction)
    {
        return false;
    }

    auto* sceneInstruction = dynamic_cast<SceneInstruction*>(guiInstruction->parent);
    if (!sceneInstruction)
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

    auto& window = sceneInstruction->mGame->mWindow;

    sf::Vector2f uiPos = window.mapPixelToCoords({touchPos.x, touchPos.y}, sceneInstruction->mGame->uiView());

    sf::Vector2f viewSize = {2532.f, 1170.f};
    float        cx       = viewSize.x / 2.f;
    float        cy       = viewSize.y;
    float        scale    = cy / 720.f;

    sf::Vector2f btnPos;
    sf::Vector2f btnSize;

    if (mType == ButtonType::Vietnamese)
    {
        btnPos  = {cx - 130.f * scale, cy * 0.17f};
        btnSize = {220.f * scale, 52.f * scale};
    }
    else if (mType == ButtonType::English)
    {
        btnPos  = {cx + 130.f * scale, cy * 0.17f};
        btnSize = {220.f * scale, 52.f * scale};
    }
    else if (mType == ButtonType::Back)
    {
        btnPos  = {cx, cy * 0.92f};
        btnSize = {220.f * scale, 52.f * scale};
    }

    sf::FloatRect btnRect({btnPos.x - btnSize.x / 2.f, btnPos.y - btnSize.y / 2.f}, btnSize);
    if (!btnRect.contains(uiPos))
    {
        return false;
    }

    if (mType == ButtonType::Vietnamese)
    {
        Action action(Action::Type::Vietnamese);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::English)
    {
        Action action(Action::Type::English);
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

void GUIInstruction::Button::render()
{
    PROFILE_FUNCTION();
    auto* guiInstruction = dynamic_cast<GUIInstruction*>(parent);
    if (!guiInstruction)
    {
        return;
    }

    auto* sceneInstruction = dynamic_cast<SceneInstruction*>(guiInstruction->parent);
    if (!sceneInstruction)
    {
        return;
    }

    auto& window = sceneInstruction->mGame->mWindow;
    window.setView(sceneInstruction->mGame->uiView());

    sf::Vector2f viewSize = {2532.f, 1170.f};
    float        cx       = viewSize.x / 2.f;
    float        cy       = viewSize.y;
    float        scale    = cy / 720.f;

    sf::Vector2f btnPos;
    sf::Vector2f btnSize;
    bool         selected = false;

    if (mType == ButtonType::Vietnamese)
    {
        btnPos   = {cx - 130.f * scale, cy * 0.17f};
        btnSize  = {220.f * scale, 52.f * scale};
        selected = sceneInstruction->mShowVietnamese;
    }
    else if (mType == ButtonType::English)
    {
        btnPos   = {cx + 130.f * scale, cy * 0.17f};
        btnSize  = {220.f * scale, 52.f * scale};
        selected = !sceneInstruction->mShowVietnamese;
    }
    else if (mType == ButtonType::Back)
    {
        btnPos  = {cx, cy * 0.92f};
        btnSize = {220.f * scale, 52.f * scale};
    }

    sf::Vector2f mpos = window.mapPixelToCoords(sf::Mouse::getPosition(window), sceneInstruction->mGame->uiView());
    bool hovered      = sf::FloatRect({btnPos.x - btnSize.x / 2.f, btnPos.y - btnSize.y / 2.f}, btnSize).contains(mpos);

    sf::Vector2f renderSize = btnSize;
    if (hovered)
    {
        renderSize *= 1.04f;
    }

    sf::RectangleShape rect(renderSize);
    rect.setOrigin({renderSize.x / 2.f, renderSize.y / 2.f});
    rect.setPosition(btnPos);

    if (selected)
    {
        rect.setFillColor(sf::Color(0, 205, 120, 180));
        rect.setOutlineColor(sf::Color(200, 255, 220, 220));
        rect.setOutlineThickness(2.f * scale);
        mText.setFillColor(sf::Color::White);
    }
    else if (hovered)
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

GUIInstruction::LabelData::LabelData(sf::Font& font) : text(font, "", TextUtil::kBaseCharSize)
{
    text.setFillColor(sf::Color::White);
}

GUIInstruction::GUIInstruction(sf::Font& font) :
    mLabels{LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font),
            LabelData(font)}
{
    addChild<Button>(Button::ButtonType::Vietnamese, font);
    addChild<Button>(Button::ButtonType::English, font);
    addChild<Button>(Button::ButtonType::Back, font);
}

GUIInstruction::~GUIInstruction()
{
    logout("~GUIInstruction()");
}

bool GUIInstruction::input(const sf::Event&)
{
    return false;
}

void GUIInstruction::update()
{
    PROFILE_FUNCTION();
}

void GUIInstruction::drawPanel(sf::RenderTarget&  target,
                               sf::FloatRect      rect,
                               const std::string& title,
                               sf::Font&          font,
                               float              scale,
                               sf::Color          outlineColor)
{
    sf::RectangleShape panelShape({rect.size.x, rect.size.y});
    panelShape.setPosition({rect.position.x, rect.position.y});
    panelShape.setFillColor(sf::Color(20, 26, 42, 180));
    panelShape.setOutlineColor(outlineColor);
    panelShape.setOutlineThickness(1.5f * scale);
    target.draw(panelShape);

    sf::RectangleShape accent({70.f * scale, 3.f * scale});
    accent.setPosition({rect.position.x, rect.position.y});
    accent.setFillColor(sf::Color(0, 200, 255, 220));
    target.draw(accent);

    sf::Text titleText = TextUtil::make(font, title, 20.f * scale);
    titleText.setFillColor(sf::Color(0, 205, 255));
    titleText.setStyle(sf::Text::Bold);
    titleText.setPosition({rect.position.x + 20.f * scale, rect.position.y + 12.f * scale});
    target.draw(titleText);
}

void GUIInstruction::drawLabel(
    sf::RenderTarget&  target,
    LabelData&         label,
    const std::string& str,
    sf::Vector2f       pos,
    unsigned int       charSize,
    sf::Color          color,
    bool               bold)
{
    label.text.setString(str);
    TextUtil::setSize(label.text, static_cast<float>(charSize));
    label.text.setFillColor(color);
    if (bold)
        label.text.setStyle(sf::Text::Bold);
    else
        label.text.setStyle(sf::Text::Regular);

    label.text.setOrigin({0.f, label.text.getLocalBounds().size.y / 2.f});
    label.text.setPosition(pos);
    target.draw(label.text);
}

void GUIInstruction::render()
{
    PROFILE_FUNCTION();
    if (auto* sceneInstruction = dynamic_cast<SceneInstruction*>(parent))
    {
        auto& mGame = sceneInstruction->mGame;
        mGame->mWindow.setView(mGame->uiView());
        sf::Vector2f viewSize = {2532.f, 1170.f};
        float        cx       = viewSize.x / 2.f;
        float        cy       = viewSize.y;
        float        scale    = cy / 720.f;
        float        time     = mGame->mClockGui.getElapsedTime().asSeconds();

        sf::VertexArray background(sf::PrimitiveType::TriangleStrip, 4);
        background[0] = sf::Vertex({0.f, 0.f}, sf::Color(12, 16, 30));
        background[1] = sf::Vertex({viewSize.x, 0.f}, sf::Color(12, 16, 30));
        background[2] = sf::Vertex({0.f, viewSize.y}, sf::Color(22, 14, 34));
        background[3] = sf::Vertex({viewSize.x, viewSize.y}, sf::Color(22, 14, 34));
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
            sf::Color pCol(0, 200, 255, (uint8_t)(80 + std::sin(time * 2.2f + j) * 55));

            particles.append(sf::Vertex({px, py - pSize}, pCol));
            particles.append(sf::Vertex({px + pSize, py + pSize}, pCol));
            particles.append(sf::Vertex({px - pSize, py + pSize}, pCol));
        }
        mGame->mWindow.draw(particles);

        static sf::Text titleText{mGame->mAssets.fontMap.at("Consolas"), "", TextUtil::kBaseCharSize};
        titleText.setString(sceneInstruction->mShowVietnamese ? "HUONG DAN CHOI" : "HOW TO PLAY");
        float titleSize  = std::clamp(viewSize.y * 0.055f, 24.f, 44.f);
        float titlePulse = std::sin(time * 2.f) * 0.02f + 1.f;
        TextUtil::setSize(titleText, titleSize * titlePulse);
        auto titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(
            {titleBounds.position.x + titleBounds.size.x / 2.f, titleBounds.position.y + titleBounds.size.y / 2.f});
        titleText.setPosition({cx, viewSize.y * 0.08f});

        titleText.setFillColor(sf::Color(0, 200, 255, 60));
        titleText.setOutlineThickness(2.5f * scale);
        titleText.setOutlineColor(sf::Color(0, 180, 255, 70));
        mGame->mWindow.draw(titleText);

        titleText.setFillColor(sf::Color::White);
        titleText.setOutlineThickness(0.f);
        mGame->mWindow.draw(titleText);


        sf::Font& font = mGame->mAssets.fontMap.at("Consolas");

        float p1Y = cy * 0.24f;
        float p1H = cy * 0.15f;
        drawPanel(mGame->mWindow,
                  {{cx - 400.f * scale, p1Y}, {800.f * scale, p1H}},
                  sceneInstruction->mShowVietnamese ? "DIEU KIEN THANG / THUA" : "WIN / LOSS CONDITIONS",
                  font,
                  scale);

        float p1TextX = cx - 370.f * scale;
        if (sceneInstruction->mShowVietnamese)
        {
            drawLabel(mGame->mWindow,
                      mLabels[1],
                      "- Thang: Danh bai tat ca ke dich o level cao nhat",
                      {p1TextX, p1Y + 50.f * scale},
                      static_cast<unsigned int>(17.f * scale),
                      sf::Color(100, 255, 150));
            drawLabel(mGame->mWindow,
                      mLabels[2],
                      "- Thua: Tat ca dong minh bi tieu diet",
                      {p1TextX, p1Y + 82.f * scale},
                      static_cast<unsigned int>(17.f * scale),
                      sf::Color(255, 120, 120));
        }
        else
        {
            drawLabel(mGame->mWindow,
                      mLabels[1],
                      "- Win: Survive all waves or destroy opposing bases",
                      {p1TextX, p1Y + 50.f * scale},
                      static_cast<unsigned int>(17.f * scale),
                      sf::Color(100, 255, 150));
            drawLabel(mGame->mWindow,
                      mLabels[2],
                      "- Lose: All friendly bases are destroyed",
                      {p1TextX, p1Y + 82.f * scale},
                      static_cast<unsigned int>(17.f * scale),
                      sf::Color(255, 120, 120));
        }

        float p2Y = cy * 0.41f;
        float p2H = cy * 0.22f;
        drawPanel(mGame->mWindow,
                  {{cx - 400.f * scale, p2Y}, {800.f * scale, p2H}},
                  sceneInstruction->mShowVietnamese ? "CHUC NANG CAC BUC TUONG" : "WALL FUNCTIONS",
                  font,
                  scale);

        float col1X    = cx - 370.f * scale;
        float col2X    = cx + 50.f * scale;
        float wallRowH = 36.f * scale;

        auto drawWallKey = [&](sf::Color wallColor, const std::string& desc, sf::Vector2f pos, LabelData& lbl)
        {
            sf::RectangleShape keyRect({16.f * scale, 16.f * scale});
            keyRect.setOrigin({0.f, 8.f * scale});
            keyRect.setPosition(pos);
            keyRect.setFillColor(wallColor);
            keyRect.setOutlineColor(sf::Color::White);
            keyRect.setOutlineThickness(1.f * scale);
            mGame->mWindow.draw(keyRect);

            drawLabel(mGame->mWindow,
                      lbl,
                      desc,
                      {pos.x + 28.f * scale, pos.y},
                      static_cast<unsigned int>(16.f * scale),
                      sf::Color(220, 230, 255));
        };

        if (sceneInstruction->mShowVietnamese)
        {
            drawWallKey(sf::Color(255, 140, 30),
                        "Duoc phong thu: Soi sang & chan dich",
                        {col1X, p2Y + 55.f * scale},
                        mLabels[4]);
            drawWallKey(sf::Color(0, 150, 255),
                        "Tuong dien (lam): Tia dien giat",
                        {col1X, p2Y + 55.f * scale + wallRowH},
                        mLabels[5]);
            drawWallKey(sf::Color(255, 60, 60),
                        "Tuong lua (do): Phu lua dot",
                        {col1X, p2Y + 55.f * scale + wallRowH * 2},
                        mLabels[6]);

            drawWallKey(sf::Color(140, 140, 140), "Tuong sung (xam): Ban sung may", {col2X, p2Y + 55.f * scale}, mLabels[7]);
            drawWallKey(sf::Color(40, 240, 120),
                        "Tuong hoi mau (xanh la): Hoi mau quan ta",
                        {col2X, p2Y + 55.f * scale + wallRowH},
                        mLabels[8]);
            drawWallKey(sf::Color(240, 210, 0),
                        "Tuong chi huy (vang): Dinh huong",
                        {col2X, p2Y + 55.f * scale + wallRowH * 2},
                        mLabels[9]);
        }
        else
        {
            drawWallKey(sf::Color(255, 140, 30),
                        "Torch (Defensive): Illuminates & blocks",
                        {col1X, p2Y + 55.f * scale},
                        mLabels[4]);
            drawWallKey(sf::Color(0, 150, 255),
                        "Electric (blue): Electric bolts",
                        {col1X, p2Y + 55.f * scale + wallRowH},
                        mLabels[5]);
            drawWallKey(sf::Color(255, 60, 60),
                        "Fire wall (red): Shoots flames",
                        {col1X, p2Y + 55.f * scale + wallRowH * 2},
                        mLabels[6]);

            drawWallKey(sf::Color(140, 140, 140), "Gun wall (gray): Shoots bullets", {col2X, p2Y + 55.f * scale}, mLabels[7]);
            drawWallKey(sf::Color(40, 240, 120),
                        "Healing radar (green): Heals friendly units",
                        {col2X, p2Y + 55.f * scale + wallRowH},
                        mLabels[8]);
            drawWallKey(sf::Color(240, 210, 0),
                        "Command point (yellow): Guides units",
                        {col2X, p2Y + 55.f * scale + wallRowH * 2},
                        mLabels[9]);
        }

        float p3Y = cy * 0.65f;
        float p3H = cy * 0.17f;
        drawPanel(mGame->mWindow,
                  {{cx - 400.f * scale, p3Y}, {800.f * scale, p3H}},
                  sceneInstruction->mShowVietnamese ? "CHE DO TACTICAL" : "COMBAT AI MODES",
                  font,
                  scale);

        float p3TextX = cx - 370.f * scale;
        if (sceneInstruction->mShowVietnamese)
        {
            drawLabel(mGame->mWindow,
                      mLabels[11],
                      "- Auto-Attack: Dong minh tu tim & tan cong ke dich.",
                      {p3TextX, p3Y + 45.f * scale},
                      static_cast<unsigned int>(16.f * scale),
                      sf::Color(240, 240, 255));
            drawLabel(mGame->mWindow,
                      mLabels[12],
                      "- Tactical: Dong minh di chuyen theo huong tuong chi huy.",
                      {p3TextX, p3Y + 70.f * scale},
                      static_cast<unsigned int>(16.f * scale),
                      sf::Color(240, 240, 255));
            drawLabel(mGame->mWindow,
                      mLabels[13],
                      "  Giu va di chuyen tuong vang de dieu quan ta mot cach thong minh.",
                      {p3TextX, p3Y + 95.f * scale},
                      static_cast<unsigned int>(15.f * scale),
                      sf::Color(180, 210, 255));
        }
        else
        {
            drawLabel(mGame->mWindow,
                      mLabels[11],
                      "- Auto-Attack: Units automatically locate & attack opponents.",
                      {p3TextX, p3Y + 45.f * scale},
                      static_cast<unsigned int>(16.f * scale),
                      sf::Color(240, 240, 255));
            drawLabel(mGame->mWindow,
                      mLabels[12],
                      "- Tactical: Units march toward the designated command point.",
                      {p3TextX, p3Y + 70.f * scale},
                      static_cast<unsigned int>(16.f * scale),
                      sf::Color(240, 240, 255));
            drawLabel(mGame->mWindow,
                      mLabels[13],
                      "  Drag the command point to guide your army strategically.",
                      {p3TextX, p3Y + 95.f * scale},
                      static_cast<unsigned int>(15.f * scale),
                      sf::Color(180, 210, 255));
        }
    }
}
