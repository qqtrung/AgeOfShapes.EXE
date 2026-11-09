#include "SoundManager.hpp"

#include "Debug.hpp"

#include <algorithm>

void SoundManager::init(const std::map<std::string, sf::SoundBuffer>* buffers, const std::map<std::string, size_t>* maxVolumes)
{
    mSoundBuffers = buffers;
    mMaxVolumes   = maxVolumes;
    mSoundGroups.clear();

    if (!mSoundBuffers)
        return;

    for (const auto& [name, buffer] : *mSoundBuffers)
    {
        auto& group = mSoundGroups[name];
        group.voices.reserve(VOICES_PER_SOUND);
        for (size_t i = 0; i < VOICES_PER_SOUND; ++i)
        {
            group.voices.push_back(std::make_unique<sf::Sound>(buffer));
        }
    }
}

void SoundManager::setMasterVolumeRatio(float ratio)
{
    mMasterVolume = std::clamp(ratio, 0.0f, 1.0f);
}

void SoundManager::stopAll()
{
    for (auto& [name, group] : mSoundGroups)
    {
        for (auto& voice : group.voices)
        {
            if (voice)
            {
                voice->stop();
            }
        }
    }
}

SoundRankResult SoundManager::playSound(const SoundEventContext&            context,
                                        const std::optional<sf::Vector2f>&  listenerPos,
                                        const std::optional<sf::FloatRect>& viewBounds)
{
    if (!mSoundBuffers || !mSoundBuffers->contains(context.soundName))
    {
        return SoundRankResult{.score = 0.f, .volumeRatio = 0.f, .pan = 0.f, .shouldPlay = false};
    }

    auto it = mSoundGroups.find(context.soundName);
    if (it == mSoundGroups.end())
    {
        if (!mSoundBuffers || !mSoundBuffers->contains(context.soundName))
        {
            logout("[SoundManager] Sound not found in buffer: " + context.soundName, true);
            return SoundRankResult{.score = 0.f, .volumeRatio = 0.f, .pan = 0.f, .shouldPlay = false};
        }
        const auto& buffer = mSoundBuffers->at(context.soundName);
        auto&       group  = mSoundGroups[context.soundName];
        group.voices.reserve(VOICES_PER_SOUND);
        for (size_t i = 0; i < VOICES_PER_SOUND; ++i)
        {
            group.voices.push_back(std::make_unique<sf::Sound>(buffer));
        }
        it = mSoundGroups.find(context.soundName);
    }

    auto& group = it->second;

    if (group.lastPlayClock.getElapsedTime().asMilliseconds() < 35)
    {
        return SoundRankResult{.score = 0.f, .volumeRatio = 0.f, .pan = 0.f, .shouldPlay = false};
    }

    SoundRankResult rank = mRanker.evaluate(context, listenerPos, viewBounds);
    if (!rank.shouldPlay)
    {
        return rank;
    }

    sf::Sound* targetVoice = nullptr;

    for (auto& voice : group.voices)
    {
        if (voice->getStatus() == sf::Sound::Status::Stopped)
        {
            targetVoice = voice.get();
            break;
        }
    }

    if (!targetVoice)
    {
        targetVoice     = group.voices[group.nextIndex].get();
        group.nextIndex = (group.nextIndex + 1) % group.voices.size();
    }

    float rawVol = 200.0f;
    if (mMaxVolumes && mMaxVolumes->contains(context.soundName))
    {
        rawVol = static_cast<float>(mMaxVolumes->at(context.soundName));
    }

    float baseVol  = std::min(100.0f, rawVol * 0.5f);
    float finalVol = baseVol * 0.45f * mMasterVolume * rank.volumeRatio;

    targetVoice->setVolume(std::clamp(finalVol, 0.0f, 100.0f));
    targetVoice->play();

    group.lastPlayClock.restart();
    mRanker.recordPlay(context.soundName);

    return rank;
}
