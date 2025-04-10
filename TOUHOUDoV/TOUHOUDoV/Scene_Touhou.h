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

struct ViewBounds {
	float left;
	float right;
	float top;
	float bot;
};

struct UIText {
	//UI
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
	bool								_endGame{ false };
	int									_pauseMenuIndex{ 0 };
	sf::Text							_pauseMenuText;
	std::vector<std::string>			_pauseMenuOptions{ "Continue", "Quit" };
	int									_endScreenIndex{ 0 };
	sf::Text							_endScreenText;
	std::vector<std::string>			_endScreenOptions{ "Continue", "Finish" };
	bool								bulletsMoving{ true };
	sf::Time							bulletMovementTimer{ sf::seconds(0) };
	sf::Vector2f						bossTargetPosition;
	bool								isBossMovingToTarget = false;
	sf::CircleShape						m_expandingCircle;
	bool								m_isExpandingCircleActive{ false };
	float								m_expandingCircleSpeed{ 300.f };
	int									_score{ 0 };
	int									lastSpreadLevel;
	int									_lastScoreThreshold = 0;
	bool								bossHasPassed1200{ false };
	int									bulletIndex = 0;
	int									currentColumn = 0;
	sf::Time							bulletSpawnTimer = sf::Time::Zero;
	sf::Time							columnSpawnTimer = sf::Time::Zero;

	//UI
	sf::Text							_scoreText;
	sf::Text							_livesText;
	sf::Text							_spellCardsText;
	sf::Text							_parryText;
	sf::Text							_backgroundCooldownText;
	std::vector<UIText>					_temporaryTexts;

	//systems
	void                    sMovement(sf::Time dt);
	void                    sSpawnEnemies();

	void                    sCollisions();
	void                    sUpdate(sf::Time dt);
	void                    sGunUpdate(sf::Time dt);
	void					updateBossSpreadLevel(CGun& gun, int bossCurrentHP);
	void                    sAutoPilot(sf::Time dt);
	void                    sAnimation(sf::Time dt);

	// Movements

	void                    movementBoss(sf::Time dt);

	void					movementEnemyBullet(sf::Time dt);

	// helper functions
	void					despawnAllBullets();
	void                    checkIfDead(sPtrEntt e);
	void					resetGameState();

	void startGame();

	void                    checkSpellCardCollision();
	void                    checkBulletCollision();
	sf::FloatRect           getBattlefieldBounds() const;
	void                    destroyOutsideBattlefieldBounds();
	void					spawnBoss(SpawnPoint sp);

	void handleFiring(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& bulletTexture, const std::string& lineTexture, const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread0(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& bulletTexture, const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread1(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread2(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& lineTexture);

	void fireSpread3(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);

	void fireSpread4(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);

	void fireSpread5(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);

	void                    spawnBullet(sf::Vector2f pos, bool isEnemy, const std::string& spriteName);
	void trim(std::string& str);
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
	void                    drawHP(sPtrEntt e);
	void                    drawAmmo(sPtrEntt e);
	void                    drawEntt(sPtrEntt e);
	void					playerSize(bool smaller);
	void drawUI(float uiX, float uiY);
	void drawHealthHearts(float uiX, float uiY);
	void drawSpellCards();
	void drawPickups();
	void drawBullets();
	void drawBossHealthBar();
	void drawBossEntities();
	void drawEntities();
	void drawBackground();

	ViewBounds getViewBounds() const {
		sf::Vector2f center = _worldView.getCenter();
		sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

		return {
			center.x - (viewHalfSize.x / 2.f),
			center.x + (viewHalfSize.x / 3.f),
			center.y - viewHalfSize.y,
			center.y + viewHalfSize.y
		};
	}

public:
	Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath);
	void		            update(sf::Time dt) override;
	void					updateView(const sf::Vector2u& size);

	void					drawPauseOverlay();
	void					drawEndOverlay();
	void		            sDoAction(const Command& command) override;

	void		            sRender() override;

	void                    fireBullet();
	void					restartGame(const std::string& path);
};
