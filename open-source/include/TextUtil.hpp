#ifndef TEXTUTIL_HPP
#define TEXTUTIL_HPP

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>

namespace TextUtil
{
inline constexpr unsigned int kBaseCharSize = 48;

void setSize(sf::Text& text, float pixelSize);

[[nodiscard]] sf::Text make(const sf::Font& font, const sf::String& str, float pixelSize);

[[nodiscard]] sf::FloatRect bounds(const sf::Text& text);
} // namespace TextUtil

#endif
