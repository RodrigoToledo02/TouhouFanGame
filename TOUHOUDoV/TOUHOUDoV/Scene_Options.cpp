#include "Scene_Options.h"
#include <fstream>
#include <iostream>
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <map>

static std::string keyToString(sf::Keyboard::Key key)
{
	switch (key) {
	case sf::Keyboard::A: return "A";
	case sf::Keyboard::B: return "B";
	case sf::Keyboard::C: return "C";
	case sf::Keyboard::D: return "D";
	case sf::Keyboard::E: return "E";
	case sf::Keyboard::F: return "F";
	case sf::Keyboard::G: return "G";
	case sf::Keyboard::H: return "H";
	case sf::Keyboard::I: return "I";
	case sf::Keyboard::J: return "J";
	case sf::Keyboard::K: return "K";
	case sf::Keyboard::L: return "L";
	case sf::Keyboard::M: return "M";
	case sf::Keyboard::N: return "N";
	case sf::Keyboard::O: return "O";
	case sf::Keyboard::P: return "P";
	case sf::Keyboard::Q: return "Q";
	case sf::Keyboard::R: return "R";
	case sf::Keyboard::S: return "S";
	case sf::Keyboard::T: return "T";
	case sf::Keyboard::U: return "U";
	case sf::Keyboard::V: return "V";
	case sf::Keyboard::W: return "W";
	case sf::Keyboard::X: return "X";
	case sf::Keyboard::Y: return "Y";
	case sf::Keyboard::Z: return "Z";
	case sf::Keyboard::Num0: return "0";
	case sf::Keyboard::Num1: return "1";
	case sf::Keyboard::Num2: return "2";
	case sf::Keyboard::Num3: return "3";
	case sf::Keyboard::Num4: return "4";
	case sf::Keyboard::Num5: return "5";
	case sf::Keyboard::Num6: return "6";
	case sf::Keyboard::Num7: return "7";
	case sf::Keyboard::Num8: return "8";
	case sf::Keyboard::Num9: return "9";
	case sf::Keyboard::Space: return "Space";
	case sf::Keyboard::Escape: return "Escape";
	case sf::Keyboard::Enter: return "Enter";
	case sf::Keyboard::Tab: return "Tab";
	case sf::Keyboard::Backspace: return "Backspace";
	case sf::Keyboard::Left: return "Left Arrow";
	case sf::Keyboard::Right: return "Right Arrow";
	case sf::Keyboard::Up: return "Up Arrow";
	case sf::Keyboard::Down: return "Down Arrow";
	case sf::Keyboard::LShift: return "Left Shift";
	case sf::Keyboard::RShift: return "Right Shift";
	case sf::Keyboard::LControl: return "Left Control";
	case sf::Keyboard::RControl: return "Right Control";
	case sf::Keyboard::LAlt: return "Left Alt";
	case sf::Keyboard::RAlt: return "Right Alt";
	case sf::Keyboard::F11: return "F11";
		// Add more cases as needed
	default: return "Unknown";
	}
}

