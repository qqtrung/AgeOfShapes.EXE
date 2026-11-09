#include "Animation.hpp"

Animation::Animation(const sf::Texture& texture, int frames, int speed)
{
    init(texture, frames, speed);
}

void Animation::init(const sf::Texture& texture, int frames, int speed)
{
    mFrames       = frames;
    mSpeed        = speed;
    mCurrentFrame = 0;
    mFrameSize.x  = static_cast<int>(texture.getSize().x) / frames;
    mFrameSize.y  = static_cast<int>(texture.getSize().y);
    mSprite.emplace(texture);
    mSprite->setTextureRect({{0, 0}, mFrameSize});
    mSprite->setOrigin({mFrameSize.x / 2.f, mFrameSize.y / 2.f});
}
