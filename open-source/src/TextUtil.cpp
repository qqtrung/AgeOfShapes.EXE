#include "TextUtil.hpp"

namespace TextUtil
{
void setSize(sf::Text& text, float pixelSize)
{
    if (text.getCharacterSize() != kBaseCharSize)
    {
        text.setCharacterSize(kBaseCharSize);
    }
    const float s = pixelSize / static_cast<float>(kBaseCharSize);
    text.setScale({s, s});
}

sf::Text make(const sf::Font& font, const sf::String& str, float pixelSize)
{
    sf::Text text(font, str, kBaseCharSize);
    setSize(text, pixelSize);
    return text;
}

sf::FloatRect bounds(const sf::Text& text)
{
    const sf::FloatRect b  = text.getLocalBounds();
    const sf::Vector2f  sc = text.getScale();
    return {{b.position.x * sc.x, b.position.y * sc.y}, {b.size.x * sc.x, b.size.y * sc.y}};
}
} // namespace TextUtil
