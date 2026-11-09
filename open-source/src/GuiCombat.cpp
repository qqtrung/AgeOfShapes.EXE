#include "GuiCombat.hpp"

#include "ClientGame.hpp"
#include "FloatFixed.hpp"
#include "Game.hpp"
#include "GameState.hpp"
#include "Profiler.hpp"
#include "TextUtil.hpp"
#include "ViewState.hpp"
#include "ViewStateDetail.hpp"

#include <iomanip>
#include <sstream>

#include <cmath>


RenderBatch::RenderBatch() : triangles(sf::PrimitiveType::Triangles), lines(sf::PrimitiveType::Lines)
{
}

void RenderBatch::clear()
{
    triangles.clear();
    lines.clear();
}

void RenderBatch::draw(sf::RenderTarget& target)
{
    if (triangles.getVertexCount() > 0)
    {
        target.draw(triangles);
    }
    if (lines.getVertexCount() > 0)
    {
        target.draw(lines);
    }
}

void RenderBatch::addRect(sf::Vector2f pos, sf::Vector2f size, sf::Color color)
{
    if (color.a == 0)
        return;
    sf::Vertex v0(pos, color);
    sf::Vertex v1({pos.x + size.x, pos.y}, color);
    sf::Vertex v2({pos.x, pos.y + size.y}, color);
    sf::Vertex v3({pos.x + size.x, pos.y + size.y}, color);

    triangles.append(v0);
    triangles.append(v1);
    triangles.append(v2);

    triangles.append(v2);
    triangles.append(v1);
    triangles.append(v3);
}

void RenderBatch::addRectWithOutline(sf::Vector2f pos, sf::Vector2f size, sf::Color fillColor, sf::Color outlineColor, float thickness)
{
    if (fillColor.a > 0)
    {
        addRect(pos, size, fillColor);
    }
    if (thickness > 0.f && outlineColor.a > 0)
    {
        addRect({pos.x - thickness, pos.y - thickness}, {size.x + 2.f * thickness, thickness}, outlineColor);
        addRect({pos.x - thickness, pos.y + size.y}, {size.x + 2.f * thickness, thickness}, outlineColor);
        addRect({pos.x - thickness, pos.y}, {thickness, size.y}, outlineColor);
        addRect({pos.x + size.x, pos.y}, {thickness, size.y}, outlineColor);
    }
}

void RenderBatch::addLine(sf::Vector2f p1, sf::Vector2f p2, sf::Color color)
{
    if (color.a == 0)
        return;
    lines.append(sf::Vertex(p1, color));
    lines.append(sf::Vertex(p2, color));
}

void RenderBatch::addTechBrackets(sf::Vector2f pos, sf::Vector2f size, float scale, float bracketLen, sf::Color color)
{
    float x     = pos.x;
    float y     = pos.y;
    float w     = size.x;
    float h     = size.y;
    float len   = bracketLen * scale;
    float thick = 1.5f * scale;

    addRect({x, y}, {len, thick}, color);
    addRect({x, y}, {thick, len}, color);

    addRect({x + w - len, y}, {len, thick}, color);
    addRect({x + w - thick, y}, {thick, len}, color);

    addRect({x, y + h - thick}, {len, thick}, color);
    addRect({x, y + h - len}, {thick, len}, color);

    addRect({x + w - len, y + h - thick}, {len, thick}, color);
    addRect({x + w - thick, y + h - len}, {thick, len}, color);
}


GUICombat::Button::Button(ButtonType type, sf::Font& font) :
    mType(type),
    mOriginalSize(150.f, 60.f),
    mText(font, "", TextUtil::kBaseCharSize)
{
    mText.setFillColor(sf::Color::White);

    const char* label = "";
    switch (mType)
    {
        case ButtonType::Back:
            label = "Back";
            break;
        case ButtonType::Exit:
            label = "Exit";
            break;
        case ButtonType::CombatMode:
            label = "Auto-Attack";
            break;
        case ButtonType::DesignatedMode:
            label = "Tactical";
            break;
        case ButtonType::ToggleInputMode:
            label = "Select";
            break;
        case ButtonType::TogglePlacementPalette:
            label = "Unit";
            break;
    }
    mText.setString(label);
    TextUtil::setSize(mText, 16.f * 1.55f);
    auto bounds = mText.getLocalBounds();
    mText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
}

GUICombat::Button::~Button()
{
    logout("~Button()");
}

void GUICombat::Button::update()
{
    PROFILE_FUNCTION();
    GUICombat* guiCombat = nullptr;
    for (Space* p = parent; p != nullptr; p = p->parent)
    {
        if ((guiCombat = dynamic_cast<GUICombat*>(p)))
            break;
    }
    if (!guiCombat)
        return;

    auto* client = dynamic_cast<ClientGame*>(guiCombat->parent);
    if (!client)
        return;

    if (mType == ButtonType::Back)
    {
        for (Space* child = firstChild; child != nullptr; child = child->next)
        {
            child->visible = guiCombat->mShowExitConfirm;
        }
    }
    else if (mType == ButtonType::ToggleInputMode)
    {
        bool isSelectMode = (client->mViewState->input().inputMode == InputMode::Select);
        mText.setString(isSelectMode ? "Drag" : "Select");
        for (Space* child = firstChild; child != nullptr; child = child->next)
        {
            child->visible = isSelectMode;
        }
    }
    else if (mType == ButtonType::TogglePlacementPalette)
    {
        mText.setString(client->mViewState->input().build.palette == PlacementPalette::Wall ? "Unit" : "Wall");
    }
}

