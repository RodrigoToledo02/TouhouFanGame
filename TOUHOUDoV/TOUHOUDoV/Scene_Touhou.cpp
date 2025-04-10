#include "Scene_Touhou.h"
#include "Components.h"
#include "Physics.h"
#include "Utilities.h"
#include "MusicPlayer.h"
#include "Assets.h"
#include "SoundPlayer.h"
#include "GameEngine.h"

#include <random>
#include <fstream>
#include <iostream>
#include <vector>

bool firstTimePassing1200 = true;
bool amountOfBullets = false;
bool shooting = false;
bool isSpread4 = false;
int  lastCheckedHP = 50000;
std::string _levelPath;

namespace {
	std::random_device rd;
	std::mt19937 rng(rd());

	struct leg {
		float bearing{ 0 };
		sf::Time dt{ sf::seconds(1) };
	};

	const std::vector<leg> autopilot_directions{
		{0, sf::seconds(1)},
		{50, sf::seconds(2)},
		{0, sf::seconds(1)},
		{-50, sf::seconds(4)},
		{50, sf::seconds(2)},
	};
}

float getRandomFloat(float min, float max) {
	std::uniform_real_distribution<float> dis(min, max);
	return dis(rng);
}

Scene_Touhou::Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath)
	: Scene(gameEngine), _worldView(gameEngine->window().getDefaultView()) {
	init(levelPath);
}

#pragma region GameStates

void Scene_Touhou::init(const std::string& levelPath) {
	_levelPath = levelPath;
	loadLevel(levelPath);
	registerActions();

	auto bounds = getViewBounds();

	auto centerX = bounds.left + (bounds.right - bounds.left) / 2.f;
	sf::Vector2f bottomPos{ centerX, bounds.bot };

	sf::Vector2f spawnPos{ _worldView.getSize().x / 2.f, bounds.top - _worldView.getSize().y / 2.f };

	_worldView.setCenter(spawnPos);

	spawnPlayer(bottomPos);

	MusicPlayer::getInstance().play("gameTheme");
	MusicPlayer::getInstance().setVolume(15);

	// Initialize UI elements
	_scoreText.setFont(Assets::getInstance().getFont("Arial"));
	_scoreText.setCharacterSize(20);
	_scoreText.setFillColor(sf::Color::White);
	_scoreText.setString("Score: 0");

	_livesText.setFont(Assets::getInstance().getFont("Arial"));
	_livesText.setCharacterSize(20);
	_livesText.setFillColor(sf::Color::White);
	_livesText.setString("Lives: ");

	_spellCardsText.setFont(Assets::getInstance().getFont("Arial"));
	_spellCardsText.setCharacterSize(20);
	_spellCardsText.setFillColor(sf::Color::White);
	_spellCardsText.setString("Spell Cards: ");

	_parryText.setFont(Assets::getInstance().getFont("Arial"));
	_parryText.setCharacterSize(20);
	_parryText.setFillColor(sf::Color::White);
	_parryText.setString("Parry: 0");

	_backgroundCooldownText.setFont(Assets::getInstance().getFont("Arial"));
	_backgroundCooldownText.setCharacterSize(20);
	_backgroundCooldownText.setFillColor(sf::Color::White);
	_backgroundCooldownText.setString("Switch Background CD: Ready~");
}

void Scene_Touhou::resetGameState() {
	// Destroy all entities
	for (auto const& e : _entityManager.getEntities()) {
		e->destroy();
	}

	_entityManager.update();

	// Reset flags and variables
	bossHasPassed1200 = false;
	firstTimePassing1200 = true;
	amountOfBullets = false;
	shooting = false;
	isSpread4 = false;
	lastCheckedHP = 50000;
	_isPaused = false;
	_score = 0;

	for (auto const& e : _entityManager.getEntities("bossEnemy")) {
		if (e->hasComponent<CState>()) {
			e->getComponent<CState>().state = "BossInit";
		}
	}
}

void Scene_Touhou::startGame() {
	// Player setup
	auto& gun = _player->addComponent<CGun>();
	auto& spellCard = _player->addComponent<CSpellCard>();
	spellCard.spellCardCount = 3;
	gun.countdown = sf::Time::Zero;
	gun.spreadLevel = 0;
	gun.isFiring = false;
	gun.fireRate = 65;
}

void Scene_Touhou::restartGame(const std::string& levelPath) {
	resetGameState();
	init(levelPath);
}
#pragma endregion

#pragma region GameRelated
void Scene_Touhou::loadLevel(const std::string& path) {
	std::ifstream config(path);
	_levelPath = path;
	if (config.fail()) {
		std::cerr << "Open file " << path << " failed\n";
		config.close();
		exit(1);
	}

	std::string token{ "" };
	config >> token;
	while (!config.eof()) {
		if (token == "Bkg") {
			std::string name;
			sf::Vector2f pos;
			config >> name >> pos.x >> pos.y;
			auto e = _entityManager.addEntity("bkg");

			// for background, no textureRect its just the whole texture
			// and no center origin, position by top left corner
			auto& sprite = e->addComponent<CSprite>(Assets::getInstance().getTexture(name)).sprite;
			sprite.setOrigin(0.f, 0.f);
			sprite.setPosition(pos);
		}
		else if (token == "Bkg1") {
			std::string name;
			sf::Vector2f pos;
			config >> name >> pos.x >> pos.y;
			auto e = _entityManager.addEntity("bkg1");
			auto& sprite = e->addComponent<CSprite>(Assets::getInstance().getTexture(name)).sprite;
			sprite.setOrigin(0.f, 0.f);
			sprite.setPosition(pos);
		}
		else if (token == "HP") {
			std::string name;
			sf::Vector2f pos;
			config >> name >> pos.x >> pos.y;
			auto e = _entityManager.addEntity("HP");
			auto& sprite = e->addComponent<CSprite>(Assets::getInstance().getTexture(name)).sprite;
			sprite.setOrigin(0.f, 0.f);
			sprite.setPosition(pos);
		}
		else if (token == "World") {
			config >> _worldBounds.width >> _worldBounds.height;
		}
		else if (token == "ScrollSpeed") {
			config >> _config.scrollSpeed;
		}
		else if (token == "PlayerSpeed") {
			config >> _config.playerSpeed;
		}
		else if (token == "EnemySpeed") {
			config >> _config.enemySpeed;
		}
		else if (token == "BulletSpeed") {
			config >> _config.bulletSpeed;
		}
		else if (token == "MissileSpeed") {
			config >> _config.missileSpeed;
		}
		else if (token == "FireInterval") {
			float interval;
			config >> interval;
			_config.fireInterval = sf::seconds(interval);
			if (config.fail()) {
				config.clear();
				std::cout << "*** Error reading config file\n";
			}
		}
		else if (token == "Spawn") {
			SpawnPoint p;
			config >> p.type >> p.y;
			p.y = _worldBounds.height - _game->windowSize().y - p.y;
			_spawnPoints.push(p);
		}
		config >> token;
	}

	config.close();
}

void Scene_Touhou::onEnd() {
	_game->changeScene("MENU", nullptr, true);
	resetGameState();
	MusicPlayer::getInstance().play("menuTheme");
}

void Scene_Touhou::fireBullet() {
	if (_player->hasComponent<CGun>())
		_player->getComponent<CGun>().isFiring = true;
}

#pragma endregion