static sf::Keyboard::Key stringToKey(const std::string& keyStr)
{
	if (keyStr == "A") return sf::Keyboard::A;
	if (keyStr == "B") return sf::Keyboard::B;
	if (keyStr == "C") return sf::Keyboard::C;
	if (keyStr == "D") return sf::Keyboard::D;
	if (keyStr == "E") return sf::Keyboard::E;
	if (keyStr == "F") return sf::Keyboard::F;
	if (keyStr == "G") return sf::Keyboard::G;
	if (keyStr == "H") return sf::Keyboard::H;
	if (keyStr == "I") return sf::Keyboard::I;
	if (keyStr == "J") return sf::Keyboard::J;
	if (keyStr == "K") return sf::Keyboard::K;
	if (keyStr == "L") return sf::Keyboard::L;
	if (keyStr == "M") return sf::Keyboard::M;
	if (keyStr == "N") return sf::Keyboard::N;
	if (keyStr == "O") return sf::Keyboard::O;
	if (keyStr == "P") return sf::Keyboard::P;
	if (keyStr == "Q") return sf::Keyboard::Q;
	if (keyStr == "R") return sf::Keyboard::R;
	if (keyStr == "S") return sf::Keyboard::S;
	if (keyStr == "T") return sf::Keyboard::T;
	if (keyStr == "U") return sf::Keyboard::U;
	if (keyStr == "V") return sf::Keyboard::V;
	if (keyStr == "W") return sf::Keyboard::W;
	if (keyStr == "X") return sf::Keyboard::X;
	if (keyStr == "Y") return sf::Keyboard::Y;
	if (keyStr == "Z") return sf::Keyboard::Z;
	if (keyStr == "0") return sf::Keyboard::Num0;
	if (keyStr == "1") return sf::Keyboard::Num1;
	if (keyStr == "2") return sf::Keyboard::Num2;
	if (keyStr == "3") return sf::Keyboard::Num3;
	if (keyStr == "4") return sf::Keyboard::Num4;
	if (keyStr == "5") return sf::Keyboard::Num5;
	if (keyStr == "6") return sf::Keyboard::Num6;
	if (keyStr == "7") return sf::Keyboard::Num7;
	if (keyStr == "8") return sf::Keyboard::Num8;
	if (keyStr == "9") return sf::Keyboard::Num9;
	if (keyStr == "Space") return sf::Keyboard::Space;
	if (keyStr == "Escape") return sf::Keyboard::Escape;
	if (keyStr == "Enter") return sf::Keyboard::Enter;
	if (keyStr == "Tab") return sf::Keyboard::Tab;
	if (keyStr == "Backspace") return sf::Keyboard::Backspace;
	if (keyStr == "Left Arrow") return sf::Keyboard::Left;
	if (keyStr == "Right Arrow") return sf::Keyboard::Right;
	if (keyStr == "Up Arrow") return sf::Keyboard::Up;
	if (keyStr == "Down Arrow") return sf::Keyboard::Down;
	if (keyStr == "Left Shift") return sf::Keyboard::LShift;
	if (keyStr == "Right Shift") return sf::Keyboard::RShift;
	if (keyStr == "Left Control") return sf::Keyboard::LControl;
	if (keyStr == "Right Control") return sf::Keyboard::RControl;
	if (keyStr == "Left Alt") return sf::Keyboard::LAlt;
	if (keyStr == "Right Alt") return sf::Keyboard::RAlt;
	if (keyStr == "F11") return sf::Keyboard::F11;
	// Add more checks as needed

	return sf::Keyboard::Unknown;  // Return Unknown if the string doesn't match any key
}

void Scene_Options::init()
{
	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::Up, "UP");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Down, "DOWN");
	registerAction(sf::Keyboard::Space, "SELECT");
	registerAction(sf::Keyboard::Escape, "QUIT");

	m_backOption = "Back";

	m_actionBindings = {
	{"LAUNCH", sf::Keyboard::X},
	{"GRAZE", sf::Keyboard::LShift},
	{"SWITCH", sf::Keyboard::C},
	{"LEFT", sf::Keyboard::A},
	{"RIGHT", sf::Keyboard::D},
	{"UP", sf::Keyboard::W},
	{"DOWN", sf::Keyboard::S},
	{"SELECT", sf::Keyboard::Space}
	};

	m_controlsText.setFont(Assets::getInstance().getFont("Venice"));
	m_controlsText.setCharacterSize(36);

	loadKeyBindings();
}

void Scene_Options::trim(std::string& str)
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

void Scene_Options::loadKeyBindings()
{
	std::ifstream file("../key.txt");

	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			// Split the line into action and key
			size_t colonPos = line.find(":");
			if (colonPos != std::string::npos) {
				std::string action = line.substr(0, colonPos);
				std::string keyStr = line.substr(colonPos + 1);

				// Trim any whitespace from the action and key
				trim(action);
				trim(keyStr);

				sf::Keyboard::Key key = stringToKey(keyStr);
				if (key != sf::Keyboard::Unknown) {
					m_actionBindings[action] = key;
				}
			}
		}
		std::cout << "Keybindings loaded from ../key.txt" << std::endl;

		// Rebuild the display strings after loading
		m_controls.clear();
		for (const auto& [action, key] : m_actionBindings) {
			m_controls.push_back(keyToString(key) + " - " + action);
		}
	}
	else {
		std::cerr << "Failed to open file for loading keybindings!" << std::endl;
	}
}