bool GUICombat::Button::input(const sf::Event& event)
{
    GUICombat* guiCombat = nullptr;
    for (Space* p = parent; p != nullptr; p = p->parent)
    {
        if ((guiCombat = dynamic_cast<GUICombat*>(p)))
            break;
    }
    if (!guiCombat)
    {
        return false;
    }

    auto* client = dynamic_cast<ClientGame*>(guiCombat->parent);
    if (!client || client->isGameOver())
    {
        return false;
    }

    if (mType == ButtonType::Exit && !guiCombat->mShowExitConfirm)
    {
        return false;
    }

    if ((mType == ButtonType::CombatMode || mType == ButtonType::DesignatedMode) &&
        client->mViewState->input().inputMode != InputMode::Select)
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

    auto& window = client->window();

    sf::Vector2f uiPos = window.mapPixelToCoords({touchPos.x, touchPos.y}, client->mViewState->uiView());

    float width = 2532.f;
    float scale = 1.55f;

    const float topY    = 20.f * scale;
    const float spacing = 15.f * scale;
    const float btnW    = 150.f * scale;
    const float btnH    = 60.f * scale;

    sf::Vector2f btnPos;
    sf::Vector2f btnSize(btnW, btnH);

    if (mType == ButtonType::Back)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY};
    }
    else if (mType == ButtonType::Exit)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing), topY};
    }
    else if (mType == ButtonType::ToggleInputMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY + btnH + spacing};
    }
    else if (mType == ButtonType::TogglePlacementPalette)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY + (btnH + spacing) * 2.f};
    }
    else if (mType == ButtonType::CombatMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing) * 2.f, topY + btnH + spacing};
    }
    else if (mType == ButtonType::DesignatedMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing), topY + btnH + spacing};
    }

    sf::FloatRect btnRect(btnPos, btnSize);
    if (!btnRect.contains(uiPos))
    {
        return false;
    }

    if (mType == ButtonType::Back)
    {
        guiCombat->mShowExitConfirm = !guiCombat->mShowExitConfirm;
        for (Space* child = firstChild; child != nullptr; child = child->next)
        {
            child->visible = guiCombat->mShowExitConfirm;
        }
        return true;
    }
    if (mType == ButtonType::Exit)
    {
        guiCombat->mShowExitConfirm = false;
        Action action(Action::Type::ExitToMenu);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::ToggleInputMode)
    {
        Action action(Action::Type::ToggleInputMode);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::TogglePlacementPalette)
    {
        Action action(Action::Type::TogglePlacementPalette);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::CombatMode)
    {
        Action action(Action::Type::CombatMode);
        pushAction(action);
        return true;
    }
    if (mType == ButtonType::DesignatedMode)
    {
        Action action(Action::Type::DesignatedMode);
        pushAction(action);
        return true;
    }

    return false;
}

void GUICombat::Button::render()
{
    PROFILE_FUNCTION();
    GUICombat* guiCombat = nullptr;
    for (Space* p = parent; p != nullptr; p = p->parent)
    {
        if ((guiCombat = dynamic_cast<GUICombat*>(p)))
            break;
    }
    if (!guiCombat)
    {
        return;
    }

    auto* client = dynamic_cast<ClientGame*>(guiCombat->parent);
    if (!client)
    {
        return;
    }

    if (mType == ButtonType::Exit && !guiCombat->mShowExitConfirm)
    {
        return;
    }

    if ((mType == ButtonType::CombatMode || mType == ButtonType::DesignatedMode) &&
        client->mViewState->input().inputMode != InputMode::Select)
    {
        return;
    }

    auto& window = client->window();
    window.setView(client->mViewState->uiView());

    float width = 2532.f;
    float scale = 1.55f;

    const float topY    = 20.f * scale;
    const float spacing = 15.f * scale;
    const float btnW    = 150.f * scale;
    const float btnH    = 60.f * scale;

    sf::Vector2f btnPos;
    sf::Vector2f btnSize(btnW, btnH);

    if (mType == ButtonType::Back)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY};
    }
    else if (mType == ButtonType::Exit)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing), topY};
    }
    else if (mType == ButtonType::ToggleInputMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY + btnH + spacing};
    }
    else if (mType == ButtonType::TogglePlacementPalette)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX, topY + (btnH + spacing) * 2.f};
    }
    else if (mType == ButtonType::CombatMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing) * 2.f, topY + btnH + spacing};
    }
    else if (mType == ButtonType::DesignatedMode)
    {
        float rightX = width - btnW - 20.f * scale;
        btnPos       = {rightX - (btnW + spacing), topY + btnH + spacing};
    }

    sf::Color fillColor = sf::Color(26, 32, 44, 160);

    if (mType == ButtonType::Back && guiCombat->mShowExitConfirm)
    {
        fillColor = sf::Color(220, 100, 50, 210);
    }
    else if (mType == ButtonType::Exit)
    {
        fillColor = sf::Color(210, 45, 45, 220);
    }
    else if (mType == ButtonType::ToggleInputMode && client->mViewState->input().inputMode == InputMode::Select)
    {
        fillColor = sf::Color(0, 205, 120, 210);
    }
    else if (mType == ButtonType::TogglePlacementPalette &&
             client->mViewState->input().build.palette == PlacementPalette::Unit)
    {
        fillColor = sf::Color(0, 205, 120, 210);
    }
    else if (mType == ButtonType::CombatMode && (guiCombat->mCombatMode == GUICombat::CombatMode::Attack ||
                                                 client->mViewState->input().combatModeState == CombatStance::Attack))
    {
        fillColor = sf::Color(0, 205, 120, 210);
    }
    else if (mType == ButtonType::DesignatedMode &&
             (guiCombat->mCombatMode == GUICombat::CombatMode::Tactical ||
              client->mViewState->input().combatModeState == CombatStance::Tactical))
    {
        fillColor = sf::Color(0, 205, 120, 210);
    }

    sf::Vector2f mpos    = window.mapPixelToCoords(sf::Mouse::getPosition(window), client->mViewState->uiView());
    bool         hovered = sf::FloatRect(btnPos, btnSize).contains(mpos);

    const bool stateChanged = mNeedsRebuild || (hovered != mPrevHovered) || (fillColor != mPrevFillColor) ||
                              (btnPos != mPrevBtnPos);

    if (stateChanged)
    {
        mCachedBatch.clear();
        drawShapes(mCachedBatch, btnPos, fillColor, scale, hovered);
        mPrevHovered   = hovered;
        mPrevFillColor = fillColor;
        mPrevBtnPos    = btnPos;
        mNeedsRebuild  = false;
    }

    mCachedBatch.draw(window);

    drawText(window, btnPos, scale, hovered);
}

void GUICombat::Button::drawShapes(RenderBatch& batch, sf::Vector2f pos, sf::Color fillColor, float scale, bool hovered)
{
    sf::Vector2f baseSize   = mOriginalSize * scale;
    sf::Vector2f renderSize = baseSize;
    sf::Vector2f centerPos  = pos + baseSize * 0.5f;

    if (hovered)
    {
        renderSize = baseSize * 1.05f;
    }

    sf::Vector2f rectPos = centerPos - renderSize * 0.5f;

    sf::Color outlineColor     = sf::Color(100, 115, 140, 80);
    float     outlineThickness = 1.5f * scale;
    sf::Color currentFillColor = fillColor;

    if (hovered)
    {
        currentFillColor.r = std::min(255, currentFillColor.r + 20);
        currentFillColor.g = std::min(255, currentFillColor.g + 20);
        currentFillColor.b = std::min(255, currentFillColor.b + 20);
        outlineColor       = sf::Color(0, 205, 255, 230);
        outlineThickness   = 2.f * scale;
    }

    batch.addRectWithOutline(rectPos, renderSize, currentFillColor, outlineColor, outlineThickness);
}

