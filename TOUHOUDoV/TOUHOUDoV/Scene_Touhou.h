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

struct TemporaryText {
	sf::Text text;
	sf::Time lifetime;
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
	bool								backgroundToggle{ true };
	sf::Time							_backgroundSwitchCooldown{ sf::Time::Zero };
	bool								_isPaused{ false };
	int									_pauseMenuIndex{ 0 };
	sf::Text							_pauseMenuText;
	std::vector<std::string>			_pauseMenuOptions{ "Continue", "Quit" };
	bool								bulletsMoving{ true };
	sf::Time							bulletMovementTimer{ sf::seconds(0) };
	sf::Vector2f						bossTargetPosition;
	bool								isBossMovingToTarget = false;
	sf::CircleShape						m_expandingCircle;
	bool								m_isExpandingCircleActive{ false };
	float								m_expandingCircleSpeed{ 300.f };
	int									_score{ 0 };
	std::queue<int>						_lastSpreadLevels;
	int _lastScoreThreshold = 0;

	//UI
	sf::Text							_scoreText;
	sf::Text							_livesText;
	sf::Text							_spellCardsText;
	sf::Text							_parryText;
	sf::Text							_backgroundCooldownText;
	std::vector<TemporaryText>			_temporaryTexts;
	//const sf::Texture& _whiteHeartTexture;

	void					moveEnemyBullets(sf::Time dt);

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
	void					despawnAllBullets();
	bool                    isGameOver();
	void                    dropPickup(sf::Vector2f pos);
	void                    startAnimation(sPtrEntt e, std::string animation);
	void                    checkIfDead(sPtrEntt e);
	void					resetGameState();

	void                    checkSpellCardCollision();
	void                    checkBulletCollision();
	void                    checkPlaneCollision();
	void                    checkPickupCollision();
	sf::Vector2f            findClosestEnemy(sf::Vector2f mPos);
	sf::FloatRect           getBattlefieldBounds() const;
	void                    destroyOutsideBattlefieldBounds();
	void                    spawnEnemyPlanes(SpawnPoint sp);
	void					spawnBoss(SpawnPoint sp);

	void                    spawnBullet(sf::Vector2f pos, bool isEnemy, const std::string& spriteName);
	void	                registerActions();
	void                    spawnPlayer(sf::Vector2f pos);
	void                    playerMovement();
	void                    animatePlayer();
	void                    adjustPlayerPosition();
	void                    init(const std::string& path);
	void                    loadLevel(const std::string& path);
	void                    spawnSpellCard();
	void	                onEnd() override;
	void                    drawAABB(std::shared_ptr<Entity> e);
	void                    drawCameraView();

	void					changeBackground();
	void                    drawHP(sPtrEntt e);
	void                    drawAmmo(sPtrEntt e);
	void                    drawEntt(sPtrEntt e);
	void					playerSize(bool smaller);

public:
	Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath);
	void		            update(sf::Time dt) override;
	void					updateView(const sf::Vector2u& size);
	void					drawPauseOverlay();
	void		            sDoAction(const Command& command) override;

	void		            sRender() override;

	void                    fireBullet();
	void					restartGame(const std::string& path);
};