#pragma region Updates
void Scene_Touhou::update(sf::Time dt) {
	if (!_isPaused) {
		sUpdate(dt);
	}
}

void Scene_Touhou::sUpdate(sf::Time dt) {
	if (_isPaused) {
		return; // Skip updates when the game is paused
	}

	SoundPlayer::getInstance().removeStoppedSounds();
	_entityManager.update();
	_worldView.move(0.f, 0.f);

	sf::Vector2f center = _worldView.getCenter();
	SoundPlayer::getInstance().setListnerPosition(center);

	sAnimation(dt);
	sAutoPilot(dt);
	sMovement(dt);
	adjustPlayerPosition();
	sCollisions();
	sSpawnEnemies();
	sGunUpdate(dt);
	destroyOutsideBattlefieldBounds();

	if (_score >= _lastScoreThreshold + 10000) {
		_lastScoreThreshold += 10000;

		// Add a spell card to the player
		if (_player->hasComponent<CSpellCard>()) {
			auto& spellCard = _player->getComponent<CSpellCard>();
			spellCard.spellCardCount += 1;

			// Optionally, update the UI or display a message
			_spellCardsText.setString("Spell Cards: " + std::to_string(spellCard.spellCardCount));
		}
	}

	if (_backgroundSwitchCooldown > sf::Time::Zero) {
		_backgroundSwitchCooldown -= dt;
		auto cooldownSeconds = static_cast<int>(std::ceil(_backgroundSwitchCooldown.asSeconds()));
		_backgroundCooldownText.setString("Switch Background CD: " + std::to_string(cooldownSeconds) + "s");
	}
	else {
		_backgroundCooldownText.setString("Switch Background CD: READY~");
	}

	for (auto it = _temporaryTexts.begin(); it != _temporaryTexts.end();) {
		it->lifetime -= dt;
		if (it->lifetime <= sf::Time::Zero) {
			it = _temporaryTexts.erase(it);
		}
		else {
			++it;
		}
	}
}
#pragma endregion

#pragma region PlayerInputs

void Scene_Touhou::registerActions() {
	registerAction(sf::Keyboard::I, "SKIP");
	registerAction(sf::Keyboard::O, "ZOOMIN");
	registerAction(sf::Keyboard::F11, "TOGGLE_VIEW_MODE");

	registerAction(sf::Keyboard::Escape, "PAUSE");
	registerAction(sf::Keyboard::P, "BACK");
	registerAction(sf::Keyboard::Q, "QUIT");

	registerAction(sf::Keyboard::B, "TOGGLE_COLLISION");
	registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
	registerAction(sf::Keyboard::V, "TOGGLE_CAMOUTLINE");

	registerAction(sf::Keyboard::X, "LAUNCH");
	registerAction(sf::Keyboard::LShift, "GRAZE");
	registerAction(sf::Keyboard::C, "SWITCH");

	registerAction(sf::Keyboard::A, "LEFT");
	registerAction(sf::Keyboard::Left, "LEFT");
	registerAction(sf::Keyboard::D, "RIGHT");
	registerAction(sf::Keyboard::Right, "RIGHT");
	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::Up, "UP");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Down, "DOWN");
	registerAction(sf::Keyboard::Space, "SELECT");
}

void Scene_Touhou::sDoAction(const Command& command) {
	// On Key Press
	if (command.type() == "START") {
		if (command.name() == "PAUSE") {
			_isPaused = !_isPaused;
		}
		else if (_isPaused) {
			if (command.name() == "UP") {
				_pauseMenuIndex = (_pauseMenuIndex + _pauseMenuOptions.size() - 1) % _pauseMenuOptions.size();
			}
			else if (command.name() == "DOWN") {
				_pauseMenuIndex = (_pauseMenuIndex + 1) % _pauseMenuOptions.size();
			}
			else if (command.name() == "SELECT") {
				if (_pauseMenuIndex == 0) { // Continue
					_isPaused = false;
				}
				else if (_pauseMenuIndex == 1) { // Quit
					onEnd();
				}
			}
		}
		else if (command.name() == "TOGGLE_VIEW_MODE") {
			_game->toggleViewMode();
		}
		else if (
			command.name() == "QUIT") {
			onEnd();
		}
		else if (
			command.name() == "BACK") {
			onEnd();
		}
		else if (
			command.name() == "ZOOMOUT") {
			_worldView.zoom(1.5);
		}
		else if (
			command.name() == "ZOOMIN") {
			_worldView.zoom(0.66667);
		}
		else if (
			command.name() == "TOGGLE_TEXTURE") {
			_drawTextures = !_drawTextures;
		}
		else if (
			command.name() == "TOGGLE_COLLISION") {
			_drawAABB = !_drawAABB;
		}
		else if (
			command.name() == "TOGGLE_CAMOUTLINE") {
			_drawCam = !_drawCam;
		}

		// Player control
		else if (command.name() == "LEFT") { _player->getComponent<CInput>().left = true; }
		else if (
			command.name() == "RIGHT") {
			_player->getComponent<CInput>().right = true;
		}
		else if (
			command.name() == "UP") {
			_player->getComponent<CInput>().up = true;
		}
		else if (command.name() == "DOWN") {
			_player->getComponent<CInput>().down = true;
		}
		else if (command.name() == "LAUNCH") { spawnSpellCard(); }
		else if (command.name() == "GRAZE") {
			playerSize(true);
			_player->getComponent<CInput>().lshift = true;
		}
		else if (command.name() == "SWITCH") {
			if (_backgroundSwitchCooldown <= sf::Time::Zero) {
				backgroundToggle = !backgroundToggle;
				_backgroundSwitchCooldown = sf::seconds(60);
				despawnAllBullets();
			}
		}
	}
	// on Key Release
	else if (command.type() == "END") {
		// Player control
		if (command.name() == "LEFT") { _player->getComponent<CInput>().left = false; }
		else if (
			command.name() == "RIGHT") {
			_player->getComponent<CInput>().right = false;
		}
		else if (
			command.name() == "UP") {
			_player->getComponent<CInput>().up = false;
		}
		else if (command.name() == "DOWN") {
			_player->getComponent<CInput>().down = false;
		}
		else if (command.name() == "GRAZE") {
			playerSize(false);
			_player->getComponent<CInput>().lshift = false;
		}
	}
}

#pragma endregion

#pragma region Camera

void Scene_Touhou::updateView(const sf::Vector2u& size) {
	_worldView.setSize(static_cast<float>(size.x), static_cast<float>(size.y));
	_worldView.setCenter(size.x / 2.f, size.y / 2.f);
	_game->window().setView(_worldView);
}

void Scene_Touhou::drawCameraView() {
	if (_drawCam) {
		auto size = _game->window().getSize();
		_worldView.setSize(static_cast<float>(size.x), static_cast<float>(size.y));
		_worldView.setCenter(size.x / 2.f, size.y / 2.f);
		_game->window().setView(_worldView);

		sf::RectangleShape rect;
		rect.setSize(sf::Vector2f(size.x - 10, size.y - 10));
		centerOrigin(rect);
		rect.setPosition(_worldView.getCenter());
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color{ 0, 0, 0 });
		rect.setOutlineThickness(8.f);
		_game->window().draw(rect);
	}
}

#pragma endregion

#pragma region Renders

