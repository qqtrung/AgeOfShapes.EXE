#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <SFML/Graphics.hpp>

#include <optional>

struct Animation
{
    std::optional<sf::Sprite> mSprite;
    sf::Vector2i              mFrameSize;
    int                       mFrames       = 0;
    int                       mSpeed        = 0;
    int                       mCurrentFrame = 0;

    Animation() = default;
    Animation(const sf::Texture& texture, int frames, int speed);

    void init(const sf::Texture& texture, int frames, int speed);

    [[nodiscard]] bool isReady() const
    {
        return mSprite.has_value();
    }

    [[nodiscard]] sf::Sprite& sprite()
    {
        return *mSprite;
    }
    [[nodiscard]] const sf::Sprite& sprite() const
    {
        return *mSprite;
    }
};

#endif
