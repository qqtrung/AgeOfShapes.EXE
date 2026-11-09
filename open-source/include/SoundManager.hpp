#ifndef SOUNDMANAGER_HPP
#define SOUNDMANAGER_HPP

#include "SoundRanker.hpp"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <SFML/System/Clock.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class SoundManager
{
public:
    static constexpr size_t VOICES_PER_SOUND = 3;

    struct SoundChannelGroup
    {
        std::vector<std::unique_ptr<sf::Sound>> voices;
        size_t                                  nextIndex = 0;
        sf::Clock                               lastPlayClock;
    };

    SoundRanker                                   mRanker;
    std::map<std::string, SoundChannelGroup>      mSoundGroups;
    const std::map<std::string, sf::SoundBuffer>* mSoundBuffers = nullptr;
    const std::map<std::string, size_t>*          mMaxVolumes   = nullptr;
    float                                         mMasterVolume = 0.5f;

    SoundManager() = default;

    void init(const std::map<std::string, sf::SoundBuffer>* buffers, const std::map<std::string, size_t>* maxVolumes);
    void setMasterVolumeRatio(float ratio);
    void stopAll();

    SoundRankResult playSound(const SoundEventContext&            context,
                              const std::optional<sf::Vector2f>&  listenerPos = std::nullopt,
                              const std::optional<sf::FloatRect>& viewBounds  = std::nullopt);
};

#endif
