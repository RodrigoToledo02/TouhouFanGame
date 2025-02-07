#pragma once

#include "Assets.h"

#include <memory>
#include <map>

class Scene;

using SceneMap = std::map<std::string, std::shared_ptr<Scene>>;

class GameEngine
{
public:
	sf::RenderWindow	        _window;
	std::string			        _currentScene;
	SceneMap			        _sceneMap;
	size_t				        _simulationSpeed{ 1 };
	bool				        _running{ true };

	// stats
	sf::Text					_statisticsText;
	sf::Time					_statisticsUpdateTime{ sf::Time::Zero };
	unsigned int				_statisticsNumFrames{ 0 };

public:
	void					init(const std::string& path);
	void					update();
	void					sUserInput();
	std::shared_ptr<Scene>	currentScene();

public:

	GameEngine(const std::string& path);
	;
	void				changeScene(const std::string& sceneName,
		std::shared_ptr<Scene> scene,
		bool endCurrentScene = false);
	void				quit();
	void				run();
	void				quitLevel();
	void				backLevel();
	sf::RenderWindow& window();
	sf::Vector2f		windowSize() const;
	bool				isRunning();
	void				loadConfigFromFile(const std::string& path,
		unsigned int& width, unsigned int& height) const;
};
