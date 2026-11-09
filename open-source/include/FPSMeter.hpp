#ifndef FPSMETER_HPP
#define FPSMETER_HPP

#include "Space.hpp"
#include "TextUtil.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <string>

struct FPSMeter : Space
{
    sf::Font& mFont;
    sf::Text  mFpsText;
    sf::Text  mRttText;
    sf::Text  mBufText;
    sf::Clock mFpsClock;
    int       countUpdate    = 0;
    int       countRender    = 0;
    int       mLastFps       = 60;
    int       mLastUps       = 60;
    uint32_t  mLastRtt       = 0;
    int       mBufferSize    = -1;
    bool      mIsConnected   = false;
    bool      mNetworkActive = false;

    FPSMeter(sf::Font& font);
    ~FPSMeter() override;

    void setRtt(uint32_t rtt, bool isConnected = true, bool networkActive = true);
    void setBufferSize(int size);

    void update() override;

    void draw(sf::RenderTarget& target, const sf::View& uiView);
    void draw(sf::RenderTarget& target, const sf::View& uiView, uint32_t rtt, bool isConnected = true, bool networkActive = true);
    void draw(sf::RenderTarget& target);

    void render() override;
    bool input(const sf::Event&) override;
};

#endif
