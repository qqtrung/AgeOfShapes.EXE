#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "Animation.hpp"
#include "Debug.hpp"
#include "SoundManager.hpp"

#include <SFML/Graphics.hpp>

#include <SFML/Audio.hpp>
#include <SFML/Audio/SoundSource.hpp>

#include <map>
#include <optional>
#include <sstream>
#include <string>

struct Assets
{
    std::map<std::string, sf::SoundBuffer> soundBufferMap;
    std::map<std::string, sf::Texture>     textureMap;
    std::map<std::string, sf::Font>        fontMap;
    std::map<std::string, Animation>       animationMap;
    std::map<std::string, sf::Sound>       soundMap;
    std::map<std::string, size_t>          maxVolumeMap;
    SoundManager                           soundManager;

    Assets();

    void addTexture(const std::string& textureName, const std::string& path);
    void addAnimation(const std::string& animationName, const std::string& textureName, size_t frames, size_t speed);
    void addFont(const std::string& fontName, const std::string& path);
    void addSound(const std::string& soundName, const std::string& soundPath, int volume);

    void playSound(const std::string& soundName);
    void playSound(const std::string&                  soundName,
                   const std::optional<sf::Vector2f>&  worldPos,
                   std::optional<float>                customImportance = std::nullopt,
                   const std::optional<sf::Vector2f>&  listenerPos      = std::nullopt,
                   const std::optional<sf::FloatRect>& viewBounds       = std::nullopt);

    void setAllVolumes(float ratioMasterVolume, float ratioMusicVolume, float ratioSfxVolume);
    void loadFromFile(const std::string& path);
};

#endif
