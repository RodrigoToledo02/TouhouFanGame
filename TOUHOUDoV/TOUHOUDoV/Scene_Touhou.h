#pragma once

#include "Components.h"
#include "Scene.h"
#include <queue>

struct SpawnPoint {
	std::string     type;
	float           y;
	auto operator<=>(const SpawnPoint& other) const {
		return  y <=> other.y;
	}
};

struct LevelConfig {
	float       scrollSpeed{ 100.f };
	float       playerSpeed{ 200.f };
	float       enemySpeed{ 200.f };
	float       bulletSpeed{ 400.f };
	float       missileSpeed{ 150.f };
	sf::Time    fireInterval{ sf::seconds(5) };
};

class Scene_Touhou : public Scene
{
	sPtrEntt                            _player{ nullptr };
	sf::View                            _worldView; // camera
	sf::FloatRect                       _worldBounds;
	LevelConfig                         _config;
	std::priority_queue<SpawnPoint>     _spawnPoints;
	bool                                _drawTextures{ true };
	bool                                _drawAABB{ false };
	bool                                _drawCam{ false };

	//systems
	void                    sMovement(sf::Time dt);
	void                    sSpawnEnemies();
	void                    sCollisions();
	void                    sUpdate(sf::Time dt);
	void                    sGunUpdate(sf::Time dt);
	void                    sAutoPilot(sf::Time dt);
	void                    sGuideMissiles(sf::Time dt);
	void                    sAnimation(sf::Time dt);

	// helper functions
	bool                    isGameOver();
	void                    dropPickup(sf::Vector2f pos);
	void                    startAnimation(sPtrEntt e, std::string animation);
	void                    checkIfDead(sPtrEntt e);
	void                    checkMissileCollision();
	void                    checkBulletCollision();
	void                    checkPlaneCollision();
	void                    checkPickupCollision();
	sf::Vector2f            findClosestEnemy(sf::Vector2f mPos);
	sf::FloatRect           getBattlefieldBounds() const;
	void                    destroyOutsideBattlefieldBounds();
	void                    spawnEnemyPlanes(SpawnPoint sp);

	void                    spawnBullet(sf::Vector2f pos, bool isEnemy);
	void	                registerActions();
	void                    spawnPlayer(sf::Vector2f pos);
	void                    playerMovement();
	void                    animatePlayer();
	void                    adjustPlayerPosition();
	void                    init(const std::string& path);
	void                    loadLevel(const std::string& path);
	void                    spawnMisille();
	void	                onEnd() override;
	void                    drawAABB(std::shared_ptr<Entity> e);
	void                    drawCameraView();
	void                    drawHP(sPtrEntt e);
	void                    drawAmmo(sPtrEntt e);
	void                    drawEntt(sPtrEntt e);

public:
	Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath);
	void		            update(sf::Time dt) override;
	void		            sDoAction(const Command& command) override;
	void		            sRender() override;

	void                    fireBullet();
};
