#ifndef GUICOMBAT_HPP
#define GUICOMBAT_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct RenderBatch
{
    sf::VertexArray triangles;
    sf::VertexArray lines;

    RenderBatch();
    void clear();
    void draw(sf::RenderTarget& target);
    void addRect(sf::Vector2f pos, sf::Vector2f size, sf::Color color);
    void addRectWithOutline(sf::Vector2f pos, sf::Vector2f size, sf::Color fillColor, sf::Color outlineColor, float thickness);
    void addLine(sf::Vector2f p1, sf::Vector2f p2, sf::Color color);
    void addTechBrackets(sf::Vector2f pos, sf::Vector2f size, float scale, float bracketLen, sf::Color color);
};

struct GUICombat : Space
{
    struct Button : Space
    {
        enum class ButtonType
        {
            Back,
            Exit,
            CombatMode,
            DesignatedMode,
            ToggleInputMode,
            TogglePlacementPalette,
        };

        ButtonType   mType;
        sf::Vector2f mOriginalSize;
        sf::Text     mText;

        RenderBatch  mCachedBatch;
        bool         mNeedsRebuild = true;
        bool         mPrevHovered  = false;
        sf::Color    mPrevFillColor;
        sf::Vector2f mPrevBtnPos;

        Button(ButtonType type, sf::Font& font);
        ~Button() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;

        void drawShapes(RenderBatch& batch, sf::Vector2f pos, sf::Color fillColor, float scale, bool hovered = false);
        void drawText(sf::RenderTarget& target, sf::Vector2f pos, float scale, bool hovered = false);
    };

    void drawTechBrackets(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, float scale, float bracketLen, sf::Color color);

    sf::Text    mHudText;
    RenderBatch mBatch;

    enum class CombatMode
    {
        None,
        Attack,
        Tactical,
    };

    CombatMode mCombatMode = CombatMode::None;

    bool mShowUnitInfoPanel = false;
    bool mShowHud           = false;
    bool mShowExitConfirm   = false;

    UnitClass mSelectedUnitClass = UnitClass::Warrior;
    int       mSelectedAgentId   = -1;

    GUICombat(sf::Font& font);
    ~GUICombat() override;

    bool input(const sf::Event& event) override;
    void update() override;
    void render() override;
};

#endif
