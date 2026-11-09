#include "FPSMeter.hpp"

FPSMeter::FPSMeter(sf::Font& font) : mFont(font), mFpsText(font), mRttText(font), mBufText(font)
{
    TextUtil::setSize(mFpsText, 18);
    mFpsText.setFillColor(sf::Color(230, 240, 255));

    TextUtil::setSize(mRttText, 18);
    mRttText.setFillColor(sf::Color(230, 240, 255));

    TextUtil::setSize(mBufText, 18);
    mBufText.setFillColor(sf::Color(230, 240, 255));
}

FPSMeter::~FPSMeter()
{
    logout("~FPSMeter()");
}

void FPSMeter::setRtt(uint32_t rtt, bool isConnected, bool networkActive)
{
    mLastRtt       = rtt;
    mIsConnected   = isConnected;
    mNetworkActive = networkActive;
}

void FPSMeter::setBufferSize(int size)
{
    mBufferSize = size;
}

void FPSMeter::update()
{
    countUpdate++;

    if (mFpsClock.getElapsedTime().asMilliseconds() >= 1000)
    {
        mLastFps    = countRender;
        mLastUps    = countUpdate;
        countUpdate = 0;
        countRender = 0;
        mFpsClock.restart();
    }
}

void FPSMeter::draw(sf::RenderTarget& target, const sf::View& uiView)
{
    render();

    sf::View oldView = target.getView();
    target.setView(uiView);

    sf::Color fpsColor = sf::Color(50, 255, 140);
    if (mLastFps < 30)
    {
        fpsColor = sf::Color(255, 80, 80);
    }
    else if (mLastFps < 55)
    {
        fpsColor = sf::Color(255, 205, 50);
    }

    int       activeBars = 0;
    sf::Color pingColor  = sf::Color(130, 145, 165);
    if (mNetworkActive && mIsConnected)
    {
        if (mLastRtt < 60)
        {
            activeBars = 4;
            pingColor  = sf::Color(50, 255, 140);
        }
        else if (mLastRtt < 120)
        {
            activeBars = 3;
            pingColor  = sf::Color(160, 245, 70);
        }
        else if (mLastRtt < 200)
        {
            activeBars = 2;
            pingColor  = sf::Color(255, 205, 50);
        }
        else
        {
            activeBars = 1;
            pingColor  = sf::Color(255, 80, 80);
        }
    }

    std::string fpsStr = "FPS " + std::to_string(mLastFps);
    mFpsText.setString(fpsStr);
    TextUtil::setSize(mFpsText, 22);
    mFpsText.setFillColor(fpsColor);
    auto fpsBounds = TextUtil::bounds(mFpsText);

    std::string rttStr = (mNetworkActive && mIsConnected) ? (std::to_string(mLastRtt) + " ms") : "? ms";
    mRttText.setString(rttStr);
    TextUtil::setSize(mRttText, 22);
    mRttText.setFillColor((mNetworkActive && mIsConnected) ? pingColor : sf::Color(160, 175, 195));
    auto rttBounds = TextUtil::bounds(mRttText);

    constexpr float barW       = 4.5f;
    constexpr float barGap     = 3.f;
    constexpr float wifiTotalW = 4 * barW + 3 * barGap;

    constexpr float bufBarW = 56.f;
    constexpr float bufBarH = 14.f;

    const bool showBuffer = (mBufferSize >= 0);
    sf::Color  bufColor   = sf::Color(50, 255, 140);
    if (mBufferSize == 0)
    {
        bufColor = sf::Color(255, 80, 80);
    }
    else if (mBufferSize < 3)
    {
        bufColor = sf::Color(255, 205, 50);
    }
    else if (mBufferSize <= 5)
    {
        bufColor = sf::Color(50, 255, 140);
    }
    else if (mBufferSize <= 7)
    {
        bufColor = sf::Color(80, 200, 255);
    }
    else
    {
        bufColor = sf::Color(255, 130, 50);
    }

    const float badgeH = 40.f;
    const float padX   = 16.f;
    const float gapX   = 14.f;
    float       badgeW = padX + fpsBounds.size.x + gapX + 2.5f + gapX + wifiTotalW + 10.f + rttBounds.size.x;
    if (showBuffer)
    {
        badgeW += gapX + 2.5f + gapX + bufBarW;
    }
    badgeW += padX;

    const float badgeX = 60.f;
    const float badgeY = 1110.f;

    const float fpsX     = badgeX + padX;
    const float dividerX = fpsX + fpsBounds.size.x + gapX;
    const float wifiX    = dividerX + 2.5f + gapX;
    const float rttX     = wifiX + wifiTotalW + 10.f;

    const float divider2X = rttX + rttBounds.size.x + gapX;
    const float bufGaugeX = divider2X + 2.5f + gapX;

    sf::RectangleShape badge({badgeW, badgeH});
    badge.setPosition({badgeX, badgeY});
    badge.setFillColor(sf::Color(10, 15, 24, 240));
    badge.setOutlineColor(sf::Color(100, 145, 205, 245));
    badge.setOutlineThickness(2.5f);
    target.draw(badge);

    mFpsText.setPosition({fpsX, badgeY + (badgeH - fpsBounds.size.y) / 2.f - fpsBounds.position.y});
    target.draw(mFpsText);

    sf::RectangleShape divider({2.5f, 24.f});
    divider.setPosition({dividerX, badgeY + (badgeH - 24.f) / 2.f});
    divider.setFillColor(sf::Color(110, 155, 215, 240));
    target.draw(divider);

    const float maxBarH       = fpsBounds.size.y;
    const float barHeights[4] = {maxBarH * 0.28f, maxBarH * 0.52f, maxBarH * 0.76f, maxBarH};
    const float barBottomY    = badgeY + (badgeH + maxBarH) / 2.f;
    for (int i = 0; i < 4; ++i)
    {
        float              h = barHeights[i];
        sf::RectangleShape bar({barW, h});
        bar.setPosition({wifiX + i * (barW + barGap), barBottomY - h});
        bar.setFillColor((i < activeBars) ? pingColor : sf::Color(70, 90, 120, 210));
        target.draw(bar);
    }

    mRttText.setPosition({rttX, badgeY + (badgeH - rttBounds.size.y) / 2.f - rttBounds.position.y});
    target.draw(mRttText);

    if (showBuffer)
    {
        sf::RectangleShape divider2({2.5f, 24.f});
        divider2.setPosition({divider2X, badgeY + (badgeH - 24.f) / 2.f});
        divider2.setFillColor(sf::Color(110, 155, 215, 240));
        target.draw(divider2);

        const float barY = badgeY + (badgeH - bufBarH) / 2.f;

        sf::RectangleShape bufBg({bufBarW, bufBarH});
        bufBg.setPosition({bufGaugeX, barY});
        bufBg.setFillColor(sf::Color(20, 28, 42, 220));
        bufBg.setOutlineColor(sf::Color(70, 95, 130, 200));
        bufBg.setOutlineThickness(1.5f);
        target.draw(bufBg);

        float ratio = std::clamp(static_cast<float>(mBufferSize) / 10.f, 0.f, 1.f);
        float fillW = bufBarW * ratio;
        if (fillW > 0.f)
        {
            sf::RectangleShape bufFill({fillW, bufBarH});
            bufFill.setPosition({bufGaugeX, barY});
            bufFill.setFillColor(bufColor);
            target.draw(bufFill);
        }
    }

    target.setView(oldView);
}

void FPSMeter::draw(sf::RenderTarget& target, const sf::View& uiView, uint32_t rtt, bool isConnected, bool networkActive)
{
    setRtt(rtt, isConnected, networkActive);
    draw(target, uiView);
}

void FPSMeter::draw(sf::RenderTarget& target)
{
    draw(target, target.getView());
}

void FPSMeter::render()
{
    countRender++;
}

bool FPSMeter::input(const sf::Event&)
{
    return false;
}
