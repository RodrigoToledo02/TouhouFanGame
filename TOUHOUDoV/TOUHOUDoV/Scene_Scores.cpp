#include "Scene_Scores.h"
#include "Assets.h"
#include "MusicPlayer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Scene_Menu.h"

Scene_Scores::Scene_Scores(GameEngine* gameEngine)
	: Scene(gameEngine), m_menuIndex(0)
{
	init();
}

void Scene_Scores::init()
{
	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::Up, "UP");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Down, "DOWN");
	registerAction(sf::Keyboard::Space, "SELECT");
	registerAction(sf::Keyboard::Escape, "QUIT");
	// Load scores from file
	loadScores();

	// "Back" option
	m_backOption = "Press SPACE to go back";

	// Set up the font for displaying text
	m_menuText.setFont(Assets::getInstance().getFont("Venice"));
	m_menuText.setCharacterSize(48);
}

void Scene_Scores::loadScores()
{
	// Open the file and read the scores
	std::ifstream inFile("../score.txt");
	std::string line;

	if (inFile.is_open()) {
		while (std::getline(inFile, line)) {
			m_scores.push_back(line);  // Add each line (score) to the vector
		}
		inFile.close();
	}
	else {
		std::cerr << "Failed to open score.txt" << std::endl;
	}
}

void Scene_Scores::onEnd()
{
}

void Scene_Scores::update(sf::Time dt)
{
	_entityManager.update();
}

void Scene_Scores::sRender()
{
	sf::Vector2u windowSize = _game->window().getSize();
	_game->window().clear(sf::Color(0, 0, 0));

	// Render scores
	float menuStartY = windowSize.y * 0.2f;
	const float menuSpacing = windowSize.y * 0.1f;

	for (size_t i = 0; i < m_scores.size(); ++i) {
		m_menuText.setFillColor(sf::Color::White);
		m_menuText.setString(m_scores[i]);

		sf::FloatRect textBounds = m_menuText.getLocalBounds();
		m_menuText.setOrigin(textBounds.width / 2, textBounds.height / 2);
		m_menuText.setPosition(windowSize.x / 2.f, menuStartY + (i * menuSpacing));

		_game->window().draw(m_menuText);
	}

	// Render "Back" option
	m_menuText.setString(m_backOption);
	m_menuText.setFillColor((m_menuIndex == m_scores.size() ? sf::Color(150, 150, 150) : sf::Color::White));

	sf::FloatRect textBounds = m_menuText.getLocalBounds();
	m_menuText.setOrigin(textBounds.width / 2, textBounds.height / 2);
	m_menuText.setPosition(windowSize.x / 2.f, menuStartY + (m_scores.size() * menuSpacing));

	_game->window().draw(m_menuText);
}

void Scene_Scores::sDoAction(const Command& action)
{
	if (action.type() == "START")
	{
		if (action.name() == "UP") {
			//In case something gets added
		}
		else if (action.name() == "DOWN") {
			//In case something gets added
		}
		else if (action.name() == "SELECT") {
			if (m_menuIndex == 0) {  // Back option
				_game->changeScene("MENU", nullptr, true);
			}
		}
	}
}