#pragma once

#include "Scene.h"
#include <unordered_map>

class Scene_Options : public Scene {
private:
	void saveKeyBindings();
	void loadKeyBindings();
	void init();
	void trim(std::string& str);
	void onEnd() override;

	std::vector<std::string> m_controls;   // add to Scene_Options.h
	sf::Text m_controlsText;              // for rendering
	std::string m_backOption;
	size_t m_menuIndex{ 0 };

	size_t m_scrollOffset{ 0 };
	const size_t m_maxVisibleItems{ 12 }; // Adjust based on screen size and font

	std::unordered_map<std::string, sf::Keyboard::Key> m_actionBindings;
	bool m_waitingForKey = false;
	std::string m_actionToRebind;

public:
	explicit Scene_Options(GameEngine* gameEngine);

	void update(sf::Time dt) override;
	void sRender() override;
	void sDoAction(const Command& action) override;
};
