#include "Scene.h"


Scene::Scene(GameEngine* gameEngine) : _game(gameEngine)
{}

Scene::~Scene()
{}

void Scene::setPaused(bool paused)
{
    _isPaused = paused;
}


void Scene::simulate(int)
{}

void Scene::doAction(Command command)
{
	this->sDoAction(command);
}


const CommandMap Scene::getActionMap() const
{
	return _commands;
}

void Scene::registerAction(int inputKey, std::string command)
{
	_commands[inputKey] = command;
}
