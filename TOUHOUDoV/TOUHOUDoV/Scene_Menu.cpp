#include "Scene_Menu.h"
#include "Scene_Touhou.h"
#include "MusicPlayer.h"
#include <memory>
#include "Scene_Options.h"
#include "Scene_Scores.h"

void Scene_Menu::onEnd()
{
	_game->window().close();
}

Scene_Menu::Scene_Menu(GameEngine* gameEngine)
	: Scene(gameEngine)
{
	init();
}

void Scene_Menu::init()
{
	MusicPlayer::getInstance().play("menuTheme");
	MusicPlayer::getInstance().setVolume(5);

	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::Up, "UP");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Down, "DOWN");
	registerAction(sf::Keyboard::Space, "PLAY");
	registerAction(sf::Keyboard::Escape, "QUIT");
	registerAction(sf::Keyboard::F11, "TOGGLE_VIEW_MODE");

	m_title = "TOUHOU: Darkness of the Void";
	m_menuStrings.emplace_back("Start");
	m_menuStrings.emplace_back("Controls");
	m_menuStrings.emplace_back("High-Score");
	m_menuStrings.emplace_back("Quit");

	m_levelPaths.emplace_back("../level1.txt");
	//m_levelPaths.emplace_back("../options.txt");
	//m_levelPaths.emplace_back(2);

	m_menuText.setFont(Assets::getInstance().getFont("Venice"));

	const size_t CHAR_SIZE{ 64 };
	m_menuText.setCharacterSize(CHAR_SIZE);
}

void Scene_Menu::updateView(const sf::Vector2u& size) {
	sf::View view = _game->window().getView();
	view.setSize(static_cast<float>(size.x), static_cast<float>(size.y));
	view.setCenter(size.x / 2.f, size.y / 2.f);
	_game->window().setView(view);
}

void Scene_Menu::update(sf::Time dt)
{
	_entityManager.update();
}

void Scene_Menu::trim(std::string& str)
{
	size_t first = str.find_first_not_of(" \t\n\r\f\v");
	if (first == std::string::npos) {
		str.clear();
	}
	else {
		size_t last = str.find_last_not_of(" \t\n\r\f\v");
		str = str.substr(first, last - first + 1);
	}
}

void Scene_Menu::sRender()
{
	sf::View view = _game->window().getView();
	sf::Vector2u windowSize = _game->window().getSize();
	view.setCenter(windowSize.x / 2.f, windowSize.y / 2.f);
	_game->window().setView(view);

	_game->window().clear(sf::Color(0, 0, 0));

	static const sf::Color selectedColor(sf::Color::Yellow);
	static const sf::Color normalColor(255, 255, 255);

	const unsigned int minFontSize = 32;
	const unsigned int maxFontSize = 64;

	unsigned int fontSize = std::max(minFontSize, std::min(maxFontSize, windowSize.y / 15));
	m_menuText.setCharacterSize(fontSize);

	static const sf::Color backgroundColor(0, 0, 0);

	m_menuText.setFillColor(normalColor);
	m_menuText.setString(m_title);
	sf::FloatRect titleBounds = m_menuText.getLocalBounds();
	m_menuText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
	m_menuText.setPosition(windowSize.x / 2.f, windowSize.y * 0.15f);

	float maxTitleWidth = windowSize.x * 0.8f;
	if (titleBounds.width > maxTitleWidth)
	{
		float scale = maxTitleWidth / titleBounds.width;
		m_menuText.setScale(scale, scale);
	}
	else
	{
		m_menuText.setScale(1.f, 1.f);
	}
	_game->window().draw(m_menuText);

	const float menuStartY = windowSize.y * 0.3f;
	const float menuSpacing = windowSize.y * 0.1f;

	for (size_t i{ 0 }; i < m_menuStrings.size(); ++i)
	{
		m_menuText.setFillColor((i == m_menuIndex ? selectedColor : normalColor));
		m_menuText.setString(m_menuStrings.at(i));

		sf::FloatRect textBounds = m_menuText.getLocalBounds();
		m_menuText.setOrigin(textBounds.width / 2, textBounds.height / 2);
		m_menuText.setPosition(windowSize.x / 2.f, menuStartY + (i * menuSpacing));

		if (textBounds.width > windowSize.x * 0.8f)
		{
			float scale = (windowSize.x * 0.8f) / textBounds.width;
			m_menuText.setScale(scale, scale);
		}
		else
		{
			m_menuText.setScale(1.f, 1.f);
		}

		_game->window().draw(m_menuText);

		sf::Text hintText;
		hintText.setFont(Assets::getInstance().getFont("Venice"));
		hintText.setCharacterSize(windowSize.y / 35); // smaller size
		hintText.setFillColor(sf::Color(200, 200, 200)); // light grey
		hintText.setString("SPACE to select");

		sf::FloatRect hintBounds = hintText.getLocalBounds();
		hintText.setOrigin(0.f, 0.f); // top-left origin
		hintText.setPosition(10.f, windowSize.y - hintBounds.height - 30.f); // small padding from bottom

		_game->window().draw(hintText);
	}
}

void Scene_Menu::sDoAction(const Command& action)
{
	if (action.type() == "START")
	{
		if (action.name() == "UP")
		{
			m_menuIndex = (m_menuIndex + m_menuStrings.size() - 1) % m_menuStrings.size();
		}
		else if (action.name() == "DOWN")
		{
			m_menuIndex = (m_menuIndex + 1) % m_menuStrings.size();
		}
		else if (action.name() == "PLAY")
		{
			if (m_menuIndex == 0) // Start option
			{
				_game->changeScene("PLAY", std::make_shared<Scene_Touhou>(_game, m_levelPaths[m_menuIndex]));
			}
			else if (m_menuIndex == 1) // Quit option
			{
				_game->changeScene("OPTIONS", std::make_shared<Scene_Options>(_game));
			}
			else if (m_menuIndex == 2) {  // Scores option
				_game->changeScene("SCORES", std::make_shared<Scene_Scores>(_game));
			}
			else if (m_menuIndex == 3) // Quit option
			{
				onEnd();
			}
		}
		else if (action.name() == "QUIT")
		{
			onEnd();
		}
		else if (action.name() == "TOGGLE_VIEW_MODE")
		{
			_game->toggleViewMode();
		}
	}
}