void Scene_Touhou::sRender() {
	auto& center = _worldView.getCenter();
	_game->window().setView(_worldView);
	_game->window().clear(sf::Color::White);
	auto bounds = getViewBounds();

	// Position UI elements to the right of the background
	float uiX = bounds.right + 20.f;
	float uiY = bounds.top + 20.f;

	sf::FloatRect backgroundBounds;
	if (backgroundToggle) {
		for (auto& e : _entityManager.getEntities("bkg")) {
			auto& sprite = e->getComponent<CSprite>().sprite;
			backgroundBounds = sprite.getGlobalBounds();
			break;
		}
	}
	else {
		for (auto& e : _entityManager.getEntities("bkg1")) {
			auto& sprite = e->getComponent<CSprite>().sprite;
			backgroundBounds = sprite.getGlobalBounds();
			break;
		}
	}

	// draw bkg first
	if (backgroundToggle) {
		for (auto& e : _entityManager.getEntities("bkg")) {
			_game->window().clear(sf::Color::White);

			_scoreText.setPosition(uiX, uiY);
			_scoreText.setFillColor(sf::Color::Black);
			_game->window().draw(_scoreText);

			_livesText.setPosition(uiX, uiY + 30.f);
			_livesText.setFillColor(sf::Color::Black);
			_game->window().draw(_livesText);

			_spellCardsText.setPosition(uiX, uiY + 60.f);
			_spellCardsText.setFillColor(sf::Color::Black);
			_game->window().draw(_spellCardsText);

			_backgroundCooldownText.setPosition(uiX, uiY + 120.f);
			_backgroundCooldownText.setFillColor(sf::Color::Black);
			_game->window().draw(_backgroundCooldownText);

			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				sprite.setPosition(bounds.left, bounds.top);
				_game->window().draw(sprite);
			}
		}
	}
	else {
		for (auto& e : _entityManager.getEntities("bkg1")) {
			_game->window().clear(sf::Color::Black);

			_scoreText.setPosition(uiX, uiY);
			_scoreText.setFillColor(sf::Color::White);
			_game->window().draw(_scoreText);

			_livesText.setPosition(uiX, uiY + 30.f);
			_livesText.setFillColor(sf::Color::White);
			_game->window().draw(_livesText);

			_spellCardsText.setPosition(uiX, uiY + 60.f);
			_spellCardsText.setFillColor(sf::Color::White);
			_game->window().draw(_spellCardsText);

			_backgroundCooldownText.setPosition(uiX, uiY + 120.f);
			_backgroundCooldownText.setFillColor(sf::Color::White);
			_game->window().draw(_backgroundCooldownText);

			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				sprite.setPosition(bounds.left, bounds.top);

				sprite.setScale((bounds.right - bounds.left) / sprite.getTexture()->getSize().x, (bounds.bot - bounds.top) / sprite.getTexture()->getSize().y);
				_game->window().draw(sprite);
			}
		}
	}

	for (auto& e : _entityManager.getEntities("HP")) {
		auto& sprite = e->getComponent<CSprite>().sprite;
		int playerHP = _player->getComponent<CHealth>().hp;
		int numHearts = playerHP / 10;
		for (int i = 0; i < numHearts; ++i) {
			sprite.setScale(0.7f, 0.7f);
			if (backgroundToggle)
				sprite.setTextureRect(sf::IntRect(32, 0, 32, 32));
			else
				sprite.setTextureRect(sf::IntRect(0, 0, 32, 32));
			sprite.setPosition((_livesText.getPosition().x + 64.f) + i * 25.f, _livesText.getPosition().y);
			_game->window().draw(sprite);
		}
	}

	for (auto& e : _entityManager.getEntities("HP")) {
		auto& sprite = e->getComponent<CSprite>().sprite;
		int spellCardCount = _player->getComponent<CSpellCard>().spellCardCount; // Get the player's spell card count
		for (int i = 0; i < spellCardCount; ++i) {
			sprite.setScale(0.7f, 0.7f);
			if (backgroundToggle)
				sprite.setTextureRect(sf::IntRect(32, 0, 32, 32)); // Use a specific texture rect for spell cards
			else
				sprite.setTextureRect(sf::IntRect(0, 0, 32, 32));
			sprite.setPosition((_spellCardsText.getPosition().x + 128.f) + i * 25.f, _spellCardsText.getPosition().y); // Position the sprites
			_game->window().draw(sprite);
		}
	}

	// draw pickups
	for (auto& e : _entityManager.getEntities("Pickup")) {
		drawEntt(e);
		drawAABB(e);
	}

	// draw player bullets
	for (auto& e : _entityManager.getEntities("PlayerBullet")) {
		auto pos = e->getComponent<CTransform>().pos;
		if (pos.x >= backgroundBounds.left && pos.x <= (backgroundBounds.left + backgroundBounds.width)) {
			drawEntt(e);
			drawAABB(e);
		}
	}

	for (auto& e : _entityManager.getEntities("EnemyBullet")) {
		auto pos = e->getComponent<CTransform>().pos;
		if (pos.x >= backgroundBounds.left && pos.x <= (backgroundBounds.left + backgroundBounds.width)) {
			drawEntt(e);
			drawAABB(e);
		}
	}

	for (auto& e : _entityManager.getEntities("spellCard")) {
		//auto pos = e->getComponent<CTransform>().pos;
		//if (pos.x >= backgroundBounds.left && pos.x <= (backgroundBounds.left + backgroundBounds.width)) {
		drawEntt(e);
		drawAABB(e);
		//}
	}

	for (auto& e : _entityManager.getEntities("bossEnemy")) {
		if (e->getComponent<CState>().state == "BossActive") {
			auto& health = e->getComponent<CHealth>();
			float healthPercentage = static_cast<float>(health.hp) / 50000.f;
			float healthBarWidth = backgroundBounds.width * 0.9f * healthPercentage;
			sf::RectangleShape healthBar(sf::Vector2f(100.f, 20.f));
			healthBar.setFillColor(sf::Color::Red);
			healthBar.setPosition(center.x, bounds.top - 30.f);
			_game->window().draw(healthBar);
			drawHP(e);
		}
		drawEntt(e);
		drawAABB(e);

		drawAmmo(e);
	}

	// draw all entities
	for (auto& e : _entityManager.getEntities()) {
		if (!e->hasComponent<CAnimation>() || e->getTag() == "bkg" || e->getTag() == "Pickup" || e->getTag() == "PlayerBullet" || e->getTag() == "EnemyBullet" || e->getTag() == "bossEnemy" || e->getTag() == "spellCard")
			continue;
		drawEntt(e);
		drawAABB(e);
		drawHP(e);
		drawAmmo(e);
	}
	drawCameraView();

	// Render UI elements

	if (_isPaused) {
		drawPauseOverlay();
	}

	for (const auto& tempText : _temporaryTexts) {
		_game->window().draw(tempText.text);
	}
}

void Scene_Touhou::drawPauseOverlay() {
	auto size = _game->window().getSize();
	auto bounds = getViewBounds();
	auto& center = _worldView.getCenter();

	_game->window().setView(_worldView); // Set the current view

	sf::RectangleShape overlay(sf::Vector2f(bounds.right - bounds.left, bounds.bot - bounds.top));
	overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Semi-transparent black
	overlay.setPosition(bounds.left, bounds.top);
	_game->window().draw(overlay);

	_pauseMenuText.setFont(Assets::getInstance().getFont("main"));
	_pauseMenuText.setCharacterSize(30);

	static const sf::Color selectedColor(150, 150, 150);
	static const sf::Color normalColor(255, 255, 255);

	for (size_t i = 0; i < _pauseMenuOptions.size(); ++i) {
		_pauseMenuText.setString(_pauseMenuOptions[i]);
		_pauseMenuText.setFillColor((i == _pauseMenuIndex) ? selectedColor : normalColor);

		sf::FloatRect textBounds = _pauseMenuText.getLocalBounds();
		_pauseMenuText.setOrigin(textBounds.width / 2, textBounds.height / 2);
		_pauseMenuText.setPosition(center.x, center.y + i * 40);

		_game->window().draw(_pauseMenuText);
	}
}

