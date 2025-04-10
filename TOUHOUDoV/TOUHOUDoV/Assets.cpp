//
// Created by David Burchill on 2023-10-31.
//

#include "Assets.h"
#include "MusicPlayer.h"
#include "json.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

Assets::Assets()
{
}

Assets& Assets::getInstance() {
	static Assets instance;          // Meyers Singleton implementation
	return instance;
}

void Assets::addFont(const std::string& fontName, const std::string& path) {
	std::unique_ptr<sf::Font> font(new sf::Font);
	if (!font->loadFromFile(path))
		throw std::runtime_error("Load failed - " + path);

	auto rc = _fontMap.insert(std::make_pair(fontName, std::move(font)));
	if (!rc.second) assert(0); // big problems if insert fails

	std::cout << "Loaded font: " << path << std::endl;
}

void Assets::addSound(const std::string& soundName, const std::string& path) {
	std::unique_ptr<sf::SoundBuffer> sb(new sf::SoundBuffer);
	if (!sb->loadFromFile(path))
		throw std::runtime_error("Load failed - " + path);

	auto rc = _soundEffects.insert(std::make_pair(soundName, std::move(sb)));
	if (!rc.second) assert(0); // big problems if insert fails

	std::cout << "Loaded sound effect: " << path << std::endl;
}

void Assets::addTexture(const std::string& textureName, const std::string& path, bool smooth)
{
	auto& texture = _textures[textureName];
	if (!texture.loadFromFile(path)) {
		std::cerr << "Could not load texture: " << path << std::endl;
		_textures.erase(textureName);
		return;
	}
	texture.setSmooth(smooth);
}

const sf::Font& Assets::getFont(const std::string& fontName) const {
	auto found = _fontMap.find(fontName);
	assert(found != _fontMap.end());
	return *found->second;
}

const sf::SoundBuffer& Assets::getSound(const std::string& soundName) const {
	auto found = _soundEffects.find(soundName);
	assert(found != _soundEffects.end());
	return *found->second;
}

const sf::Texture& Assets::getTexture(const std::string& textureName) const
{
	return _textures.at(textureName);
}

void Assets::loadFonts(const std::string& path)
{
	std::ifstream confFile(path);
	if (!fs::exists(path)) {
		std::cerr << "File does not exist: " << path << '\n';
		return;
	}
	if (!confFile)
		throw std::runtime_error("Failed to open file: " + path);

	std::string token{ "" };
	confFile >> token;
	while (confFile) {
		if (token == "Font") {
			std::string name, path;
			confFile >> name >> path;
			addFont(name, path);
		}
		else {
			// ignore rest of line and continue
			std::string buffer;
			std::getline(confFile, buffer);
		}
		confFile >> token;
	}
	confFile.close();
}

void Assets::loadTextures(const std::string& path)
{
	// Read Config file
	if (!fs::exists(path)) {
		std::cerr << "File does not exist: " << path << '\n';
		return;
	}

	std::ifstream confFile(path);
	if (!confFile)
		throw std::runtime_error("Failed to open file: " + path);

	std::string token{ "" };
	confFile >> token;
	while (confFile) {
		if (token == "Texture") {
			std::string name;
			std::string path;
			confFile >> name >> path;
			addTexture(name, path);
		}
		else {
			// ignore rest of line and continue
			std::string buffer;
			std::getline(confFile, buffer);
		}
		confFile >> token;
	}
	confFile.close();
}

void Assets::loadSounds(const std::string& path)
{
	std::ifstream confFile(path);
	if (!fs::exists(path)) {
		std::cerr << "File does not exist: " << path << '\n';
		return;
	}
	if (!confFile)
		throw std::runtime_error("Failed to open file: " + path);

	std::string token{ "" };
	confFile >> token;
	while (confFile) {
		if (token == "Sound") {
			std::string name;
			std::string path;
			confFile >> name >> path;
			addSound(name, path);
		}
		else {
			// ignore rest of line and continue
			std::string buffer;
			std::getline(confFile, buffer);
		}
		confFile >> token;
	}
	confFile.close();
}

void Assets::loadJson(const std::string& path) {
	// Read Config file
	std::ifstream confFile(path);
	if (!confFile)
		throw std::runtime_error("Failed to open file: " + path);

	std::string token{ "" };
	confFile >> token;
	while (confFile)
	{
		if (token == "JSON")
		{
			using json = nlohmann::json;
			std::string  path;
			confFile >> path;

			// read the FrameSets from the json file
			std::ifstream f(path);
			json data = json::parse(f)["frames"];
			for (auto i : data) {
				// clean up animation name
				std::string tmp = i["filename"];
				std::string::size_type n = tmp.find(" (");
				if (n == std::string::npos)
					n = tmp.find(".png");

				// create IntRect for each frame in animation
				auto ir = sf::IntRect(i["frame"]["x"], i["frame"]["y"],
					i["frame"]["w"], i["frame"]["h"]);

				_frameSets[tmp.substr(0, n)].push_back(ir);
			}
			f.close();
		}
		else
		{
			// ignore rest of line and continue
			std::string buffer;
			std::getline(confFile, buffer);
		}
		confFile >> token;
	}
	confFile.close();
}

void Assets::loadAnimations(const std::string& path) {
	// Read Config file
	std::ifstream confFile(path);
	if (!fs::exists(path)) {
		std::cerr << "File does not exist: " << path << '\n';
		return;
	}
	if (!confFile)
		throw std::runtime_error("Failed to open file: " + path);

	std::string token{ "" };
	confFile >> token;
	while (confFile)
	{
		if (token == "Animation")
		{
			std::string name;
			std::string texture, repeat;
			float speed;
			confFile >> name >> texture >> speed >> repeat;

			Animation a(name,
				getTexture(texture),
				_frameSets[name],
				sf::seconds(1 / speed),
				(repeat == "yes"));

			_animationMap[name] = a;
		}
		else
		{
			// ignore rest of line and continue
			std::string buffer;
			std::getline(confFile, buffer);
		}
		confFile >> token;
	}
	confFile.close();
}

const Animation& Assets::getAnimation(const std::string& name) const {
	return _animationMap.at(name);
}

void Assets::loadFromFile(const std::string path) {
	loadFonts(path);
	loadTextures(path);
	loadSounds(path);

	loadJson(path);
	loadAnimations(path);  // requires  _framesets must come after loadJson
}