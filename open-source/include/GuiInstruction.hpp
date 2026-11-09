#ifndef GUIINSTRUCTION_HPP
#define GUIINSTRUCTION_HPP

#include "Space.hpp"

#include <SFML/Graphics.hpp>

#include <array>

struct GUIInstruction : Space
{
    struct Button : Space
    {
        enum class ButtonType
        {
            Vietnamese,
            English,
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

    std::array<LabelData, 15> mLabels;

    GUIInstruction(sf::Font& font);
    ~GUIInstruction() override;

    bool input(const sf::Event& event) override;
    void update() override;

    void drawPanel(sf::RenderTarget&  target,
                   sf::FloatRect      rect,
                   const std::string& title,
                   sf::Font&          font,
                   float              scale,
                   sf::Color          outlineColor = sf::Color(110, 140, 180, 80));

    void drawLabel(sf::RenderTarget&  target,
                   LabelData&         label,
                   const std::string& str,
                   sf::Vector2f       pos,
                   unsigned int       charSize = 15,
                   sf::Color          color    = sf::Color(220, 230, 255),
                   bool               bold     = false);

    void render() override;
};

#endif