void GUICombat::Button::drawText(sf::RenderTarget& target, sf::Vector2f pos, float scale, bool hovered)
{
    (void)hovered;
    sf::Vector2f baseSize  = mOriginalSize * scale;
    sf::Vector2f centerPos = pos + baseSize * 0.5f;

    mText.setPosition(centerPos);
    target.draw(mText);
}


void GUICombat::drawTechBrackets(sf::RenderTarget& target,
                                 sf::Vector2f      pos,
                                 sf::Vector2f      size,
                                 float             scale,
                                 float             bracketLen,
                                 sf::Color         color)
{
    float x     = pos.x;
    float y     = pos.y;
    float w     = size.x;
    float h     = size.y;
    float len   = bracketLen * scale;
    float thick = 1.5f * scale;

    sf::RectangleShape h1({len, thick});
    h1.setPosition({x, y});
    h1.setFillColor(color);
    target.draw(h1);
    sf::RectangleShape v1({thick, len});
    v1.setPosition({x, y});
    v1.setFillColor(color);
    target.draw(v1);

    sf::RectangleShape h2({len, thick});
    h2.setPosition({x + w - len, y});
    h2.setFillColor(color);
    target.draw(h2);
    sf::RectangleShape v2({thick, len});
    v2.setPosition({x + w - thick, y});
    v2.setFillColor(color);
    target.draw(v2);

    sf::RectangleShape h3({len, thick});
    h3.setPosition({x, y + h - thick});
    h3.setFillColor(color);
    target.draw(h3);
    sf::RectangleShape v3({thick, len});
    v3.setPosition({x, y + h - len});
    v3.setFillColor(color);
    target.draw(v3);

    sf::RectangleShape h4({len, thick});
    h4.setPosition({x + w - len, y + h - thick});
    h4.setFillColor(color);
    target.draw(h4);
    sf::RectangleShape v4({thick, len});
    v4.setPosition({x + w - thick, y + h - len});
    v4.setFillColor(color);
    target.draw(v4);
}

GUICombat::GUICombat(sf::Font& font) : mHudText(font, "", TextUtil::kBaseCharSize)
{
    mHudText.setFillColor(sf::Color::White);

    auto* backBtn    = addChild<Button>(Button::ButtonType::Back, font);
    auto* exitBtn    = backBtn->addChild<Button>(Button::ButtonType::Exit, font);
    exitBtn->visible = false;

    auto* inputModeBtn = addChild<Button>(Button::ButtonType::ToggleInputMode, font);
    auto* combatBtn    = inputModeBtn->addChild<Button>(Button::ButtonType::CombatMode, font);
    auto* tacticalBtn  = inputModeBtn->addChild<Button>(Button::ButtonType::DesignatedMode, font);

    combatBtn->visible   = false;
    tacticalBtn->visible = false;

    addChild<Button>(Button::ButtonType::TogglePlacementPalette, font);
}

GUICombat::~GUICombat()
{
    logout("~GUICombat()");
}

bool GUICombat::input(const sf::Event& event)
{
    auto* client = dynamic_cast<ClientGame*>(parent);

    if (!client || client->isGameOver())
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

    auto& window = client->window();

    sf::Vector2f uiPos = window.mapPixelToCoords({touchPos.x, touchPos.y}, client->mViewState->uiView());

    float scale = 1.55f;


    if (mShowUnitInfoPanel)
    {
        const float panelX = 16.f * scale;
        const float panelY = 180.f * scale;
        const float panelW = 600.f * scale;
        const float panelH = 450.f * scale;

        sf::FloatRect panelRect({panelX, panelY}, {panelW, panelH});

        const float unitBtnW       = 85.f * scale;
        const float unitBtnH       = 40.f * scale;
        const float unitBtnSpacing = 10.f * scale;
        const float unitBtnStartX  = panelX + 20.f * scale;
        const float unitBtnStartY  = panelY + 20.f * scale;

        std::array<UnitClass, 6> unitClasses =
            {UnitClass::Warrior, UnitClass::Archer, UnitClass::Tank, UnitClass::Grunt, UnitClass::Shooter, UnitClass::Berserker};

        for (size_t i = 0; i < unitClasses.size(); i++)
        {
            float btnX = unitBtnStartX + (unitBtnW + unitBtnSpacing) * (i % 3);
            float btnY = unitBtnStartY + (unitBtnH + unitBtnSpacing) * (i / 3);

            sf::FloatRect unitBtnRect({btnX, btnY}, {unitBtnW, unitBtnH});

            if (unitBtnRect.contains(uiPos))
            {
                mSelectedUnitClass = unitClasses[i];
                return true;
            }
        }
    }

    return false;
}

void GUICombat::update()
{
    PROFILE_FUNCTION();
    auto* client = dynamic_cast<ClientGame*>(parent);

    if (!client)
    {
        return;
    }

    if (mSelectedAgentId != -1)
    {
        const auto& pool = client->gameState().mAdmin.mPool;
        const auto* mask = pool.getMask(mSelectedAgentId);

        if (!pool.has(mSelectedAgentId) || !pool.hasComponent<CPosition>(mask) || !pool.hasComponent<CHealth>(mask))
        {
            mSelectedAgentId = -1;
        }
    }
}

