#include "SoundRanker.hpp"

#include <algorithm>

#include <cmath>

float SoundRanker::getDefaultImportance(const std::string& soundName)
{
    if (soundName == "SndDie")
        return 0.85f;
    if (soundName == "SndPickup")
        return 0.80f;
    if (soundName == "SndBuild")
        return 0.80f;
    if (soundName == "SndShoot")
        return 0.65f;
    if (soundName == "SndWallFire")
        return 0.70f;
    if (soundName == "SndWallElectric")
        return 0.70f;
    if (soundName == "SndHit")
        return 0.55f;
    return 0.50f;
}

SoundRankResult SoundRanker::evaluate(const SoundEventContext&            context,
                                      const std::optional<sf::Vector2f>&  listenerPos,
                                      const std::optional<sf::FloatRect>& viewBounds)
{
    cleanupOldHistory();

    float distFactor = 1.0f;

    if (context.position && listenerPos)
    {
        float dx   = context.position->x - listenerPos->x;
        float dy   = context.position->y - listenerPos->y;
        float dist = std::hypot(dx, dy);

        if (dist <= mConfig.minDistance)
        {
            distFactor = 1.0f;
        }
        else if (dist >= mConfig.maxDistance)
        {
            distFactor = 0.0f;
        }
        else
        {
            float norm = (dist - mConfig.minDistance) / (mConfig.maxDistance - mConfig.minDistance);
            float inv  = 1.0f - norm;
            distFactor = inv * inv;
        }
    }

    float impFactor = std::clamp(context.baseImportance, 0.0f, 1.0f);

    float screenFactor = 1.0f;
    if (context.position && viewBounds)
    {
        if (viewBounds->contains(*context.position))
        {
            screenFactor = 1.0f;
        }
        else
        {
            screenFactor = 0.40f;
        }
    }

    int   recentCount     = countRecentPlays(context.soundName);
    float freshnessFactor = 1.0f / (1.0f + 0.30f * static_cast<float>(recentCount));

    float score = mConfig.wDist * distFactor + mConfig.wImp * impFactor + mConfig.wScreen * screenFactor +
                  mConfig.wFreshness * freshnessFactor;

    score = std::clamp(score, 0.0f, 1.0f);

    SoundRankResult result;
    result.score = score;
    result.pan   = 0.0f;

    if (score < mConfig.cullingThreshold)
    {
        result.shouldPlay  = false;
        result.volumeRatio = 0.0f;
    }
    else
    {
        result.shouldPlay  = true;
        result.volumeRatio = std::pow(std::max(0.0f, score), mConfig.volumeExponent);
    }

    return result;
}

void SoundRanker::recordPlay(const std::string& soundName)
{
    sf::Time now = mClock.getElapsedTime();
    mHistory.push_back({soundName, now});
}

void SoundRanker::cleanupOldHistory()
{
    sf::Time now = mClock.getElapsedTime();
    while (!mHistory.empty() && (now - mHistory.front().time) > mConfig.frequencyWindow)
    {
        mHistory.pop_front();
    }
}

int SoundRanker::countRecentPlays(const std::string& name) const
{
    int count = 0;
    for (const auto& rec : mHistory)
    {
        if (rec.name == name)
        {
            count++;
        }
    }
    return count;
}
