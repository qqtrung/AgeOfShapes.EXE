#ifndef GUISetting_HPP
#define GUISetting_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

#include <array>

struct GUISetting : Space
{
    struct Button : Space
    {
        enum class ButtonType
        {
            MasterVolumeMinus,
            MasterVolumePlus,
            MusicVolumeMinus,
            MusicVolumePlus,
            SfxVolumeMinus,
            SfxVolumePlus,
            GraphicsMinus,
            GraphicsPlus,
            Back
        };

        ButtonType   mType;
        sf::Vector2f mOriginalSize;
        sf::Text     mText;

        Button(ButtonType type, sf::Font& font);
        ~Button() override;

        void update() override;
        bool input(const sf::Event& event) override;
        void render() override;
    };

    struct LabelData
    {
        sf::Text text;

        LabelData(sf::Font& font);
    };

    sf::Text mBtnText;
    sf::Text mLabelText;

    std::array<LabelData, 4> mLabels;

    GUISetting(sf::Font& font);
    ~GUISetting() override;

    bool input(const sf::Event& event) override;
    void update() override;

    void drawLabel(sf::RenderTarget&  target,
                   LabelData&         label,
                   const std::string& str,
                   sf::Vector2f       pos,
                   unsigned int       charSize = 16);
    void drawProgressBar(sf::RenderTarget& target, sf::Vector2f centerPos, float value, sf::Color fillColor, float scale);
    void drawSegmentSelector(sf::RenderTarget& target,
                             sf::Vector2f      centerPos,
                             int               selectedIndex,
                             sf::Font&         font,
                             sf::Vector2f      mousePos,
                             float             scale);

    void render() override;
};

#endif
