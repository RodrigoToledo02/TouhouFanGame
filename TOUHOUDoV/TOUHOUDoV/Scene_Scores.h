#pragma once

#include "Scene.h"

class Scene_Scores : public Scene {
private:
	void init();
	void loadScores();
	void onEnd() override;

	std::vector<std::string> m_scores;  // To store the score data
	std::string m_backOption;           // "Back" option for the menu
	int m_menuIndex;                   // Index of the selected menu option
	sf::Text m_menuText;                // Text for displaying options and scores
public:
	explicit Scene_Scores(GameEngine* gameEngine);

	void update(sf::Time dt) override;
	void sRender() override;
	void sDoAction(const Command& action) override;
};