void Scene_Touhou::drawAABB(std::shared_ptr<Entity> e) {
	if (_drawAABB) {
		auto box = e->getComponent<CBoundingBox>();
		sf::Shape* shape;
		if (box.isCircular) {
			shape = new sf::CircleShape(box.radius);
		}
		else {
			shape = new sf::RectangleShape(sf::Vector2f{ box.size.x, box.size.y });
		}
		centerOrigin(*shape);
		shape->setPosition(e->getComponent<CTransform>().pos);
		shape->setFillColor(sf::Color(0, 0, 0, 0));
		shape->setOutlineColor(sf::Color{ 0, 255, 0 });
		shape->setOutlineThickness(2.f);
		_game->window().draw(*shape);
		delete shape;
	}
}

void Scene_Touhou::drawHP(sPtrEntt e) {
	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;
	int bossRefills = 0;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto top = center.y - viewHalfSize.y;

	auto hpColor = backgroundToggle ? sf::Color::White : sf::Color::Black;

	sf::Text text = sf::Text("HP: ", Assets::getInstance().getFont("Arial"), 15);
	if (e->getTag() == "bossEnemy" && e->hasComponent<CHealth>()) {
		auto& health = e->getComponent<CHealth>();
		int currentHP = health.hp % 10000;
		if (currentHP == 0 && health.hp > 0) {
			currentHP = 10000;
		}
		float healthPercentage = static_cast<float>(currentHP) / 10000.f; // Assuming max HP is 10,000
		float healthBarWidth = (right - left) * 0.9f * healthPercentage;
		sf::RectangleShape healthBar(sf::Vector2f(healthBarWidth, 20.f));
		healthBar.setFillColor(hpColor);
		auto centerX = left + (right - left) / 2.f;
		healthBar.setPosition(centerX - (healthBarWidth / 2.f), top + 10.f);
		_game->window().draw(healthBar);
	}
	else if (e->hasComponent<CHealth>()) {
		int hp = e->getComponent<CHealth>().hp;
		std::string str = "HP: " + std::to_string(hp);
		text.setString(str);
		centerOrigin(text);

		sf::Vector2f offset(0.f, 40.f);
		offset.y = (e->getTag() == "enemy") ? -40.f : 40.f;
		text.setPosition(e->getComponent<CTransform>().pos + offset);
		_game->window().draw(text);
	}
}

void Scene_Touhou::drawAmmo(sPtrEntt e) {
	// draw ammo count if missiles
	sf::Text text = sf::Text("M: ", Assets::getInstance().getFont("Arial"), 15);

	if (e->hasComponent<CSpellCard>()) {
		int count = e->getComponent<CSpellCard>().spellCardCount;
		std::string str = "M: " + std::to_string(count);
		text.setString(str);
		centerOrigin(text);

		sf::Vector2f offset(0.f, 55.f);
		if (e->getTag() == "enemy")
			offset *= -1.f;
		text.setPosition(e->getComponent<CTransform>().pos + offset);
		_game->window().draw(text);
	}
}

void Scene_Touhou::drawEntt(sPtrEntt e) {
	// Draw Sprite
	auto& anim = e->getComponent<CAnimation>().animation;
	auto& tfm = e->getComponent<CTransform>();
	anim.getSprite().setPosition(tfm.pos);
	anim.getSprite().setRotation(tfm.angle);
	_game->window().draw(anim.getSprite());
}

#pragma endregion

#pragma region PlayerSettings

void Scene_Touhou::playerSize(bool set) {
	auto& bb = _player->getComponent<CBoundingBox>();
	static sf::Vector2f originalSize = bb.size; // Store the original size

	if (set) {
		// Make the bounding box smaller and circular
		bb.isCircular = true;
		bb.radius = std::min(originalSize.x, originalSize.y) / 4.f; // Half the radius
		_player->getComponent<CGun>().spreadLevel = 1;
	}
	else {
		// Restore the original bounding box size and shape
		bb.isCircular = false;
		bb.size = originalSize;
		bb.halfSize = bb.size / 2.f;
		_player->getComponent<CGun>().spreadLevel = 0;
	}
}

void Scene_Touhou::playerMovement() {
	// no movement if player is dead
	if (_player->hasComponent<CState>() && _player->getComponent<CState>().state == "dead")
		return;

	// player movement
	sf::Vector2f pv{ 0.f, 0.f };
	auto& pInput = _player->getComponent<CInput>();
	if (pInput.left) pv.x -= 1;
	if (pInput.right) pv.x += 1;
	if (pInput.up) pv.y -= 1;
	if (pInput.down) pv.y += 1;
	float speed = pInput.lshift ? _config.playerSpeed * 0.5f : _config.playerSpeed;
	pv = normalize(pv);
	_player->getComponent<CTransform>().vel = speed * pv;

	animatePlayer();
}

void Scene_Touhou::animatePlayer() {
	if (_player->getComponent<CState>().state == "dead")
		return;

	auto pv = _player->getComponent<CTransform>().vel;
	// implement roll animation, set texture rec accordingly
	if (pv.x < -0.1)
		_player->addComponent<CAnimation>(Assets::getInstance().getAnimation("Idle"));
	else if (pv.x > 0.1)
		_player->addComponent<CAnimation>(Assets::getInstance().getAnimation("Idle"));
	else
		_player->addComponent<CAnimation>(Assets::getInstance().getAnimation("Idle"));
}

void Scene_Touhou::adjustPlayerPosition() {
	auto bounds = getViewBounds();
	// don't adjust position if dead
	if (_player->getComponent<CState>().state == "dead")
		return;

	auto& player_pos = _player->getComponent<CTransform>().pos;
	auto halfSize = _player->getComponent<CBoundingBox>().halfSize;

	// keep player in bounds
	player_pos.x = std::max(player_pos.x, bounds.left + halfSize.x);
	player_pos.x = std::min(player_pos.x, bounds.right - halfSize.x);
	player_pos.y = std::max(player_pos.y, bounds.top + halfSize.y);
	player_pos.y = std::min(player_pos.y, bounds.bot - halfSize.y);
}

#pragma endregion

#pragma region Movement
void Scene_Touhou::sMovement(sf::Time dt) {
	playerMovement();
	animatePlayer();
	movementBoss(dt);
	movementEnemyBullet(dt);

	// move all objects
	for (auto e : _entityManager.getEntities()) {
		if (e->hasComponent<CTransform>() &&
			e->getTag() != "bossEnemy" &&
			e->getTag() != "EnemyBullet")
		{
			auto& tfm = e->getComponent<CTransform>();

			auto pBullet = e->getTag() == "PlayerBullet";
			auto sCard = e->getTag() == "spellCard";

			if (pBullet)
			{
				tfm.pos += tfm.vel * dt.asSeconds();
				tfm.angle = 90 + bearing(tfm.vel);
				tfm.angle += tfm.angVel * dt.asSeconds();
			}
			else if (sCard)
			{
				tfm.pos += tfm.vel * dt.asSeconds();
				tfm.angVel = 180.f;
				tfm.angle += tfm.angVel * dt.asSeconds();
			}
			else {
				tfm.pos += tfm.vel * dt.asSeconds();
				tfm.angle += tfm.angVel * dt.asSeconds();
			}
		}
	}
}