void Scene_Options::saveKeyBindings()
{
	std::ofstream file("../key.txt");

	if (file.is_open()) {
		// Write each action and keybinding to the file
		for (const auto& [action, key] : m_actionBindings) {
			file << action << ": " << keyToString(key) << std::endl;
		}
		std::cout << "Keybindings saved to ../key.txt" << std::endl;
	}
	else {
		std::cerr << "Failed to open file for saving keybindings!" << std::endl;
	}
}

void Scene_Options::onEnd()
{
	saveKeyBindings();
	_game->changeScene("MENU", nullptr, true);
}

Scene_Options::Scene_Options(GameEngine* gameEngine)
	: Scene(gameEngine)
{
	init();
}

void Scene_Options::update(sf::Time dt)
{
	if (m_waitingForKey) {
		for (int k = 0; k < sf::Keyboard::KeyCount; ++k) {
			if (sf::Keyboard::isKeyPressed((sf::Keyboard::Key)k)) {
				sf::Keyboard::Key newKey = static_cast<sf::Keyboard::Key>(k);

				// Avoid duplicate bindings
				bool isDuplicate = false;
				for (auto& [action, key] : m_actionBindings) {
					if (key == newKey) {
						isDuplicate = true;
						break;
					}
				}

				if (!isDuplicate) {
					// Show the user has pressed the key
					m_actionBindings[m_actionToRebind] = newKey;

					// Rebuild display strings
					m_controls.clear();
					for (const auto& [action, key] : m_actionBindings) {
						m_controls.push_back(keyToString(key) + " - " + action);
					}

					// Set waiting for key to false
					m_waitingForKey = false;
				}

				break;  // Exit the loop once a key is processed
			}
		}
	}
}

void Scene_Options::sRender()
{
	auto& window = _game->window();
	window.clear(sf::Color::Black);
	auto winSize = window.getSize();

	// Draw title
	sf::Text title;
	title.setFont(Assets::getInstance().getFont("Venice"));
	title.setCharacterSize(48);
	title.setString("Controls");
	title.setFillColor(sf::Color::Yellow);
	sf::FloatRect tb = title.getLocalBounds();
	title.setOrigin(tb.width / 2, tb.height / 2);
	title.setPosition(winSize.x / 2.f, 30.f);
	window.draw(title);

	const float menuStartY = 100.f;
	const float menuSpacing = 40.f;

	// Calculate visible range
	size_t totalItems = m_controls.size() + 1; // +1 for Back
	size_t endIndex = std::min(m_scrollOffset + m_maxVisibleItems, totalItems);

	for (size_t i = m_scrollOffset; i < endIndex; ++i) {
		std::string text = (i == m_controls.size()) ? m_backOption : m_controls[i];

		// Highlight selected item
		if (i == m_menuIndex) {
			m_controlsText.setFillColor(sf::Color(200, 200, 100));
		}
		else {
			m_controlsText.setFillColor(sf::Color::White);
		}

		m_controlsText.setString(text);
		sf::FloatRect bounds = m_controlsText.getLocalBounds();
		m_controlsText.setOrigin(bounds.width / 2, bounds.height / 2);
		m_controlsText.setPosition(winSize.x / 2.f, menuStartY + (i - m_scrollOffset) * menuSpacing);

		window.draw(m_controlsText);
	}
}

void Scene_Options::sDoAction(const Command& action)
{
	if (action.type() == "START") {
		if (action.name() == "UP") {
			m_menuIndex = (m_menuIndex + m_controls.size()) % (m_controls.size() + 1);
		}
		else if (action.name() == "DOWN") {
			m_menuIndex = (m_menuIndex + 1) % (m_controls.size() + 1);
		}
		else if (action.name() == "SELECT") {
			if (m_menuIndex == m_controls.size()) {
				onEnd();
			}
			else {
				// Begin rebinding for selected action
				auto it = std::next(m_actionBindings.begin(), m_menuIndex);
				m_actionToRebind = it->first;
				m_waitingForKey = true;
			}
		}
	}

	if (m_menuIndex < m_scrollOffset) {
		m_scrollOffset = m_menuIndex;
	}
	else if (m_menuIndex >= m_scrollOffset + m_maxVisibleItems) {
		m_scrollOffset = m_menuIndex - m_maxVisibleItems + 1;
	}
}