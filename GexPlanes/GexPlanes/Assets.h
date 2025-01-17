//
// Created by David Burchill on 2023-10-31.
//

#ifndef BREAKOUT_ASSETS_H
#define BREAKOUT_ASSETS_H

#include  "Animation.h"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>

struct SpriteRec {
    std::string texName;
    sf::IntRect texRect;
};

struct AnimationRec {
    std::string     texName;
    sf::Vector2i    frameSize;
    size_t          numbFrames;
    sf::Time        duration;
    bool            repeat;
};

class Assets {

private:
    // singleton class
    Assets();
    ~Assets() = default;

public:
    static Assets& getInstance();

    // no copy or move
    Assets(const Assets&)               = delete;
    Assets(Assets&&)                    = delete;
    Assets& operator=(const Assets&)    = delete;
    Assets& operator=( Assets&&)        = delete;

private:
    std::map<std::string, std::unique_ptr<sf::Font>>            _fontMap;
    std::map<std::string, std::unique_ptr<sf::SoundBuffer>>     _soundEffects;
    std::map<std::string, sf::Texture>                          _textures;

    std::map<std::string, Animation>                            _animationMap;
    std::map<std::string, std::vector<sf::IntRect>>             _frameSets;

    void loadFonts(const std::string& path);
    void loadTextures(const std::string& path);
    void loadSounds(const std::string& path);
    void loadJson(const std::string& path);
    void loadAnimations(const std::string& path);
    void loadSpriteRecs(const std::string& path);
    void loadAnimationRecs(const std::string& path);

public:
    void loadFromFile(const std::string path);

    void addFont(const std::string &fontName, const std::string &path);
    void addSound(const std::string &soundEffectName, const std::string &path);
    void addTexture(const std::string& textureName, const std::string& path, bool smooth = true);

    void addSpriteRec(const std::string& name, SpriteRec sr);
    void addAnimationRec(const std::string& name, AnimationRec ar);



    const sf::Font&             getFont(const std::string &fontName) const;
    const sf::SoundBuffer&      getSound(const std::string &fontName) const;
    const sf::Texture&          getTexture(const std::string& textureName) const;

    const Animation&            getAnimation(const std::string& name) const;

    const SpriteRec&            getSpriteRec(const std::string& name) const;
    const AnimationRec&         getAnimationRec(const std::string name) const;
};


#endif //BREAKOUT_ASSETS_H
