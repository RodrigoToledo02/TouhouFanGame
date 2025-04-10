#pragma once

#include "Scene.h"

class Scene_Menu : public Scene
{
private:
	std::vector<std::string>	m_menuStrings;
	sf::Text					m_menuText;
	std::vector<std::string>	m_levelPaths;
	int							m_menuIndex{ 0 };
	std::string					m_title;

	void init();
	void onEnd() override;
public:

	explicit Scene_Menu(GameEngine* gameEngine);

	void update(sf::Time dt) override;
	void trim(std::string& str);
	void updateView(const sf::Vector2u& size);

	void sRender() override;
	void sDoAction(const Command& action) override;
};