void Scene_Touhou::movementBoss(sf::Time dt) {
	const float dtSec = dt.asSeconds();
	const auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;
	auto bounds = getViewBounds();
	auto centerBounds = bounds.left + (bounds.right - bounds.left) / 2.f;
	String bossState = "";

	// Get boss state from first boss entity with CGun
	for (auto e : _entityManager.getEntities("bossEnemy")) {
		if (e->hasComponent<CGun>()) {
			bossState = e->getComponent<CState>().state;
			break;
		}
	}

	for (auto e : _entityManager.getEntities("bossEnemy")) {
		if (!e->hasComponent<CTransform>()) continue;

		auto& tfm = e->getComponent<CTransform>();
		auto& state = e->getComponent<CState>().state;

		if (state == "BossInit") {
			if (tfm.pos.y < center.y - 300.f && !bossHasPassed1200) {
				tfm.pos += tfm.vel * dtSec;
				tfm.angle += tfm.angVel * dtSec;
				continue;
			}

			// Transition to active state
			bossHasPassed1200 = true;
			state = "BossActive";

			if (firstTimePassing1200) {
				e->addComponent<CGun>();
				tfm.vel.x += 25.f;
				firstTimePassing1200 = false;
				startGame();
			}
		}

		else if (state == "BossRetreat") {
			sf::Vector2f target(centerBounds, center.y - 301.f);
			if (std::abs(tfm.pos.x - target.x) <= 1.f && std::abs(tfm.pos.y - target.y) <= 1.f) {
				state = "BossIdle";
				tfm.pos = target;
				continue;
			}

			sf::Vector2f dir = normalize(target - tfm.pos);
			tfm.pos += dir * _config.enemySpeed * dtSec;
		}

		else if (state == "BossActive") {
			tfm.pos += tfm.vel * dtSec;
			tfm.angle += tfm.angVel * dtSec;

			if (tfm.pos.x < bounds.left || tfm.pos.x > bounds.right) tfm.vel.x = -tfm.vel.x;
			if (tfm.pos.y < center.y - 300.f || tfm.pos.y > center.y - 100.f) tfm.vel.y = -tfm.vel.y;
		}

		else if (state == "BossIdle") {
			auto& gun = e->getComponent<CGun>();
			if (gun.cooldown > sf::Time::Zero) {
				tfm.vel = { 0.f, 0.f };
			}
			else {
				state = "BossActive";
				tfm.vel = { _config.enemySpeed, _config.enemySpeed };
			}
		}
	}
}

void Scene_Touhou::movementEnemyBullet(sf::Time dt) {
	bulletMovementTimer -= dt;
	if (bulletMovementTimer <= sf::Time::Zero) {
		bulletsMoving = !bulletsMoving;
		bulletMovementTimer = sf::seconds(3);
	}

	// Get boss spread level
	int bossSpreadLevel = 1;
	for (auto e : _entityManager.getEntities("bossEnemy")) {
		if (e->hasComponent<CGun>()) {
			bossSpreadLevel = e->getComponent<CGun>().spreadLevel;
			break;
		}
	}

	const float dtSec = dt.asSeconds();
	const bool moveBullets = bulletsMoving;
	const int bulletCount = _entityManager.getEntities("EnemyBullet").size();

	for (auto& e : _entityManager.getEntities("EnemyBullet")) {
		if (!e->hasComponent<CTransform>()) continue;

		auto& tfm = e->getComponent<CTransform>();

		switch (bossSpreadLevel) {
		case 5:
			tfm.vel.y = (bulletCount <= 100) ? 0.f : 200.f;
			tfm.pos.y += tfm.vel.y * dtSec;
			break;

		case 4:
			if (moveBullets) {
				tfm.vel.y = 0.f;
				tfm.pos.y += (tfm.vel.y * 0.1f) * dtSec;
				tfm.pos.x += (tfm.vel.x * 0.1f) * dtSec;
			}
			else {
				tfm.vel.y = 200.f;
				tfm.pos.y += (tfm.vel.y * 0.15f) * dtSec;
				tfm.pos.x += 0.f;
			}
			break;

		case 3:
			tfm.pos.y += (tfm.vel.y * 0.5f) * dtSec;
			tfm.pos.x += (tfm.vel.x * 0.5f) * dtSec;
			break;

		case 2:
			tfm.pos.y += (tfm.vel.y * 0.5f) * dtSec;
			break;

		default:
			tfm.pos.y += (tfm.vel.y * 0.7f) * dtSec;
			break;
		}
	}
}

#pragma endregion

#pragma region GunUpdate

void Scene_Touhou::sGunUpdate(sf::Time dt) {
	static sf::Time spreadChangeTimer = sf::seconds(5.f + (std::rand() % 6));

	std::string bulletTexture = backgroundToggle ? "ShotWhite" : "ShotBlack";
	std::string lineTexture = backgroundToggle ? "LineShot" : "LineShotBlack";
	std::string pkTexture = backgroundToggle ? "WhiteKnife" : "BlackKnife";
	std::string pcTexture = backgroundToggle ? "WhiteCard" : "BlackCard";

	for (auto e : _entityManager.getEntities()) {
		auto bullet = e->getTag() == "EnemyBullet";
		if (e->hasComponent<CGun>() && e->hasComponent<CTransform>()) {
			bool isEnemy = (e->getTag() == "enemy");
			bool isBossEnemy = (e->getTag() == "bossEnemy");
			bool isPlayer = (e->getTag() == "player");

			auto& gun = e->getComponent<CGun>();
			gun.countdown -= dt;
			gun.cooldown -= dt;
			bulletSpawnTimer -= dt;

			if (isBossEnemy && e->hasComponent<CHealth>()) {
				int bossCurrentHP = e->getComponent<CHealth>().hp;

				if ((lastCheckedHP - bossCurrentHP) >= 10000) {
					updateBossSpreadLevel(gun, bossCurrentHP);
				}
			}

			if (gun.cooldown > sf::Time::Zero) continue;

			if (isEnemy || isBossEnemy)
				gun.isFiring = true;

			if (gun.isFiring && gun.countdown <= sf::Time::Zero) {
				gun.isFiring = false;
				gun.countdown = _config.fireInterval / (1.f + gun.fireRate);
				auto pos = e->getComponent<CTransform>().pos;
				handleFiring(gun, pos, isPlayer, isBossEnemy, bulletTexture, lineTexture, pkTexture, pcTexture);
			}
		}
	}
}

void Scene_Touhou::updateBossSpreadLevel(CGun& gun, int bossCurrentHP) {
	std::uniform_int_distribution<int> dist(2, 5);
	int newSpreadLevel;
	bulletIndex = 0;

	do {
		newSpreadLevel = dist(rng);
	} while (newSpreadLevel == lastSpreadLevel);

	gun.spreadLevel = newSpreadLevel;
	lastSpreadLevel = newSpreadLevel;
	lastCheckedHP = bossCurrentHP;

	despawnAllBullets();
	gun.cooldown = sf::seconds(6);
	for (auto& e : _entityManager.getEntities("bossEnemy")) {
		if (e->hasComponent<CState>())
		{
			auto& state = e->getComponent<CState>().state;
			state = "BossRetreat";
		}
	}
}

