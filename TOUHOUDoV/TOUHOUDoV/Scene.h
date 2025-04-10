#pragma once

#include "EntityManager.h"
#include "GameEngine.h"
#include "Command.h"
#include <map>
#include <string>

using CommandMap = std::map<int, std::string>;

class Scene_Breakout;

class Scene
{
protected:

	GameEngine* _game;
	EntityManager	_entityManager;
	CommandMap		_commands;
	bool			_isPaused{ false };
	bool			_hasEnded{ false };
	size_t			_currentFrame{ 0 };

	virtual void	onEnd() = 0;
	void			setPaused(bool paused);

public:
	explicit Scene(GameEngine* gameEngine);
	virtual ~Scene();

	virtual void		update(sf::Time dt) = 0;
	virtual void		sDoAction(const Command& action) = 0;
	virtual void		sRender() = 0;

	void				simulate(int);
	void				doAction(Command);
	void				registerAction(int, std::string);
	const CommandMap	getActionMap() const;
};