void GUICombat::render()
{
    PROFILE_FUNCTION();
    auto* client = dynamic_cast<ClientGame*>(parent);

    if (!client)
    {
        return;
    }

    auto& window = client->window();

    float height = 1170.f;
    float scale  = 1.55f;
    (void)height;

    window.setView(client->mViewState->uiView());


    const auto& mCampaign = client->gameState().campaign();
    auto&       mDebug    = client->mViewState->debug();


    mBatch.clear();

    float totalWaveTicks = std::max(1, mCampaign.waveTotalTimerTicks);
    float waveRatio      = std::clamp(1.f - static_cast<float>(mCampaign.waveTimerTicks) / totalWaveTicks, 0.f, 1.f);
    float waveBarX       = 28.f * scale;
    float waveBarY       = 120.f * scale;
    float waveBarW       = 300.f * scale;
    float waveBarH       = 6.f * scale;

    if (mShowHud)
    {
        mBatch.addRectWithOutline({16.f * scale, 10.f * scale},
                                  {360.f * scale, 160.f * scale},
                                  sf::Color(14, 18, 30, 200),
                                  sf::Color(100, 115, 140, 75),
                                  1.5f * scale);

        sf::Color hudGridCol(0, 205, 255, 15);
        for (float gx = 36.f * scale; gx < 360.f * scale; gx += 40.f * scale)
        {
            mBatch.addLine({16.f * scale + gx, 10.f * scale}, {16.f * scale + gx, 170.f * scale}, hudGridCol);
        }
        for (float gy = 20.f * scale; gy < 160.f * scale; gy += 20.f * scale)
        {
            mBatch.addLine({16.f * scale, 10.f * scale + gy}, {376.f * scale, 10.f * scale + gy}, hudGridCol);
        }

        mBatch.addTechBrackets({16.f * scale, 10.f * scale},
                               {360.f * scale, 160.f * scale},
                               scale,
                               10.f,
                               sf::Color(0, 205, 255, 180));

        mBatch.addRect({16.f * scale, 10.f * scale}, {360.f * scale, 3.f * scale}, sf::Color(0, 205, 255));

        mBatch.addRectWithOutline({waveBarX, waveBarY},
                                  {waveBarW, waveBarH},
                                  sf::Color(20, 35, 55, 180),
                                  sf::Color(40, 90, 140, 120),
                                  0.5f * scale);
        if (waveRatio > 0.f)
        {
            const bool      isPvP      = (client->gameState().getGameMode() == GameState::GameMode::PvP);
            const sf::Color barFillCol = (!isPvP && mCampaign.currentWave == mCampaign.maxWaves)
                                             ? sf::Color(255, 75, 75, 230)
                                             : sf::Color(0, 220, 255, 220);
            mBatch.addRect({waveBarX, waveBarY}, {waveBarW * waveRatio, waveBarH}, barFillCol);
        }
    }


    sf::Vector2f mpos = window.mapPixelToCoords(sf::Mouse::getPosition(window), client->mViewState->uiView());

    float dPanelX = 16.f * scale;
    float dPanelY = 10.f * scale;

    static sf::Text profilerText(mHudText.getFont(), "", TextUtil::kBaseCharSize);
    static int      cachePanelW = 0;
    static int      cachePanelH = 0;

    if (cachePanelW == 0 && mHudText.getFont().hasGlyph('A'))
    {
        TextUtil::setSize(profilerText, 14.f * scale);
        profilerText.setString("PERFORMANCE PROFILER\nUpdate: 00ms | Render: 00ms\n");
        auto bounds = TextUtil::bounds(profilerText);
        cachePanelW = static_cast<int>(bounds.size.x);
        cachePanelH = static_cast<int>(bounds.size.y);
    }

    if (mDebug.showPanel)
    {
        float panelW = cachePanelW + 40.f * scale;
        float panelH = cachePanelH + 35.f * scale;

        mBatch.addRectWithOutline({dPanelX - 10.f * scale, dPanelY - 5.f * scale},
                                  {panelW, panelH},
                                  sf::Color(14, 18, 30, 220),
                                  sf::Color(0, 205, 255, 120),
                                  2.f * scale);

        mBatch.addRect({dPanelX - 8.f * scale, dPanelY - 3.f * scale},
                       {panelW - 4.f * scale, 3.f * scale},
                       sf::Color(0, 205, 255));

        mBatch.addTechBrackets({dPanelX - 10.f * scale, dPanelY - 5.f * scale},
                               {panelW, panelH},
                               scale,
                               12.f,
                               sf::Color(0, 205, 255, 180));
    }

    const float uiPanelX = 16.f * scale;
    const float uiPanelY = 180.f * scale;
    const float uiPanelW = 600.f * scale;
    const float uiPanelH = 450.f * scale;

    const float unitBtnW       = 85.f * scale;
    const float unitBtnH       = 40.f * scale;
    const float unitBtnSpacing = 10.f * scale;
    const float unitBtnStartX  = uiPanelX + 20.f * scale;
    const float unitBtnStartY  = uiPanelY + 20.f * scale;

    std::array<UnitClass, 6> unitClasses =
        {UnitClass::Warrior, UnitClass::Archer, UnitClass::Tank, UnitClass::Grunt, UnitClass::Shooter, UnitClass::Berserker};
    std::array<const char*, 6> unitLabels = {"Warrior", "Archer", "Tank", "Grunt", "Shooter", "Berserker"};

    const auto& selCfg = mCampaign.getUnitConfig(mSelectedUnitClass);

    int         maxHp       = selCfg.maxHp;
    int         armor       = selCfg.armor;
    float       attackRange = 0;
    float       damage      = 0;
    float       moveSpeed   = 0;
    const char* playStyle   = "";

    switch (mSelectedUnitClass)
    {
        case UnitClass::Warrior:
            attackRange = 26.f;
            damage      = 2.f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Balanced melee fighter. Fast attack speed and decent damage. Standard vanguard.";
            break;
        case UnitClass::Archer:
            attackRange = 155.f;
            damage      = 1.5f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Ranged marksman. Fragile frame but deals consistent damage from afar. Guard them.";
            break;
        case UnitClass::Tank:
            attackRange = 22.f;
            damage      = 2.f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Fortified ironclad unit. Excellent armor. Heavy hitpoints, built to absorb hits.";
            break;
        case UnitClass::Grunt:
            attackRange = 26.f;
            damage      = 2.f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Standard melee combatant. Balanced stats.";
            break;
        case UnitClass::Shooter:
            attackRange = 155.f;
            damage      = 1.f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Ranged combatant. Low HP, attacks from distance.";
            break;
        case UnitClass::Berserker:
            attackRange = 26.f;
            damage      = 3.f + selCfg.damageBonus;
            moveSpeed   = 0.65f;
            playStyle   = "Fast high-damage melee. High speed, aggressive.";
            break;
    }

    auto drawStatProgressShapes = [&](float value, float maxValue, sf::Vector2f barPos, sf::Color barColor)
    {
        float        trackWidth  = 140.f * scale;
        float        trackHeight = 8.f * scale;
        sf::Vector2f trackPos    = {barPos.x + 110.f * scale, barPos.y + 5.f * scale};
        mBatch.addRectWithOutline(trackPos,
                                  {trackWidth, trackHeight},
                                  sf::Color(30, 36, 46, 180),
                                  sf::Color(80, 95, 110, 80),
                                  0.5f * scale);

        float ratio = std::clamp(value / maxValue, 0.f, 1.f);
        if (ratio > 0.f)
        {
            mBatch.addRect(trackPos, {trackWidth * ratio, trackHeight}, barColor);
        }
    };

    float barsY      = uiPanelY + 130.f * scale;
    float rowSpacing = 35.f * scale;
    float tactY      = barsY + rowSpacing * 4 + 40.f * scale;

    if (mShowUnitInfoPanel)
    {
        mBatch.addRectWithOutline({uiPanelX, uiPanelY},
                                  {uiPanelW, uiPanelH},
                                  sf::Color(14, 18, 30, 220),
                                  sf::Color(0, 205, 255, 120),
                                  2.f * scale);

        sf::Color subGridCol(0, 205, 255, 12);
        for (float gx = 30.f * scale; gx < uiPanelW; gx += 40.f * scale)
        {
            mBatch.addLine({uiPanelX + gx, uiPanelY}, {uiPanelX + gx, uiPanelY + uiPanelH}, subGridCol);
        }
        for (float gy = 30.f * scale; gy < uiPanelH; gy += 30.f * scale)
        {
            mBatch.addLine({uiPanelX, uiPanelY + gy}, {uiPanelX + uiPanelW, uiPanelY + gy}, subGridCol);
        }

        float time  = client->mGame->mClockGui.getElapsedTime().asSeconds();
        float scanY = uiPanelY + std::fmod(time * 90.f, uiPanelH);
        mBatch.addRect({uiPanelX, scanY}, {uiPanelW, 2.f * scale}, sf::Color(0, 205, 255, 40));

        mBatch.addTechBrackets({uiPanelX, uiPanelY}, {uiPanelW, uiPanelH}, scale, 15.f, sf::Color(0, 205, 255, 180));

        mBatch.addRect({uiPanelX, uiPanelY}, {uiPanelW, 4.f * scale}, sf::Color(0, 205, 255));

        for (size_t i = 0; i < unitClasses.size(); i++)
        {
            float btnX = unitBtnStartX + (unitBtnW + unitBtnSpacing) * (i % 3);
            float btnY = unitBtnStartY + (unitBtnH + unitBtnSpacing) * (i / 3);

            sf::FloatRect rect({btnX, btnY}, {unitBtnW, unitBtnH});
            bool          isHoveredBtn  = rect.contains(mpos);
            bool          isSelectedBtn = (mSelectedUnitClass == unitClasses[i]);

            sf::Color btnFillColor;
            sf::Color btnOutlineColor;
            float     btnOutlineThickness = NAN;
            if (isSelectedBtn)
            {
                btnFillColor        = sf::Color(0, 205, 120, 180);
                btnOutlineColor     = sf::Color(200, 255, 220, 220);
                btnOutlineThickness = 1.5f * scale;
            }
            else if (isHoveredBtn)
            {
                btnFillColor        = sf::Color(55, 75, 105, 190);
                btnOutlineColor     = sf::Color(100, 220, 255, 230);
                btnOutlineThickness = 1.5f * scale;
            }
            else
            {
                btnFillColor        = sf::Color(30, 35, 45, 140);
                btnOutlineColor     = sf::Color(100, 115, 140, 80);
                btnOutlineThickness = 1.f * scale;
            }

            mBatch.addRectWithOutline({btnX, btnY}, {unitBtnW, unitBtnH}, btnFillColor, btnOutlineColor, btnOutlineThickness);
        }

        drawStatProgressShapes(static_cast<float>(maxHp), 60.f, {uiPanelX + 20.f * scale, barsY}, sf::Color(0, 220, 120));
        drawStatProgressShapes(static_cast<float>(armor),
                               10.f,
                               {uiPanelX + 20.f * scale, barsY + rowSpacing},
                               sf::Color(140, 150, 175));
        drawStatProgressShapes(attackRange, 200.f, {uiPanelX + 20.f * scale, barsY + rowSpacing * 2}, sf::Color(0, 180, 255));
        drawStatProgressShapes(damage, 10.f, {uiPanelX + 20.f * scale, barsY + rowSpacing * 3}, sf::Color(255, 90, 90));
        drawStatProgressShapes(moveSpeed * 100.f,
                               150.f,
                               {uiPanelX + 20.f * scale, barsY + rowSpacing * 4},
                               sf::Color(240, 210, 0));

        mBatch.addRectWithOutline({uiPanelX + 20.f * scale, tactY},
                                  {560.f * scale, 80.f * scale},
                                  sf::Color(20, 24, 38, 160),
                                  sf::Color(100, 120, 140, 60),
                                  1.f * scale);

        mBatch.addRect({uiPanelX + 20.f * scale, tactY}, {560.f * scale, 3.f * scale}, sf::Color(0, 205, 255, 150));

        {
            const sf::Vector2f holoBoxPos{uiPanelX + 340.f * scale, uiPanelY + 20.f * scale};
            const sf::Vector2f holoBoxSize{240.f * scale, 265.f * scale};
            mBatch.addRectWithOutline(holoBoxPos, holoBoxSize, sf::Color(16, 22, 36, 185), sf::Color(0, 205, 255, 95), 1.2f * scale);
            mBatch.addTechBrackets(holoBoxPos, holoBoxSize, scale, 10.f, sf::Color(0, 215, 255, 190));

            const sf::Vector2f holoCenter = holoBoxPos + sf::Vector2f(holoBoxSize.x * 0.5f, holoBoxSize.y * 0.72f);
            ViewStateDetail::Camera3D holoCam;
            holoCam.center      = holoCenter;
            holoCam.size        = holoBoxSize;
            holoCam.cosPitch    = 0.65f;
            holoCam.sinPitch    = 0.45f;
            holoCam.heightScale = 1.25f;
            holoCam.heightPersp = 0.40f;
            holoCam.focalDist   = 650.f * scale;

            const float     rotAngle = time * 55.f;
            const sf::Color uCol     = ViewStateDetail::getUnitClassColor(mSelectedUnitClass);

            auto appendHoloQuad =
                [&](sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Color cB, sf::Color cT, bool cull = true)
            {
                if (cull)
                {
                    const float cross = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
                    if (cross >= 0.f)
                        return;
                }
                mBatch.triangles.append(sf::Vertex(p0, cB));
                mBatch.triangles.append(sf::Vertex(p1, cB));
                mBatch.triangles.append(sf::Vertex(p2, cT));
                mBatch.triangles.append(sf::Vertex(p2, cT));
                mBatch.triangles.append(sf::Vertex(p3, cT));
                mBatch.triangles.append(sf::Vertex(p0, cB));
            };

            auto addHoloPrism =
                [&](sf::Vector2f cXY, float rBot, float rTop, float z0, float z1, int sides, sf::Color col, float rDeg)
            {
                const float rotRad = rDeg * 3.14159265f / 180.f;
                const float step   = 2.f * 3.14159265f / static_cast<float>(sides);
                for (int i = 0; i < sides; ++i)
                {
                    const float        a0   = rotRad + i * step;
                    const float        a1   = rotRad + (i + 1) * step;
                    const sf::Vector2f d0   = {std::cos(a0), std::sin(a0)};
                    const sf::Vector2f d1   = {std::cos(a1), std::sin(a1)};
                    const sf::Vector2f b0   = holoCam.project(cXY + d0 * rBot, z0);
                    const sf::Vector2f b1   = holoCam.project(cXY + d1 * rBot, z0);
                    const sf::Vector2f t0   = holoCam.project(cXY + d0 * rTop, z1);
                    const sf::Vector2f t1   = holoCam.project(cXY + d1 * rTop, z1);
                    const sf::Vector2f n    = (d0 + d1) * 0.5f;
                    const sf::Color    fCol = ViewStateDetail::computeLitColor(col, n.x, n.y, 0.15f);
                    appendHoloQuad(b1, b0, t0, t1, ViewStateDetail::shadeColor(fCol, 0.72f), fCol, true);
                }
                const sf::Vector2f topC = holoCam.project(cXY, z1);
                const sf::Color    cCap = ViewStateDetail::shadeColor(col, 1.16f, 0.08f);
                const sf::Color    cEdg = ViewStateDetail::shadeColor(col, 0.96f, 0.02f);
                for (int i = 0; i < sides; ++i)
                {
                    const float        a0 = rotRad + i * step;
                    const float        a1 = rotRad + (i + 1) * step;
                    const sf::Vector2f t0 = holoCam.project(cXY + sf::Vector2f(std::cos(a0), std::sin(a0)) * rTop, z1);
                    const sf::Vector2f t1 = holoCam.project(cXY + sf::Vector2f(std::cos(a1), std::sin(a1)) * rTop, z1);
                    mBatch.triangles.append(sf::Vertex(topC, cCap));
                    mBatch.triangles.append(sf::Vertex(t0, cEdg));
                    mBatch.triangles.append(sf::Vertex(t1, cEdg));
                }
            };

            addHoloPrism(holoCenter, 68.f * scale, 62.f * scale, 0.f, 10.f * scale, 6, sf::Color(25, 85, 130), rotAngle * 0.5f);
            addHoloPrism(holoCenter,
                         52.f * scale,
                         48.f * scale,
                         10.f * scale,
                         14.f * scale,
                         8,
                         sf::Color(0, 195, 255, 180),
                         -rotAngle * 0.8f);

            const float baseZ = 14.f * scale;
            if (mSelectedUnitClass == UnitClass::Tank)
            {
                addHoloPrism(holoCenter,
                             42.f * scale,
                             38.f * scale,
                             baseZ,
                             baseZ + 28.f * scale,
                             4,
                             sf::Color(55, 65, 78),
                             rotAngle + 45.f);
                addHoloPrism(holoCenter, 36.f * scale, 32.f * scale, baseZ + 12.f * scale, baseZ + 44.f * scale, 6, uCol, rotAngle);
                addHoloPrism(holoCenter,
                             24.f * scale,
                             19.f * scale,
                             baseZ + 44.f * scale,
                             baseZ + 68.f * scale,
                             8,
                             ViewStateDetail::shadeColor(uCol, 1.15f),
                             rotAngle);
                const float        rad  = rotAngle * 3.14159265f / 180.f;
                const sf::Vector2f bPos = holoCenter + sf::Vector2f(std::cos(rad), std::sin(rad)) * (28.f * scale);
                addHoloPrism(bPos,
                             7.f * scale,
                             6.f * scale,
                             baseZ + 50.f * scale,
                             baseZ + 60.f * scale,
                             6,
                             sf::Color(200, 215, 235),
                             rotAngle);
            }
            else if (mSelectedUnitClass == UnitClass::Archer || mSelectedUnitClass == UnitClass::Shooter)
            {
                const float hover = baseZ + 18.f * scale + std::sin(time * 4.f) * 6.f * scale;
                addHoloPrism(holoCenter,
                             18.f * scale,
                             26.f * scale,
                             hover - 10.f * scale,
                             hover,
                             6,
                             sf::Color(0, 230, 255, 180),
                             -rotAngle);
                addHoloPrism(holoCenter, 38.f * scale, 28.f * scale, hover, hover + 26.f * scale, 4, uCol, rotAngle + 45.f);
                addHoloPrism(holoCenter,
                             16.f * scale,
                             4.f * scale,
                             hover + 26.f * scale,
                             hover + 54.f * scale,
                             4,
                             sf::Color(225, 250, 255),
                             -rotAngle * 1.5f);
            }
            else if (mSelectedUnitClass == UnitClass::Berserker)
            {
                addHoloPrism(holoCenter, 32.f * scale, 26.f * scale, baseZ, baseZ + 46.f * scale, 6, uCol, rotAngle);
                addHoloPrism(holoCenter,
                             24.f * scale,
                             16.f * scale,
                             baseZ + 46.f * scale,
                             baseZ + 72.f * scale,
                             6,
                             ViewStateDetail::shadeColor(uCol, 1.15f),
                             rotAngle);
                addHoloPrism(holoCenter,
                             14.f * scale,
                             2.f * scale,
                             baseZ + 72.f * scale,
                             baseZ + 98.f * scale,
                             4,
                             sf::Color(255, 60, 60),
                             rotAngle * 2.f);
            }
            else
            {
                addHoloPrism(holoCenter, 28.f * scale, 24.f * scale, baseZ, baseZ + 48.f * scale, 8, uCol, rotAngle);
                addHoloPrism(holoCenter,
                             22.f * scale,
                             16.f * scale,
                             baseZ + 48.f * scale,
                             baseZ + 70.f * scale,
                             8,
                             ViewStateDetail::shadeColor(uCol, 1.14f),
                             rotAngle);
                addHoloPrism(holoCenter,
                             15.f * scale,
                             9.f * scale,
                             baseZ + 70.f * scale,
                             baseZ + 88.f * scale,
                             8,
                             sf::Color(230, 245, 255),
                             rotAngle);
            }
        }
    }

    mBatch.draw(window);


    auto& hudFont     = const_cast<sf::Font&>(mHudText.getFont());
    auto  hudCharSize = static_cast<unsigned int>(20.f * scale);

    if (mShowHud)
    {
        const int mapIdx = std::clamp(mCampaign.currentMap, 1, 3);
        const int curLvl = mCampaign.currentLevel;

        static sf::Text lvlText(hudFont, "", TextUtil::kBaseCharSize);
        static int      prevMapIdx = -1;
        static int      prevLvl    = -1;
        if (mapIdx != prevMapIdx || curLvl != prevLvl)
        {
            const std::string mapNames[3] = {"CO TRAN", "HOANG MAC", "THAN DIEN"};
            TextUtil::setSize(lvlText, static_cast<float>(hudCharSize));
            lvlText.setString("Map: " + mapNames[mapIdx - 1] + " | Lvl " + std::to_string(curLvl));
            lvlText.setFillColor(sf::Color(0, 225, 255));
            lvlText.setPosition({28.f * scale, 22.f * scale});
            prevMapIdx = mapIdx;
            prevLvl    = curLvl;
        }
        window.draw(lvlText);

        const bool  isPvP  = (client->gameState().getGameMode() == GameState::GameMode::PvP);
        const int   remSec = (mCampaign.waveTimerTicks + 59) / 60;
        std::string waveStatus;
        sf::Color   waveCol(255, 205, 50);
        const bool  isUrgent   = (remSec <= 3 && mCampaign.waveTimerTicks > 0);
        const bool  blinkPhase = (mCampaign.waveTimerTicks % 30 < 15);

        if (mCampaign.currentWave == 0)
        {
            if (isUrgent)
            {
                waveStatus = "! DANGER: WAVE 1 IN " + std::to_string(remSec) + "s !";
                waveCol    = blinkPhase ? sf::Color(255, 50, 50) : sf::Color(255, 220, 50);
            }
            else
            {
                waveStatus = "Prep: Wave 1 in " + std::to_string(remSec) + "s";
            }
        }
        else if (isPvP || mCampaign.currentWave < mCampaign.maxWaves)
        {
            if (isUrgent)
            {
                waveStatus = "! DANGER: WAVE " + std::to_string(mCampaign.currentWave + 1) + " IN " +
                             std::to_string(remSec) + "s !";
                waveCol    = blinkPhase ? sf::Color(255, 50, 50) : sf::Color(255, 220, 50);
            }
            else if (isPvP)
            {
                waveStatus = "Wave " + std::to_string(mCampaign.currentWave) + " (Next in " + std::to_string(remSec) +
                             "s)";
            }
            else
            {
                waveStatus = "Wave " + std::to_string(mCampaign.currentWave) + "/" +
                             std::to_string(mCampaign.maxWaves) + " (Next in " + std::to_string(remSec) + "s)";
            }
        }
        else
        {
            waveStatus = "FINAL WAVE! Survive: " + std::to_string(remSec) + "s";
            waveCol    = blinkPhase ? sf::Color(255, 60, 60) : sf::Color(255, 140, 140);
        }

        static sf::Text    waveText(hudFont, "", TextUtil::kBaseCharSize);
        static std::string prevWaveStatus;
        static sf::Color   prevWaveCol;
        if (waveStatus != prevWaveStatus || waveCol != prevWaveCol)
        {
            TextUtil::setSize(waveText, static_cast<float>(hudCharSize));
            waveText.setString(waveStatus);
            waveText.setFillColor(waveCol);
            waveText.setPosition({28.f * scale, 45.f * scale});
            prevWaveStatus = waveStatus;
            prevWaveCol    = waveCol;
        }
        window.draw(waveText);

        static sf::Text suppliesText(hudFont, "", TextUtil::kBaseCharSize);
        static int      prevSupplies = -1;
        if (mCampaign.totalSupplies != prevSupplies)
        {
            TextUtil::setSize(suppliesText, static_cast<float>(hudCharSize));
            suppliesText.setString("Supplies:   " + std::to_string(mCampaign.totalSupplies));
            suppliesText.setFillColor(sf::Color(255, 215, 0));
            suppliesText.setPosition({28.f * scale, 68.f * scale});
            prevSupplies = mCampaign.totalSupplies;
        }
        window.draw(suppliesText);

        static sf::Text progLabel(hudFont, "", TextUtil::kBaseCharSize);
        static bool     progLabelInit = false;
        if (!progLabelInit)
        {
            TextUtil::setSize(progLabel, 18.f * scale);
            progLabel.setString("Wave Time: ");
            progLabel.setFillColor(sf::Color(190, 220, 255));
            progLabel.setPosition({28.f * scale, 95.f * scale});
            progLabelInit = true;
        }
        window.draw(progLabel);

        static sf::Text  progVal(hudFont, "", TextUtil::kBaseCharSize);
        static int       prevRemSec = -1;
        static sf::Color prevProgCol;
        if (remSec != prevRemSec || waveCol != prevProgCol)
        {
            TextUtil::setSize(progVal, 18.f * scale);
            progVal.setString(std::to_string(remSec) + "s remaining");
            progVal.setFillColor(waveCol);
            progVal.setPosition({140.f * scale, 95.f * scale});
            prevRemSec  = remSec;
            prevProgCol = waveCol;
        }
        window.draw(progVal);
    }

    const bool isPvPBanner = (client->gameState().getGameMode() == GameState::GameMode::PvP);
    const int  waveRemSec  = (mCampaign.waveTimerTicks + 59) / 60;
    const bool waveBlink   = (mCampaign.waveTimerTicks % 30 < 15);

    if (waveRemSec <= 4 && mCampaign.waveTimerTicks > 0)
    {
        const int nextW = (isPvPBanner || mCampaign.currentWave < mCampaign.maxWaves)
                              ? (mCampaign.currentWave + 1)
                              : mCampaign.currentWave;
        std::string urgentText = (mCampaign.currentWave == 0)
                                     ? ("WAVE 1 IN " + std::to_string(waveRemSec) + "s")
                                     : ((isPvPBanner || mCampaign.currentWave < mCampaign.maxWaves)
                                            ? ("WAVE " + std::to_string(nextW) + " IN " + std::to_string(waveRemSec) + "s")
                                            : ("FINAL WAVE SURVIVAL: " + std::to_string(waveRemSec) + "s"));

        static sf::Text    uText(hudFont, "", TextUtil::kBaseCharSize);
        static std::string prevUText;
        static bool        prevUBlink = false;
        if (urgentText != prevUText || waveBlink != prevUBlink)
        {
            TextUtil::setSize(uText, 28.f * scale);
            uText.setString(urgentText);
            uText.setFillColor(waveBlink ? sf::Color(255, 60, 60, 240) : sf::Color(255, 215, 0, 240));
            auto ubb = TextUtil::bounds(uText);
            uText.setPosition({(window.getView().getSize().x - ubb.size.x) / 2.f, 75.f * scale});
            prevUText  = urgentText;
            prevUBlink = waveBlink;
        }
        window.draw(uText);
    }

    const int elapsedInWave = mCampaign.waveTotalTimerTicks - mCampaign.waveTimerTicks;
    if (mCampaign.currentWave >= 1 && elapsedInWave >= 0 && elapsedInWave < 210)
    {
        const float   pulse       = (std::sin(elapsedInWave * 0.15f) + 1.f) * 0.5f;
        const uint8_t bannerAlpha = static_cast<uint8_t>(170 + 85 * pulse);

        std::string        bannerText = (isPvPBanner || mCampaign.currentWave < mCampaign.maxWaves)
                                            ? ("WAVE " + std::to_string(mCampaign.currentWave) + " INCOMING")
                                            : ("FINAL WAVE INCOMING");
        static sf::Text    bText(hudFont, "", TextUtil::kBaseCharSize);
        static std::string prevBText;
        static uint8_t     prevAlpha = 0;
        if (bannerText != prevBText || bannerAlpha != prevAlpha)
        {
            TextUtil::setSize(bText, 34.f * scale);
            bText.setString(bannerText);
            bText.setFillColor((!isPvPBanner && mCampaign.currentWave == mCampaign.maxWaves)
                                   ? sf::Color(255, 50, 50, bannerAlpha)
                                   : sf::Color(255, 215, 0, bannerAlpha));
            auto bb = TextUtil::bounds(bText);
            bText.setPosition({(window.getView().getSize().x - bb.size.x) / 2.f, 70.f * scale});
            prevBText = bannerText;
            prevAlpha = bannerAlpha;
        }
        window.draw(bText);
    }


    if (mDebug.showPanel)
    {
        const auto&        prof = client->mViewState->mProfiler;
        std::ostringstream ss;
        ss.precision(1);
        ss << std::fixed;
        ss << "PERFORMANCE PROFILER  [F3]\n";
        ss << "Total: Update " << prof.updateTime << "ms | Render " << prof.renderTime << "ms\n";
        ss << "Units: Friendly=" << prof.friendlyCount << " | Opponents=" << prof.opponentCount
           << " | Proj=" << prof.projCount << " | VFX=" << prof.vfxCount << "\n\n";

        ss << "[Simulation Pipeline]\n";
        for (const auto& [name, timeUs] : prof.systemTimes)
        {
            if (timeUs >= 1000.f)
            {
                ss << "  " << name << ": " << (timeUs / 1000.f) << " ms\n";
            }
            else
            {
                ss << "  " << name << ": " << static_cast<int>(timeUs) << " us\n";
            }
        }

        if (!prof.renderTimes.empty())
        {
            ss << "\n[Render Breakdown]\n";
            for (const auto& [name, timeUs] : prof.renderTimes)
            {
                if (timeUs >= 1000.f)
                {
                    ss << "  " << name << ": " << (timeUs / 1000.f) << " ms\n";
                }
                else
                {
                    ss << "  " << name << ": " << static_cast<int>(timeUs) << " us\n";
                }
            }
        }

        profilerText.setFillColor(sf::Color(220, 245, 255));
        profilerText.setString(ss.str());
        profilerText.setPosition({dPanelX, dPanelY + 10.f * scale});
        window.draw(profilerText);

        auto profBounds = TextUtil::bounds(profilerText);
        cachePanelW     = static_cast<int>(profBounds.size.x);
        cachePanelH     = static_cast<int>(profBounds.size.y);
    }

    if (mShowUnitInfoPanel)
    {
        for (size_t i = 0; i < unitClasses.size(); i++)
        {
            float btnX = unitBtnStartX + (unitBtnW + unitBtnSpacing) * (i % 3);
            float btnY = unitBtnStartY + (unitBtnH + unitBtnSpacing) * (i / 3);

            sf::Text unitText = TextUtil::make(mHudText.getFont(), unitLabels[i], 14.f * scale);
            unitText.setFillColor(sf::Color::White);
            auto bounds = unitText.getLocalBounds();
            unitText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
            unitText.setPosition({btnX + unitBtnW / 2.f, btnY + unitBtnH / 2.f});
            window.draw(unitText);
        }

        auto drawStatProgressText =
            [&](const std::string& name, float value, sf::Vector2f barPos, const std::string& extra = "")
        {
            sf::Text statLabel = TextUtil::make(hudFont, name, 13.f * scale);
            statLabel.setFillColor(sf::Color(170, 195, 220));
            statLabel.setPosition(barPos);
            window.draw(statLabel);

            float       trackWidth = 140.f * scale;
            std::string valStr     = std::to_string(value).substr(0, std::to_string(value).find('.') + 1);
            if (!extra.empty())
                valStr += " (" + extra + ")";

            sf::Text valText = TextUtil::make(hudFont, valStr, 13.f * scale);
            valText.setFillColor(sf::Color::White);
            valText.setPosition({barPos.x + (110.f + trackWidth + 10.f) * scale, barPos.y});
            window.draw(valText);
        };

        drawStatProgressText("Max HP:",
                             static_cast<float>(maxHp),
                             {uiPanelX + 20.f * scale, barsY},
                             "Base: " + std::to_string(maxHp - selCfg.hpBonus));
        drawStatProgressText("Armor:", static_cast<float>(armor), {uiPanelX + 20.f * scale, barsY + rowSpacing});
        drawStatProgressText("Range:", attackRange, {uiPanelX + 20.f * scale, barsY + rowSpacing * 2});
        drawStatProgressText("Damage:",
                             damage,
                             {uiPanelX + 20.f * scale, barsY + rowSpacing * 3},
                             "Base: " + std::to_string(damage - selCfg.damageBonus).substr(0, 3));
        drawStatProgressText("Speed:", moveSpeed * 100.f, {uiPanelX + 20.f * scale, barsY + rowSpacing * 4});

        sf::Text tactHeader = TextUtil::make(hudFont, "TACTICAL EVALUATION:", 14.f * scale);
        tactHeader.setFillColor(sf::Color(0, 205, 255));
        tactHeader.setPosition({uiPanelX + 30.f * scale, tactY + 12.f * scale});
        window.draw(tactHeader);

        std::string rawPlaystyle = std::string(playStyle);
        std::string formattedPlaystyle;
        size_t      charCount = 0;
        for (char c : rawPlaystyle)
        {
            formattedPlaystyle += c;
            charCount++;
            if (charCount > 23 && c == ' ')
            {
                formattedPlaystyle += "\n";
                charCount = 0;
            }
        }

        sf::Text tactText = TextUtil::make(hudFont, formattedPlaystyle, 14.f * scale);
        tactText.setFillColor(sf::Color(200, 210, 225));
        tactText.setPosition({uiPanelX + 30.f * scale, tactY + 35.f * scale});
        window.draw(tactText);
    }
}
