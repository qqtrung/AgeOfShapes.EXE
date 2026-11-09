#ifndef SOUNDRANKER_HPP
#define SOUNDRANKER_HPP

#include <SFML/Graphics/Rect.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <deque>
#include <optional>
#include <string>

#include <cmath>

struct SoundEventContext
{
    std::string                 soundName;
    std::optional<sf::Vector2f> position;
    float                       baseImportance = 0.5f;
    int                         sourceEntityId = -1;
};

struct SoundRankResult
{
    float score       = 0.0f;
    float volumeRatio = 0.0f;
    float pan         = 0.0f;
    bool  shouldPlay  = false;
};

struct SoundRankerConfig
{
    float wDist      = 0.40f;
    float wImp       = 0.35f;
    float wScreen    = 0.15f;
    float wFreshness = 0.10f;

    float minDistance      = 200.0f;
    float maxDistance      = 1500.0f;
    float cullingThreshold = 0.08f;
    float volumeExponent   = 1.15f;

    sf::Time frequencyWindow = sf::milliseconds(150);
};

class SoundRanker
{
public:
    SoundRankerConfig mConfig;

    static float getDefaultImportance(const std::string& soundName);

    SoundRankResult evaluate(const SoundEventContext&            context,
                             const std::optional<sf::Vector2f>&  listenerPos,
                             const std::optional<sf::FloatRect>& viewBounds);

    void recordPlay(const std::string& soundName);

private:
    struct PlayRecord
    {
        std::string name;
        sf::Time    time;
    };

    sf::Clock              mClock;
    std::deque<PlayRecord> mHistory;

    void cleanupOldHistory();
    int  countRecentPlays(const std::string& name) const;
};

#endif