void Scene_Touhou::handleFiring(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& bulletTexture, const std::string& lineTexture, const std::string& pkTexture, const std::string& pcTexture) {
	switch (gun.spreadLevel) {
	case 0:
		fireSpread0(gun, pos, isPlayer, isBossEnemy, bulletTexture, pkTexture, pcTexture);
		break;
	case 1:
		fireSpread1(gun, pos, isPlayer, isBossEnemy, pkTexture, pcTexture);
		break;
	case 2:
		fireSpread2(gun, pos, isBossEnemy, lineTexture);
		break;
	case 3:
		fireSpread3(gun, pos, isBossEnemy, bulletTexture);
		break;
	case 4:
		fireSpread4(gun, pos, isBossEnemy, bulletTexture);
		break;
	case 5:
		fireSpread5(gun, pos, isBossEnemy, bulletTexture);
		break;
	default:
		std::cerr << "Bad spread level firing gun\n";
		break;
	}
}

void Scene_Touhou::fireSpread0(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& bulletTexture, const std::string& pkTexture, const std::string& pcTexture) {
	if (isPlayer) {
		gun.fireRate = 60;

		if (!_player->getComponent<CInput>().lshift) {
			//  Fan-shaped spread shooting
			const int numBullets = 4;
			const float spreadAngle = 10.f;
			const float baseAngle = -90.f;
			const float speed = -_config.bulletSpeed;

			for (int i = 0; i < numBullets; ++i) {
				float angleDeg = baseAngle - (spreadAngle / 2.f) + (i * (spreadAngle / (numBullets - 1)));
				float angleRad = angleDeg * 3.14159265f / 180.f;

				sf::Vector2f velocity(std::cos(angleRad), std::sin(angleRad));
				velocity *= std::abs(speed);

				// Create bullet manually (like spawnBullet, but we want custom velocity)
				auto bullet = _entityManager.addEntity("PlayerBullet");
				const auto& animation = Assets::getInstance().getAnimation(pcTexture);
				auto& animComp = bullet->addComponent<CAnimation>(animation).animation;
				auto bb = animComp.getBB();

				bullet->addComponent<CBoundingBox>(bb);

				bullet->addComponent<CTransform>(pos, velocity);
				bullet->addComponent<CSpawnPosition>(pos);
			}

			auto createSideBullet = [&](float offsetX) {
				// Create a bullet that shoots straight up
				sf::Vector2f sidePos = pos + sf::Vector2f(offsetX, 0.f);
				sf::Vector2f velocity(0.f, -std::abs(speed));  // Straight up

				auto bullet = _entityManager.addEntity("PlayerBullet");
				const auto& animation = Assets::getInstance().getAnimation(pkTexture);
				auto& animComp = bullet->addComponent<CAnimation>(animation).animation;
				auto bb = animComp.getBB();

				bullet->addComponent<CBoundingBox>(bb);

				bullet->addComponent<CTransform>(sidePos, velocity);
				bullet->addComponent<CSpawnPosition>(sidePos);
				};

			createSideBullet(-20.f);
			createSideBullet(20.f);

			// Optional: play shot sound once
			SoundPlayer::getInstance().play("Damage01", _worldView.getCenter(), 50.f);
		}
	}

	else {
		gun.fireRate = gun.originalFireRate;
		spawnBullet(pos + sf::Vector2f(-20.f, 0.f), isBossEnemy, bulletTexture);
		spawnBullet(pos + sf::Vector2f(20.f, 0.f), isBossEnemy, bulletTexture);
	}
}

void Scene_Touhou::fireSpread1(CGun& gun, const sf::Vector2f& pos, bool isPlayer, bool isBossEnemy, const std::string& pkTexture, const std::string& pcTexture) {
	if (isPlayer && _player->getComponent<CInput>().lshift) {
		const int numBullets = 5;
		const float spreadAngle = 40.f;
		const float baseAngle = -90.f;
		const float speed = -_config.bulletSpeed;
		gun.fireRate = 60;
		spawnBullet(pos + sf::Vector2f(0.f, 0.f), isBossEnemy, pkTexture);

		for (int i = 0; i < numBullets; ++i) {
			float angleDeg = baseAngle - (spreadAngle / 2.f) + (i * (spreadAngle / (numBullets - 1)));
			float angleRad = angleDeg * 3.14159265f / 180.f;

			sf::Vector2f velocity(std::cos(angleRad), std::sin(angleRad));
			velocity *= std::abs(speed);

			auto bullet = _entityManager.addEntity("PlayerBullet");
			const auto& animation = Assets::getInstance().getAnimation(pkTexture);
			auto& animComp = bullet->addComponent<CAnimation>(animation).animation;
			auto bb = animComp.getBB();

			bullet->addComponent<CBoundingBox>(bb);

			bullet->addComponent<CTransform>(pos, velocity);
			bullet->addComponent<CSpawnPosition>(pos);
		}

		auto createSideBullet = [&](float offsetX) {
			sf::Vector2f sidePos = pos + sf::Vector2f(offsetX, 0.f);
			sf::Vector2f velocity(0.f, -std::abs(speed));  // Straight up

			auto bullet = _entityManager.addEntity("PlayerBullet");
			const auto& animation = Assets::getInstance().getAnimation(pcTexture);
			auto& animComp = bullet->addComponent<CAnimation>(animation).animation;
			auto bb = animComp.getBB();

			bullet->addComponent<CBoundingBox>(bb);

			bullet->addComponent<CTransform>(sidePos, velocity);
			bullet->addComponent<CSpawnPosition>(sidePos);
			};

		createSideBullet(-20.f);
		createSideBullet(20.f);
	}
}

void Scene_Touhou::fireSpread2(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& lineTexture) {
	if (isBossEnemy) {
		gun.fireRate = gun.originalFireRate;
		bulletSpawnTimer = sf::Time::Zero;
		for (int i = -845; i <= 845; i += 65) {
			spawnBullet(pos + sf::Vector2f(i, -500.f), isBossEnemy, lineTexture);
		}
	}
}

void Scene_Touhou::fireSpread3(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture) {
	if (isBossEnemy) {
		gun.fireRate = gun.originalFireRate;
		bulletSpawnTimer = sf::Time::Zero;
		float circleRadius = 50.f;
		float bulletsPerCircle = 10;
		float angleStep = 360.f / bulletsPerCircle;

		std::vector<sf::Vector2f> circleOffsets = {
			sf::Vector2f(0.f, 100.f),
			sf::Vector2f(-150.f, -150.f),
			sf::Vector2f(150.f, -150.f)
		};

		for (int i = 0; i < 3; ++i) {
			sf::Vector2f circleCenter = pos + circleOffsets[i];
			for (int j = 0; j < bulletsPerCircle; ++j) {
				float angle = j * angleStep;
				sf::Vector2f bulletPos = circleCenter + sf::Vector2f(
					std::cos(degToRad(angle)) * circleRadius,
					std::sin(degToRad(angle)) * circleRadius
				);
				spawnBullet(bulletPos, isBossEnemy, bulletTexture);
			}
		}
	}
}

