#include "GameEngine.h"
#include "Assets.h"
#include "Scene_Menu.h"
#include "Scene_Touhou.h"
#include "Command.h"
#include <fstream>
#include <memory>
#include <cstdlib>
#include <iostream>

GameEngine::GameEngine(const std::string& path)
{
	Assets::getInstance().loadFromFile("../config.txt");
	init(path);
}

void GameEngine::toggleViewMode() {
	if (_viewMode == ViewMode::Windowed) {
		setViewMode(ViewMode::Fullscreen);
	}
	else if (_viewMode == ViewMode::Fullscreen) {
		setViewMode(ViewMode::WindowedFullscreen);
	}
	else {
		setViewMode(ViewMode::Windowed);
	}
}

void GameEngine::setViewMode(ViewMode mode) {
	_viewMode = mode;
	sf::Uint32 style;
	sf::Vector2u size;

	if (mode == ViewMode::Windowed) {
		style = sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
		size = { 800, 600 }; // Example size, adjust as needed
	}
	else if (mode == ViewMode::Fullscreen) {
		style = sf::Style::Fullscreen;
		size = { sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height };
	}
	else if (mode == ViewMode::WindowedFullscreen) {
		style = sf::Style::None;
		size = { sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height };
	}

	_window.create(sf::VideoMode(size.x, size.y), "Touhou: Darkness of the Void", style);
	_window.setView(sf::View(sf::FloatRect(0, 0, size.x, size.y)));

	auto menuScene = std::dynamic_pointer_cast<Scene_Menu>(currentScene());
	if (menuScene) {
		menuScene->updateView(size);
	}
	else {
		auto scene = std::dynamic_pointer_cast<Scene_Touhou>(currentScene());
		if (scene) {
			scene->updateView(size);
		}
		else {
			std::cerr << "Error: Current scene is not of type Scene_Touhou or Scene_Menu or is nullptr." << std::endl;
		}
	}
}

void GameEngine::init(const std::string& path) {
	unsigned int width;
	unsigned int height;
	loadConfigFromFile(path, width, height);
	setViewMode(ViewMode::WindowedFullscreen);

	_statisticsText.setFont(Assets::getInstance().getFont("main"));
	_statisticsText.setPosition(15.0f, 5.0f);
	_statisticsText.setCharacterSize(15);

	changeScene("MENU", std::make_shared<Scene_Menu>(this));
	//changeScene("TOUHOU", std::make_shared<Scene_Touhou>(this, "../level1.txt"));
}

//_window.create(sf::VideoMode(width, height), "Touhou: Darkness of the Void");
//_window.create(sf::VideoMode(width, height), "Touhou: Darkness of the Void", sf::Style::Fullscreen);

void GameEngine::loadConfigFromFile(const std::string& path, unsigned int& width, unsigned int& height) const {
	std::ifstream config(path);
	if (config.fail()) {
		std::cerr << "Open file " << path << " failed\n";
		config.close();
		exit(1);
	}
	std::string token{ "" };
	config >> token;
	while (!config.eof()) {
		if (token == "Window") {
			config >> width >> height;
		}
		else if (token[0] == '#') {
			std::string tmp;
			std::getline(config, tmp);
			std::cout << tmp << "\n";
		}

		if (config.fail()) {
			config.clear(); // clear error on stream
			std::cout << "*** Error reading config file\n";
		}
		config >> token;
	}
	config.close();
}

void GameEngine::update()
{
	//sUserInput();
	//currentScene()->update();
}

void GameEngine::sUserInput()
{
	sf::Event event;
	while (_window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			quit();

		if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
		{
			if (currentScene()->getActionMap().contains(event.key.code))
			{
				const std::string actionType = (event.type == sf::Event::KeyPressed) ? "START" : "END";
				currentScene()->doAction(Command(currentScene()->getActionMap().at(event.key.code), actionType));
			}
		}
	}
}

void GameEngine::realTimeInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
	{
		if (auto scene = std::dynamic_pointer_cast<Scene_Touhou>(currentScene())) {
			scene->fireBullet();
		}
	}
}

std::shared_ptr<Scene> GameEngine::currentScene()
{
	if (_sceneMap.find(_currentScene) == _sceneMap.end()) {
		std::cerr << "Error: Current scene '" << _currentScene << "' not found in scene map." << std::endl;
		return nullptr;
	}
	return _sceneMap.at(_currentScene);
}

void GameEngine::changeScene(const std::string& sceneName, std::shared_ptr<Scene> scene, bool endCurrentScene)
{
	if (endCurrentScene) {
		_sceneMap.erase(_currentScene);
	}

	if (!_sceneMap.contains(sceneName)) {
		_sceneMap[sceneName] = scene;
		std::cout << "Scene '" << sceneName << "' added to scene map." << std::endl;
	}

	_currentScene = sceneName;
	std::cout << "Current scene set to '" << _currentScene << "'." << std::endl;
}

void GameEngine::quit()
{
	_window.close();
}

void GameEngine::run()
{
	const sf::Time SPF = sf::seconds(1.0f / 60.f);  // seconds per frame for 60 fps

	sf::Clock clock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;

	while (isRunning())
	{
		sUserInput();								// get user input
		realTimeInput();

		timeSinceLastUpdate += clock.restart();
		while (timeSinceLastUpdate > SPF)
		{
			currentScene()->update(SPF);			// update world
			timeSinceLastUpdate -= SPF;
		}

		window().clear(sf::Color::Cyan);
		currentScene()->sRender();					// render world
		window().display();
	}
}

void GameEngine::quitLevel()
{
	changeScene("MENU", nullptr, true);
}

void GameEngine::backLevel()
{
	changeScene("MENU", nullptr, false);
}

sf::RenderWindow& GameEngine::window()
{
	return _window;
}

sf::Vector2f GameEngine::windowSize() const {
	return sf::Vector2f{ _window.getSize() };
}

bool GameEngine::isRunning() const
{
	return (_running && _window.isOpen());
}