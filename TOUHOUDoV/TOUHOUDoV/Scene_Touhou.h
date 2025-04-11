#pragma once

#include "Components.h"
#include "Scene.h"
#include <queue>

//==============================
// Supporting Structs
//==============================

struct SpawnPoint {
	std::string type;
	float y;
	auto operator<=>(const SpawnPoint& other) const = default;
};

struct ViewBounds {
	float left, right, top, bot;
};

struct UIText {
	sf::Text text;
	sf::Time lifetime;
};

struct LevelConfig {
	float scrollSpeed{ 100.f };
	float playerSpeed{ 200.f };
	float enemySpeed{ 200.f };
	float bulletSpeed{ 400.f };
	float missileSpeed{ 150.f };
	sf::Time fireInterval{ sf::seconds(5) };
};

//==============================
// Scene_Touhou Class
//==============================

class Scene_Touhou : public Scene {
	//==== Core Systems & Entities ====
	sPtrEntt _player{ nullptr };
	sf::View _worldView;
	sf::FloatRect _worldBounds;
	std::priority_queue<SpawnPoint> _spawnPoints;
	LevelConfig _config;

	//==== Game State Flags ====
	bool _drawTextures{ true };
	bool _drawAABB{ false };
	bool _drawCam{ false };
	bool _isPaused{ false };
	bool _endGame{ false };
	bool bulletsMoving{ true };
	bool backgroundToggle{ true };
	bool bossHasPassed1200{ false };
	bool isBossMovingToTarget{ false };

	//==== Timers ====
	sf::Time _backgroundSwitchCooldown{ sf::Time::Zero };
	sf::Time _backgroundSwitchCooldownMax;
	sf::Time bulletMovementTimer{ sf::seconds(0) };
	sf::Time bulletSpawnTimer{ sf::Time::Zero };
	sf::Time columnSpawnTimer{ sf::Time::Zero };

	//==== Player & Score ====
	int _score{ 0 };
	int _lastScoreThreshold = 0;
	int lastSpreadLevel;
	sf::Vector2f bossTargetPosition;

	//==== Bullet Control ====
	int bulletIndex = 0;
	int currentColumn = 0;

	//==== UI Text ====
	sf::Text _scoreText;
	sf::Text _livesText;
	sf::Text _spellCardsText;
	sf::Text _parryText;
	sf::Text _backgroundCooldownText;
	std::vector<UIText> _temporaryTexts;

	//==== Pause Menu ====
	int _pauseMenuIndex{ 0 };
	sf::Text _pauseMenuText;
	std::vector<std::string> _pauseMenuOptions{ "Continue", "Quit" };

	//==== End Screen ====
	int _endScreenIndex{ 0 };
	sf::Text _endScreenText;
	std::vector<std::string> _endScreenOptions{ "Continue", "Finish" };

	//==== Misc ====
	sf::CircleShape _cooldownCircle;

	//==== Public Interface ====
public:
	Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath);

	void update(sf::Time dt) override;
	void updateView(const sf::Vector2u& size);
	void sDoAction(const Command& command) override;
	void sRender() override;
	void restartGame(const std::string& path);

	//==== Update Systems ====
	void sMovement(sf::Time dt);
	void sUpdate(sf::Time dt);
	void sGunUpdate(sf::Time dt);
	void sCollisions();
	void sAnimation(sf::Time dt);
	void sAutoPilot(sf::Time dt);

	//==== Movement Functions ====
	void movementBoss(sf::Time dt);
	void movementEnemyBullet(sf::Time dt);

	//==== Bullet Fire Systems ====
	void handleFiring(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy,
		const std::string& bulletTexture, const std::string& lineTexture,
		const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread0(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy,
		const std::string& bulletTexture, const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread1(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy,
		const std::string& pkTexture, const std::string& pcTexture);

	void fireSpread2(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& lineTexture);
	void fireSpread3(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);
	void fireSpread4(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);
	void fireSpread5(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture);
	void updateBossSpreadLevel(CGun& gun, int bossCurrentHP);

	//==== Bullet & Enemy Spawn ====
	void spawnBullet(sf::Vector2f pos, bool isEnemy, const std::string& spriteName);
	void spawnPlayer(sf::Vector2f pos);
	void spawnBoss(SpawnPoint sp);
	void spawnSpellCard();
	void sSpawnEnemies();

	//==== Draw Functions ====
	void drawUI(float uiX, float uiY);
	void drawCooldownCircle(float uiX, float uiY);
	void drawSpellAttack(float uiX, float uiY);
	void drawHealthHearts(float uiX, float uiY);
	void drawSpellCards(float uiX, float uiY);
	void drawPickups();
	void drawBullets();
	void drawBossHealthBar();
	void drawBossEntities();
	void drawEntities();
	void drawBackground();
	void drawPauseOverlay();
	void drawEndOverlay();
	void drawEntt(sPtrEntt e);
	void drawAABB(std::shared_ptr<Entity> e);
	void drawCameraView();
	void drawHP(sPtrEntt e);

	//==== Gameplay Logic ====
	void fireBullet();
	void playerMovement();
	void animatePlayer();
	void adjustPlayerPosition();
	void playerSize(bool smaller);

	void resetGameState();
	void startGame();
	void checkIfDead(sPtrEntt e);
	void destroyOutsideBattlefieldBounds();
	void checkBulletCollision();
	void checkSpellCardCollision();
	void despawnAllBullets();

	//==== Utility / Setup ====
	void init(const std::string& path);
	void loadLevel(const std::string& path);
	void onEnd() override;
	void registerActions();
	void trim(std::string& str);

	//==== View / Bounds ====
	sf::FloatRect getBattlefieldBounds() const;
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
};
