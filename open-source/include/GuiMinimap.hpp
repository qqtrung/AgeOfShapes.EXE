#ifndef GUIMINIMAP_HPP
#define GUIMINIMAP_HPP

#include "GuiCombat.hpp"
#include "Space.hpp"

#include <SFML/Graphics.hpp>

class ClientGame;

struct GUIMinimap : Space
{
    sf::Font&     mFont;
    RenderBatch   mBatch;
    sf::FloatRect mMinimapRect;
    bool          mIsDragging     = false;
    int           mActiveFingerId = -1;

    explicit GUIMinimap(sf::Font& font);
    ~GUIMinimap() override;

    bool                        input(const sf::Event& event) override;
    void                        update() override;
    void                        render() override;
    [[nodiscard]] sf::FloatRect getMinimapRect(ClientGame* client) const;

private:
    void handleMoveCameraAt(ClientGame* client, sf::Vector2f uiPos);
};

#endif
