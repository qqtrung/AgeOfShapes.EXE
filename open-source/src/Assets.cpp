#include "Assets.hpp"

#include <sstream>

Assets::Assets()
{
    soundManager.init(&soundBufferMap, &maxVolumeMap);
}

void Assets::addTexture(const std::string& textureName, const std::string& path)
{
    ASSERT(textureMap[textureName].loadFromFile(path));
    textureMap[textureName].setSmooth(true);
}

void Assets::addAnimation(const std::string& animationName, const std::string& textureName, size_t frames, size_t speed)
{
    ASSERT(textureMap.contains(textureName));
    const sf::Texture& texture = textureMap.at(textureName);
    animationMap.emplace(animationName, Animation(texture, frames, speed));
}

void Assets::addFont(const std::string& fontName, const std::string& path)
{
    ASSERT(fontMap[fontName].openFromFile(path));
}

void Assets::addSound(const std::string& soundName, const std::string& soundPath, int volume)
{
    ASSERT(soundBufferMap[soundName].loadFromFile(soundPath));
    const sf::Sound sound{soundBufferMap.at(soundName)};
    soundMap.emplace(soundName, sound);
    maxVolumeMap.emplace(soundName, volume);
    soundMap.at(soundName).setVolume(volume / 2);
}

void Assets::playSound(const std::string& soundName)
{
    SoundEventContext ctx;
    ctx.soundName      = soundName;
    ctx.baseImportance = SoundRanker::getDefaultImportance(soundName);
    soundManager.playSound(ctx);
}

void Assets::playSound(const std::string&                  soundName,
                       const std::optional<sf::Vector2f>&  worldPos,
                       std::optional<float>                customImportance,
                       const std::optional<sf::Vector2f>&  listenerPos,
                       const std::optional<sf::FloatRect>& viewBounds)
{
    SoundEventContext ctx;
    ctx.soundName      = soundName;
    ctx.position       = worldPos;
    ctx.baseImportance = customImportance.value_or(SoundRanker::getDefaultImportance(soundName));
    soundManager.playSound(ctx, listenerPos, viewBounds);
}

void Assets::setAllVolumes(float ratioMasterVolume, float, float)
{
    soundManager.setMasterVolumeRatio(ratioMasterVolume);
    for (auto& [name, sound] : soundMap)
    {
        auto maxVolume = maxVolumeMap.at(name);
        sound.setVolume(ratioMasterVolume * maxVolume);
    }
}

void Assets::loadFromFile(const std::string& path)
{
    auto loadTextFile = [&](const std::string& filePath)
    {
        sf::FileInputStream stream;
        ASSERT(stream.open(filePath));

        std::string text;
        text.resize(stream.getSize().value());

        ASSERT((bool)stream.read(text.data(), text.size()));

        return text;
    };

    std::istringstream file(loadTextFile(path));
    while (file >> std::ws && !file.eof())
    {
        std::string str;
        file >> str;
        if (str == "Texture")
        {
            std::string name;
            std::string texPath;
            file >> name >> texPath;
            addTexture(name, texPath);
        }
        else if (str == "Animation")
        {
            std::string name;
            std::string texture;
            size_t      frames = 0;
            size_t      speed  = 0;
            file >> name >> texture >> frames >> speed;
            addAnimation(name, texture, frames, speed);
        }
        else if (str == "Font")
        {
            std::string name;
            std::string fontPath;
            file >> name >> fontPath;
            addFont(name, fontPath);
        }
        else if (str == "Sound")
        {
            std::string name;
            std::string soundPath;
            int         maxVolume = 100;
            file >> name >> soundPath >> maxVolume;
            addSound(name, soundPath, maxVolume);
        }
        else
        {
            ASSERT(false);
        }
    }
}