void Scene_Touhou::fireSpread4(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture) {
	if (isBossEnemy) {
		gun.fireRate = 3;

		if (bulletsMoving) {
			auto& center = _worldView.getCenter();
			sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;
			auto left = center.x - (viewHalfSize.x / 2.f);
			auto right = center.x + (viewHalfSize.x / 3.f);
			auto top = center.y - viewHalfSize.y;
			auto bot = center.y + viewHalfSize.y;

			float ySpacing = 100.f;
			float yOffset = (currentColumn % 2 == 0) ? 0.f : 50.f;

			for (float y = top + yOffset - 200.f; y <= bot + 200.f; y += ySpacing) {
				spawnBullet(sf::Vector2f(left - 100.f, y), isBossEnemy, bulletTexture);
				spawnBullet(sf::Vector2f(right + 100.f, y - 50.f), isBossEnemy, bulletTexture);
			}

			currentColumn++;
		}
	}
}

void Scene_Touhou::fireSpread5(CGun& gun, const sf::Vector2f& pos, bool isBossEnemy, const std::string& bulletTexture) {
	if (_entityManager.getEntities("EnemyBullet").size() <= 500) {
		if (isBossEnemy) {
			gun.fireRate = 50;
			const int numBullets = 20;
			const float angleStep = 360.f / (numBullets - 1);
			const float radius = 300.f;
			const float verticalOffset = -350.f;      // Move upward
			const float horizontalSpread = 500.f;    // Distance between rings

			if (bulletSpawnTimer <= sf::Time::Zero) {
				// Positions for the 3 rings: center, left, right
				std::vector<sf::Vector2f> ringCenters = {
					pos + sf::Vector2f(0.f, verticalOffset),                         // center
					pos + sf::Vector2f(-horizontalSpread, verticalOffset),          // left
					pos + sf::Vector2f(horizontalSpread, verticalOffset)            // right
				};

				for (const auto& centerPos : ringCenters) {
					float angle = bulletIndex * angleStep;
					sf::Vector2f offset = sf::Vector2f(
						std::cos(degToRad(angle)) * radius,
						std::sin(degToRad(angle)) * radius
					);
					spawnBullet(centerPos + offset, isBossEnemy, bulletTexture);
				}

				bulletIndex++;
				if (bulletIndex >= numBullets) {
					bulletIndex = 0;
					gun.isFiring = false;
				}
			}
		}
	}
	else {
		gun.isFiring = false;
	}
}

#pragma endregion

#pragma region Spawns

void Scene_Touhou::spawnPlayer(sf::Vector2f pos) {
	_player = _entityManager.addEntity("player");
	_player->addComponent<CTransform>(pos);

	auto bb = _player->addComponent<CAnimation>(Assets::getInstance()
		.getAnimation("Idle")).animation.getBB();
	_player->addComponent<CBoundingBox>(bb);
	bb = _player->getComponent<CBoundingBox>().halfSize;
	_player->addComponent<CBoundingBox>(bb);

	_player->addComponent<CState>("straight");
	_player->addComponent<CInput>();
	_player->addComponent<CHealth>(100);
}

void Scene_Touhou::spawnSpellCard() {
	if (_player->hasComponent<CSpellCard>()) {
		size_t& ammo = _player->getComponent<CSpellCard>().spellCardCount;
		if (ammo > 0) {
			ammo -= 1;
			auto playerPos = _player->getComponent<CTransform>().pos;

			auto texture = backgroundToggle ? "WhiteSpell" : "BlackSpell";

			auto spellCard = _entityManager.addEntity("spellCard");

			auto bb = spellCard->addComponent<CAnimation>(Assets::getInstance()
				.getAnimation(texture)).animation.getBB();
			auto& animation = spellCard->getComponent<CAnimation>().animation;
			auto& sprite = animation.getSprite();

			bb.x *= 1.5;
			bb.y *= 1.5;
			float radius = std::min(bb.x, bb.y) / 2.f;
			std::cout << bb.x << " " << bb.y << "\n";

			auto direction = sf::Vector2f(0.f, 0.f);

			spellCard->addComponent<CBoundingBox>(radius, radius);
			spellCard->addComponent<CAutoPilot>();
			spellCard->addComponent<CTransform>(playerPos, direction);
		}
	}
}

void Scene_Touhou::spawnBullet(sf::Vector2f pos, bool isEnemy, const std::string& spriteName) {
	const float baseSpeed = _config.bulletSpeed;
	const sf::Vector2f center = _worldView.getCenter();

	// Determine speed and play sound
	const float speed = isEnemy ? baseSpeed : -baseSpeed;
	SoundPlayer::getInstance().play(isEnemy ? "EnemyGunfire" : "Damage01", center, 50.f);

	// Create bullet entity
	auto bullet = _entityManager.addEntity(isEnemy ? "EnemyBullet" : "PlayerBullet");
	const auto& animation = Assets::getInstance().getAnimation(spriteName);
	auto& animComp = bullet->addComponent<CAnimation>(animation).animation;
	auto bb = animComp.getBB();

	// Apply specific shape or size modifications
	if (spriteName == "LineShot" || spriteName == "LineShotBlack") {
		animComp.getSprite().setScale(1.f, 1.5f);
		bb.x /= 2;
		bb.y *= 1.3f;
		bullet->addComponent<CBoundingBox>(sf::Vector2f(bb.x, bb.y));
	}
	else if (spriteName == "ShotWhite" || spriteName == "ShotBlack") {
		bb.x /= 2;
		bb.y /= 2;
		bullet->addComponent<CBoundingBox>(std::min(bb.x, bb.y) / 2.f);
	}
	else {
		bullet->addComponent<CBoundingBox>(bb);
	}

	// Calculate direction
	sf::Vector2f direction(0.f, speed);
	auto& bossList = _entityManager.getEntities("bossEnemy");

	if (!isEnemy && _player->getComponent<CInput>().lshift && !bossList.empty()) {
		// Homing shot from player
		auto bossPos = bossList.front()->getComponent<CTransform>().pos;
		direction = normalize(bossPos - pos) * std::abs(speed);
	}
	else if (isEnemy && !bossList.empty()) {
		auto& bossGun = bossList.front()->getComponent<CGun>();

		if (bossGun.spreadLevel == 3) {
			auto playerPos = _player->getComponent<CTransform>().pos;
			direction = normalize(playerPos - pos) * std::abs(speed);
		}
		else if (bossGun.spreadLevel == 4) {
			direction = (pos.x < center.x) ? sf::Vector2f(speed, 0.f) : sf::Vector2f(-speed, 0.f);
		}
	}

	// Add remaining components
	bullet->addComponent<CTransform>(pos, direction);
	bullet->addComponent<CSpawnPosition>(pos);
}

void Scene_Touhou::sSpawnEnemies() {
	// spawn enemies when they are half a window above the current camera/view
	auto spawnLine = _worldView.getCenter().y - _game->window().getSize().y;

	while (!_spawnPoints.empty() && _spawnPoints.top().y > spawnLine) {
		spawnBoss(_spawnPoints.top());
		_spawnPoints.pop();
	}
}

