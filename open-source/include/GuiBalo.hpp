#ifndef GUI_BALO_HPP
#define GUI_BALO_HPP

#include "GuiCombat.hpp"
#include "Space.hpp"

#include <SFML/Graphics.hpp>

struct GuiBalo : Space
{
    struct Slot : Space
    {
        sf::RectangleShape rect;
        sf::Text           stockText;
        int                mIndex     = 0;
        WallKind           mKind      = WallKind::Defensive;
        UnitClass          mUnitClass = UnitClass::Warrior;

        RenderBatch mCachedBatch;
        bool        mNeedsRebuild = true;
        bool        mPrevSelected = false;
        bool        mPrevHovered  = false;
        bool        mPrevUnitMode = false;
        int         mPrevStock    = -999;
        sf::Color   mPrevWallColor;

        int  mPrevTextStock   = -999;
        bool mPrevTextHovered = false;

        Slot(sf::Font& font);
        ~Slot() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;

        void drawShapes(RenderBatch& batch,
                        sf::Vector2f pos,
                        int          stock,
                        bool         selected,
                        sf::Color    wallColor,
                        sf::Vector2f wallSize,
                        float        scale,
                        bool         unitMode,
                        bool         hovered = false);

        void drawText(sf::RenderTarget& target, sf::Vector2f pos, int stock, float scale, bool hovered = false);
    };

    RenderBatch mBatch;

    GuiBalo(sf::Font& font);

    bool input(const sf::Event&) override;
    void update() override;
    void render() override;
};

#endif