void Scene_Touhou::spawnBoss(SpawnPoint sp) {
	auto bounds = getViewBounds();
	auto centerX = bounds.left + (bounds.right - bounds.left) / 2.f;

	sf::Vector2f pos{ centerX, bounds.top };
	auto bossEnemy = _entityManager.addEntity("bossEnemy");
	auto& tfm = bossEnemy->addComponent<CTransform>(pos, sf::Vector2f{ 0.f, _config.enemySpeed });
	auto& type = sp.type;
	type = "Boss";

	if (sp.type == "Boss")
	{
		auto& animation = bossEnemy->addComponent<CAnimation>(Assets::getInstance().getAnimation(sp.type)).animation;
		auto& sprite = animation.getSprite();

		sprite.setScale(1.7f, 1.7f);

		auto bb = animation.getBB();
		bb.x /= 2;
		bb.y /= 1.5;

		bossEnemy->addComponent<CBoundingBox>(bb);

		bossEnemy->addComponent<CHealth>(50000);
		bossEnemy->addComponent<CState>("BossInit");
	}
}

#pragma endregion

#pragma region Utilities

void Scene_Touhou::checkIfDead(sPtrEntt e) {
	std::uniform_int_distribution<int> flip(1, 2);

	if (e->hasComponent<CHealth>()) {
		if (e->getComponent<CHealth>().hp <= 0) {
			e->getComponent<CTransform>().vel = sf::Vector2f(0, 0);
			e->removeComponent<CHealth>();
			e->removeComponent<CBoundingBox>();
			if (e->getTag() == "player") {
				restartGame(_levelPath);
			}
			if (e->getTag() == "bossEnemy") {
				// Boss is killed, transition to main menu and reset game state
				_game->changeScene("MENU", nullptr, true);
				resetGameState();
				MusicPlayer::getInstance().play("menuTheme");
			}
		}
	}
}

void Scene_Touhou::sAutoPilot(sf::Time dt) {
	for (auto e : _entityManager.getEntities("spellCard")) {
		if (e->hasComponent<CAutoPilot>()) {
			auto& anim = e->getComponent<CAnimation>().animation;
			auto& sprite = anim.getSprite();
			auto& bb = e->getComponent<CBoundingBox>();

			// Increase the size of the sprite
			sf::Vector2f currentScale = sprite.getScale();
			sf::Vector2f scaleIncrement = sf::Vector2f(10.f, 10.f) * dt.asSeconds();
			sf::Vector2f newScale = currentScale + scaleIncrement;
			sprite.setScale(newScale);

			bb.size.x *= (1.f + scaleIncrement.x / currentScale.x);
			bb.size.y *= (1.f + scaleIncrement.y / currentScale.y);
			bb.halfSize = bb.size / 2.f;

			// Decrease the opacity of the sprite
			sf::Color color = sprite.getColor();
			color.a = static_cast<sf::Uint8>(std::max(0.f, color.a - 100.f * dt.asSeconds()));
			sprite.setColor(color);

			// De-spawn the sprite if it becomes fully transparent
			if (color.a == 0) {
				e->destroy();
			}
		}
	}
}

void Scene_Touhou::despawnAllBullets() {
	// Stop all bullets
	for (auto const& bullet : _entityManager.getEntities("EnemyBullet")) {
		auto pos = bullet->getComponent<CTransform>().pos;
		bullet->destroy();

		// Increase score
		_score += 10;
		_scoreText.setString("Score: " + std::to_string(_score));

		// Create "+10" text
		sf::Text text;
		text.setFont(Assets::getInstance().getFont("Arial"));
		text.setCharacterSize(20);
		if (backgroundToggle)
		{
			text.setFillColor(sf::Color::White);
			text.setOutlineColor(sf::Color::White);
		}
		else
		{
			text.setFillColor(sf::Color::Black);
			text.setOutlineColor(sf::Color::Black);
		}
		text.setString("+10");
		text.setPosition(pos);

		// Add to temporary texts
		_temporaryTexts.push_back({ text, sf::seconds(1) });
	}
}

#pragma endregion

#pragma region Bounds

sf::FloatRect Scene_Touhou::getBattlefieldBounds() const {
	auto& center = _worldView.getCenter();
	auto size = static_cast<sf::Vector2f>(_game->window().getSize());
	auto leftTop = center - size / 2.f;

	// buffer all around
	leftTop.y -= size.y / 2.f;
	size.y += size.y;
	leftTop.x -= size.x / 2.f;
	size.x += size.x;
	return sf::FloatRect(leftTop, size);
}

void Scene_Touhou::destroyOutsideBattlefieldBounds() {
	auto battleField = getBattlefieldBounds();
	for (auto e : _entityManager.getEntities()) {
		if (e->hasComponent<CTransform>() && e->hasComponent<CBoundingBox>()) {
			auto pos = e->getComponent<CTransform>().pos;
			if (!battleField.contains(pos)) {
				e->destroy();
			}
		}
	}
}

#pragma endregion

#pragma region Collisions

void Scene_Touhou::checkBulletCollision() {
	// Player Bullets
	for (auto& bullet : _entityManager.getEntities("PlayerBullet")) {
		for (auto& e : _entityManager.getEntities("bossEnemy")) {
			auto& gun = e->getComponent<CGun>();
			auto overlap = Physics::getOverlap(bullet, e);
			if (gun.cooldown > sf::Time::Zero) {
				continue;
			}
			else if (overlap.x > 0 && overlap.y > 0) {
				e->getComponent<CHealth>().hp -= 13;
				_score += 10;
				_scoreText.setString("Score: " + std::to_string(_score));
				bullet->destroy();
				checkIfDead(e);
			}
		}
	}

	// Enemy Bullets
	for (auto& bullet : _entityManager.getEntities("EnemyBullet")) {
		if (!_player->isActive()) {
			break;
		}
		auto overlap = Physics::getOverlap(_player, bullet);
		if (overlap.x > 0 && overlap.y > 0) {
			_player->getComponent<CHealth>().hp -= 10;
			bullet->destroy();
			if (_player->getComponent<CHealth>().hp <= 0) {
				_player->destroy();
				restartGame(_levelPath);
				return;
			}
		}
	}
}

void Scene_Touhou::checkSpellCardCollision() {
	// Spell Card and Enemy Bullet Collision
	for (auto& bullet : _entityManager.getEntities("EnemyBullet")) {
		for (auto& s : _entityManager.getEntities("spellCard")) {
			auto overlap1 = Physics::getOverlap(s, bullet);
			if (overlap1.x > 0 && overlap1.y > 0) {
				std::cout << "Bullet destroyed\n";
				bullet->destroy();
			}
		}
	}

	// Spell Card and Boss Enemy Collision
	for (auto& bullet : _entityManager.getEntities("spellCard")) {
		for (auto& e : _entityManager.getEntities("bossEnemy")) {
			auto& gun = e->getComponent<CGun>();
			auto overlap = Physics::getOverlap(bullet, e);
			if (gun.cooldown > sf::Time::Zero) {
				continue;
			}
			else if (overlap.x > 0 && overlap.y > 0) {
				e->getComponent<CHealth>().hp -= 20;
				_score += 5;
				_scoreText.setString("Score: " + std::to_string(_score));
				checkIfDead(e);
			}
		}
	}
}
#pragma endregion

#pragma region Systems

void Scene_Touhou::sCollisions() {
	checkSpellCardCollision();
	checkBulletCollision();
}

void Scene_Touhou::sAnimation(sf::Time dt) {
	for (auto e : _entityManager.getEntities()) {
		// update all animations
		if (e->getComponent<CAnimation>().has) {
			auto& anim = e->getComponent<CAnimation>();
			anim.animation.update(dt);

			if (anim.animation.hasEnded()) { // for explosion
				e->destroy();
			}
		}
	}
}

#pragma endregion