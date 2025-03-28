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

bool firstTimePassing1200 = true;
bool amountOfBullets = false;
bool shooting = false;
bool isSpread4 = false;
int lastCheckedHP = 50000;

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

std::string _levelPath;
float getRandomFloat(float min, float max) {
	std::uniform_real_distribution<float> dis(min, max);
	return dis(rng);
}

Scene_Touhou::Scene_Touhou(GameEngine* gameEngine, const std::string& levelPath)
	: Scene(gameEngine), _worldView(gameEngine->window().getDefaultView()) {
	init(levelPath);
}

void Scene_Touhou::init(const std::string& levelPath) {
	_levelPath = levelPath;
	loadLevel(levelPath);
	registerActions();

	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto top = center.y - viewHalfSize.y;
	auto bot = center.y + viewHalfSize.y;

	auto centerX = left + (right - left) / 2.f;
	sf::Vector2f bottomPos{ centerX, bot };

	sf::Vector2f spawnPos{ _worldView.getSize().x / 2.f, top - _worldView.getSize().y / 2.f };

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
	_livesText.setString("Lives: 3");

	_spellCardsText.setFont(Assets::getInstance().getFont("Arial"));
	_spellCardsText.setCharacterSize(20);
	_spellCardsText.setFillColor(sf::Color::White);
	_spellCardsText.setString("Spell Cards: 0");

	_parryText.setFont(Assets::getInstance().getFont("Arial"));
	_parryText.setCharacterSize(20);
	_parryText.setFillColor(sf::Color::White);
	_parryText.setString("Parry: 0");

	_backgroundCooldownText.setFont(Assets::getInstance().getFont("Arial"));
	_backgroundCooldownText.setCharacterSize(20);
	_backgroundCooldownText.setFillColor(sf::Color::White);
	_backgroundCooldownText.setString("Switch Background CD: Ready~");

	//Circle cleanup
	m_expandingCircle.setFillColor(sf::Color::Transparent);
	m_expandingCircle.setOutlineColor(sf::Color::Red);
	m_expandingCircle.setOutlineThickness(2.f);
	m_expandingCircle.setRadius(0.f);
}

void Scene_Touhou::update(sf::Time dt) {
	if (!_isPaused) {
		sUpdate(dt);
	}
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
		else if (command.name() == "LAUNCH") { spawnMisille(); }
		else if (command.name() == "GRAZE") {
			playerSize(true);
			_player->getComponent<CInput>().lshift = true;
		}
		else if (command.name() == "SWITCH") {
			if (_backgroundSwitchCooldown <= sf::Time::Zero) {
				backgroundToggle = !backgroundToggle;
				_backgroundSwitchCooldown = sf::seconds(10);

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

	if (e->hasComponent<CMissiles>()) {
		int count = e->getComponent<CMissiles>().missileCount;
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

void Scene_Touhou::sRender() {
	_game->window().setView(_worldView);
	_game->window().clear(sf::Color::White);

	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto top = center.y - viewHalfSize.y;
	auto bot = center.y + viewHalfSize.y;

	// Position UI elements to the right of the background
	float uiX = right + 20.f;
	float uiY = top + 20.f;

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

			_parryText.setPosition(uiX, uiY + 90.f);
			_parryText.setFillColor(sf::Color::Black);
			_game->window().draw(_parryText);

			_backgroundCooldownText.setPosition(uiX, uiY + 120.f);
			_backgroundCooldownText.setFillColor(sf::Color::Black);
			_game->window().draw(_backgroundCooldownText);

			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				sprite.setPosition(left, top);
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

			_parryText.setPosition(uiX, uiY + 90.f);
			_parryText.setFillColor(sf::Color::White);
			_game->window().draw(_parryText);

			_backgroundCooldownText.setPosition(uiX, uiY + 120.f);
			_backgroundCooldownText.setFillColor(sf::Color::White);
			_game->window().draw(_backgroundCooldownText);

			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				sprite.setPosition(left, top);

				sprite.setScale((right - left) / sprite.getTexture()->getSize().x, (bot - top) / sprite.getTexture()->getSize().y);
				_game->window().draw(sprite);
			}
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

	for (auto& e : _entityManager.getEntities("bossEnemy")) {
		auto& health = e->getComponent<CHealth>();
		float healthPercentage = static_cast<float>(health.hp) / 50000.f;
		float healthBarWidth = backgroundBounds.width * 0.9f * healthPercentage;
		sf::RectangleShape healthBar(sf::Vector2f(100.f, 20.f));
		healthBar.setFillColor(sf::Color::Red);
		healthBar.setPosition(center.x, top - 30.f);
		_game->window().draw(healthBar);
		drawEntt(e);
		drawAABB(e);
		drawHP(e);
		drawAmmo(e);
	}

	// draw all entities
	for (auto& e : _entityManager.getEntities()) {
		if (!e->hasComponent<CAnimation>() || e->getTag() == "bkg" || e->getTag() == "Pickup" || e->getTag() == "PlayerBullet" || e->getTag() == "EnemyBullet" || e->getTag() == "bossEnemy")
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

	if (m_isExpandingCircleActive) {
		// Calculate the intersection of the expanding circle with the background bounds
		sf::FloatRect circleBounds(
			m_expandingCircle.getPosition().x - m_expandingCircle.getRadius(),
			m_expandingCircle.getPosition().y - m_expandingCircle.getRadius(),
			m_expandingCircle.getRadius() * 2,
			m_expandingCircle.getRadius() * 2
		);

		if (backgroundBounds.intersects(circleBounds)) {
			_game->window().draw(m_expandingCircle);
		}
	}
}

void Scene_Touhou::drawPauseOverlay() {
	auto size = _game->window().getSize();

	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto top = center.y - viewHalfSize.y;
	auto bot = center.y + viewHalfSize.y;

	_game->window().setView(_worldView); // Set the current view

	sf::RectangleShape overlay(sf::Vector2f(right - left, bot - top));
	overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Semi-transparent black
	overlay.setPosition(left, top);
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

	registerAction(sf::Keyboard::M, "LAUNCH");
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
	registerAction(sf::Keyboard::Z, "SELECT");
}

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
	// don't adjust position if dead
	if (_player->getComponent<CState>().state == "dead")
		return;

	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto top = center.y - viewHalfSize.y;
	auto bot = center.y + viewHalfSize.y;

	auto& player_pos = _player->getComponent<CTransform>().pos;
	auto halfSize = _player->getComponent<CBoundingBox>().halfSize;

	// keep player in bounds
	player_pos.x = std::max(player_pos.x, left + halfSize.x);
	player_pos.x = std::min(player_pos.x, right - halfSize.x);
	player_pos.y = std::max(player_pos.y, top + halfSize.y);
	player_pos.y = std::min(player_pos.y, bot - halfSize.y);
}

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

void Scene_Touhou::sMovement(sf::Time dt) {
	bulletMovementTimer -= dt;
	if (bulletMovementTimer <= sf::Time::Zero) {
		bulletsMoving = !bulletsMoving;
		bulletMovementTimer = sf::seconds(3);
	}

	// Add a flag to track if the player position has been retrieved
	bool playerPosRetrieved = false;
	sf::Vector2f playerPos;
	bool bossHasPassed1200 = false;
	int bossSpreadLevel = 1;

	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);

	playerMovement();
	animatePlayer();

	for (auto e : _entityManager.getEntities()) {
		if (e->getTag() == "bossEnemy" && e->hasComponent<CGun>()) {
			bossSpreadLevel = e->getComponent<CGun>().spreadLevel;
			break;
		}
	}

	// move all objects
	for (auto e : _entityManager.getEntities()) {
		if (e->hasComponent<CTransform>()) {
			auto& tfm = e->getComponent<CTransform>();

			auto boss = e->getTag() == "bossEnemy";
			auto eBullet = e->getTag() == "EnemyBullet";
			auto pBullet = e->getTag() == "PlayerBullet";

			if (eBullet)
			{
				if (bossSpreadLevel == 5) {
					int bulletCount = _entityManager.getEntities("EnemyBullet").size();
					if (bulletCount <= 100)
					{
						tfm.vel.y = 0;
						tfm.pos.y += tfm.vel.y * dt.asSeconds();
					}
					else
					{
						tfm.vel.y = 200.f;
						tfm.pos.y += tfm.vel.y * dt.asSeconds();
					}
				}
				else if (bossSpreadLevel == 4) {
					if (bulletsMoving) {
						tfm.vel.y = 0.f;
						tfm.pos.y += (tfm.vel.y * 0.1) * dt.asSeconds();
						tfm.pos.x += (tfm.vel.x * 0.1) * dt.asSeconds();
					}
					else {
						tfm.vel.y = 200.f;
						tfm.pos.y += (tfm.vel.y * 0.15) * dt.asSeconds();
						tfm.pos.x += (tfm.vel.x * 0) * dt.asSeconds();
					}
				}
				else if (bossSpreadLevel == 3) {
					tfm.pos.y += (tfm.vel.y * 0.5) * dt.asSeconds();
					tfm.pos.x += (tfm.vel.x * 0.5) * dt.asSeconds();
				}
				else if (bossSpreadLevel == 2) {
					tfm.pos.y += (tfm.vel.y * 0.5) * dt.asSeconds();
				}
				else
				{
					tfm.pos.y += (tfm.vel.y * 0.7) * dt.asSeconds();
				}
			}
			else if (boss)
			{
				if (tfm.pos.y < center.y - 300.f && !bossHasPassed1200) {
					tfm.pos += tfm.vel * dt.asSeconds();
					tfm.angle += tfm.angVel * dt.asSeconds();
				}
				else if (!bossHasPassed1200) {
					bossHasPassed1200 = true;
					if (firstTimePassing1200) {
						e->addComponent<CGun>();
						tfm.vel.x += 25.f;
						firstTimePassing1200 = false;
						auto gun = _player->addComponent<CGun>();
						gun.countdown = sf::Time::Zero;
						gun.spreadLevel = 0;
						gun.isFiring = true;
						gun.fireRate = 65;
					}
					//const auto updatedX = tfm.vel.x + 25.f;
					tfm.pos.y += tfm.vel.y * dt.asSeconds();
					tfm.pos.x += tfm.vel.x * dt.asSeconds();
					tfm.angle += tfm.angVel * dt.asSeconds();

					if (tfm.pos.x < left || tfm.pos.x > right) {
						tfm.vel.x = -tfm.vel.x;
					}
					if (tfm.pos.y < center.y - 300.f || tfm.pos.y > center.y - 100.f) {
						tfm.vel.y = -tfm.vel.y;
					}

					// Clamp to ensure boss stays within range
					tfm.pos.x = std::clamp(tfm.pos.x, left, right);
					tfm.pos.y = std::clamp(tfm.pos.y, center.y - 300.f, center.y - 100.f);
				}
			}
			else if (pBullet)
			{
				tfm.pos += tfm.vel * dt.asSeconds();
				tfm.angle = 90 + bearing(tfm.vel);
				tfm.angle += tfm.angVel * dt.asSeconds();
			}
			else {
				tfm.pos += tfm.vel * dt.asSeconds();
				tfm.angle += tfm.angVel * dt.asSeconds();
			}
		}
	}
}

void Scene_Touhou::sCollisions() {
	checkMissileCollision();
	checkPlaneCollision();
	checkBulletCollision();
	checkPickupCollision();
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
	sGuideMissiles(dt);
	destroyOutsideBattlefieldBounds();

	if (_backgroundSwitchCooldown > sf::Time::Zero) {
		_backgroundSwitchCooldown -= dt;
		int cooldownSeconds = static_cast<int>(std::ceil(_backgroundSwitchCooldown.asSeconds()));
		_backgroundCooldownText.setString("Switch Background CD: " + std::to_string(cooldownSeconds) + "s");
	}
	else {
		_backgroundCooldownText.setString("Switch Background CD: READY~");
	}
}

void Scene_Touhou::onEnd() {
	_game->changeScene("MENU", nullptr, true);
	resetGameState();
	MusicPlayer::getInstance().play("menuTheme");
}

void Scene_Touhou::spawnMisille() {
	if (_player->hasComponent<CMissiles>()) {
		size_t& ammo = _player->getComponent<CMissiles>().missileCount;
		if (ammo > 0) {
			ammo -= 1;
			auto pos = _player->getComponent<CTransform>().pos;

			auto missile = _entityManager.addEntity("missile");
			missile->addComponent<CTransform>(
				pos + sf::Vector2f(0.f, -60.f),
				sf::Vector2f(0.f, -_config.missileSpeed));

			auto bb = missile->addComponent<CAnimation>(Assets::getInstance()
				.getAnimation("Missile")).animation.getBB();

			missile->addComponent<CBoundingBox>(bb);
			SoundPlayer::getInstance().play("LaunchMissile", pos, 0);

			SoundPlayer::getInstance().play("LaunchMissile", pos, 0);
		}
	}
}

void Scene_Touhou::fireBullet() {
	if (_player->hasComponent<CGun>())
		_player->getComponent<CGun>().isFiring = true;
}

void Scene_Touhou::sGunUpdate(sf::Time dt) {
	static sf::Time spreadChangeTimer = sf::seconds(5.f + (std::rand() % 6));
	static sf::Time bulletSpawnTimer = sf::Time::Zero;
	static sf::Time columnSpawnTimer = sf::Time::Zero;
	static int bulletIndex = 0;
	static int currentColumn = 0;
	std::string bulletTexture = backgroundToggle ? "ShotWhite" : "ShotBlack";
	std::string lineTexture = backgroundToggle ? "LineShot" : "LineShotBlack";

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
					std::uniform_int_distribution<int> dist(2, 5);
					gun.spreadLevel = dist(rng);
					std::cout << "New spread level: " << gun.spreadLevel << "\n";

					lastCheckedHP = bossCurrentHP;

					despawnAllBullets();
					gun.cooldown = sf::seconds(5);
				}
			}

			if (gun.cooldown > sf::Time::Zero) {
				continue;
			}

			if (isEnemy || isBossEnemy)
				gun.isFiring = true;

			//
			// when firing
			//
			if (gun.isFiring && gun.countdown <= sf::Time::Zero) {
				gun.isFiring = false;
				gun.countdown = _config.fireInterval / (1.f + gun.fireRate);

				auto pos = e->getComponent<CTransform>().pos;
				switch (gun.spreadLevel) {
				case 0:
					if (isPlayer)
					{
						gun.fireRate = 65;
						if (!_player->getComponent<CInput>().lshift) {
							spawnBullet(pos + sf::Vector2f(-20.f, 0.f), isBossEnemy, "WhiteKnife");
							spawnBullet(pos + sf::Vector2f(20.f, 0.f), isBossEnemy, "WhiteKnife");
						}
					}
					else
					{
						gun.fireRate = gun.originalFireRate;
						spawnBullet(pos + sf::Vector2f(-20.f, 0.f), isBossEnemy, bulletTexture);
						spawnBullet(pos + sf::Vector2f(20.f, 0.f), isBossEnemy, bulletTexture);
					}
					break;
				case 1:
					if (isPlayer)
					{
						if (_player->getComponent<CInput>().lshift) {
							gun.fireRate = 60;
							spawnBullet(pos + sf::Vector2f(0.f, 0.f), isBossEnemy, "WhiteKnife");
						}
					}
					break;

				case 2:
					if (isBossEnemy) {
						gun.fireRate = gun.originalFireRate;
						bulletSpawnTimer = sf::Time::Zero;
						bulletIndex = 0;
						spawnBullet(pos + sf::Vector2f(0.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-65.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-130.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-195.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-260.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-325.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-390.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-455.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-520.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-585.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-650.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-715.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-780.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(-845.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(65.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(130.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(195.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(260.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(325.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(390.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(455.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(520.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(585.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(650.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(715.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(780.f, -500.f), isBossEnemy, lineTexture);
						spawnBullet(pos + sf::Vector2f(845.f, -500.f), isBossEnemy, lineTexture);
					}
					break;

				case 3:
				{
					if (isBossEnemy) {
						gun.fireRate = gun.originalFireRate;
						bulletSpawnTimer = sf::Time::Zero;
						bulletIndex = 0;
						float circleRadius = 50.f;
						int bulletsPerCircle = 10;
						float angleStep = 360.f / bulletsPerCircle; // Angle between each bullet

						// Positions for the 3 circles in a triangular shape
						sf::Vector2f circleOffsets[3] = {
							sf::Vector2f(0.f, 100.f), // top circle
							sf::Vector2f(-150.f, -100.f),  // bottom right circle
							sf::Vector2f(150.f, -100.f)  // bottom left circle
						};

						// Loop through each circle and spawn bullets in a circular pattern
						for (int i = 0; i < 3; ++i) {
							sf::Vector2f circleCenter = pos + circleOffsets[i];

							// Spawn 10 bullets per circle
							for (int j = 0; j < bulletsPerCircle; ++j) {
								float angle = j * angleStep;  // Angle for each bullet
								sf::Vector2f bulletPos = circleCenter + sf::Vector2f(
									std::cos(degToRad(angle)) * circleRadius,  // X offset for the bullet
									std::sin(degToRad(angle)) * circleRadius   // Y offset for the bullet
								);

								spawnBullet(bulletPos, isBossEnemy, bulletTexture);
							}
						}
					}
				}
				break;

				case 4:
				{
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
								// Left side bullets
								sf::Vector2f bulletPos = sf::Vector2f(left - 100.f, y);
								spawnBullet(bulletPos, isBossEnemy, bulletTexture);

								// Right side bullets
								bulletPos = sf::Vector2f(right + 100.f, y - 50.f);
								spawnBullet(bulletPos, isBossEnemy, bulletTexture);
							}

							currentColumn++;
						}
					}
				}
				break;
				case 5:
				{
					isSpread4 = true;
					if (_entityManager.getEntities("EnemyBullet").size() <= 500) {
						if (isBossEnemy) {
							gun.fireRate = 50;
							// Semi - circle
							const int numBullets = 20;

							const float angleStep = 360.f / (numBullets - 1);

							const float radius = 250.f;

							if (bulletSpawnTimer <= sf::Time::Zero) {
								float angle = bulletIndex * angleStep;

								sf::Vector2f bulletPos = pos + sf::Vector2f(std::cos(degToRad(angle)) * radius,
									std::sin(degToRad(angle)) * radius);
								bulletPos.y -= 500.f;

								spawnBullet(bulletPos, isBossEnemy, bulletTexture);

								bulletPos.x += 500.f;

								spawnBullet(bulletPos, isBossEnemy, bulletTexture);

								bulletPos.x -= 1000.f;

								spawnBullet(bulletPos, isBossEnemy, bulletTexture);
								bulletSpawnTimer = sf::seconds(0.1f);

								//sf::Vector2f bulletPos2 = pos + sf::Vector2f(std::cos(degToRad(angle)) * radius,
									//std::sin(degToRad(-angle)) * radius);
								//bulletPos2.y -= 500.f;

								//spawnBullet(bulletPos2, isBossEnemy, bulletTexture);

								bulletIndex++;
								if (bulletIndex >= numBullets) {
									bulletIndex = 0;
									gun.isFiring = false;
								}
							}
						}
					}
					else {
						isSpread4 = false;
						gun.isFiring = false;
					}
					break;
				}

				default:
					std::cerr << "Bad spread level firing gun\n";
					break;
				}
			}
		}
	}
}

void Scene_Touhou::spawnBullet(sf::Vector2f pos, bool isEnemy, const std::string& spriteName) {
	float speed;
	sf::Vector2f center = _worldView.getCenter();
	if (isEnemy) {
		speed = _config.bulletSpeed;
		SoundPlayer::getInstance().play("EnemyGunfire", center, 50.f);
	}
	else {
		speed = -_config.bulletSpeed;
		SoundPlayer::getInstance().play("Damage01", center, 50.f);
	}

	auto bullet = _entityManager.addEntity(isEnemy ? "EnemyBullet" : "PlayerBullet");
	auto& animationName = spriteName;
	auto bb = bullet->addComponent<CAnimation>(Assets::getInstance().getAnimation(animationName)).animation.getBB();
	if (spriteName == "LineShot" || spriteName == "LineShotBlack") {
		auto& animation = bullet->addComponent<CAnimation>(Assets::getInstance().getAnimation(animationName)).animation;
		auto& sprite = animation.getSprite();

		sprite.setScale(1.f, 1.5f);

		bb.x /= 2;
		bb.y *= 1.3;
		bullet->addComponent<CBoundingBox>(sf::Vector2f(bb.x, bb.y));
	}
	else if (spriteName == "ShotWhite" || spriteName == "ShotBlack") {
		bb.x /= 2;
		bb.y /= 2;
		float radius = std::min(bb.x, bb.y) / 2.f;
		bullet->addComponent<CBoundingBox>(radius);
	}
	else {
		bullet->addComponent<CBoundingBox>(bb);
	}

	sf::Vector2f direction(0.f, speed);

	auto boss = _entityManager.getEntities("bossEnemy");
	if (!isEnemy && _player->getComponent<CInput>().lshift == true) {
		if (!boss.empty()) {
			auto bossPos = boss.front()->getComponent<CTransform>().pos;
			direction = normalize(bossPos - pos) * std::abs(speed);
		}
	}
	else if (isEnemy && !boss.empty() && boss.front()->getComponent<CGun>().spreadLevel == 3) {
		auto playerPos = _player->getComponent<CTransform>().pos;
		direction = normalize(playerPos - pos) * std::abs(speed);
	}
	else if (isEnemy && !boss.empty() && boss.front()->getComponent<CGun>().spreadLevel == 4) {
		if (pos.x < center.x) {
			direction = sf::Vector2f(speed, 0.f);
		}
		else {
			direction = sf::Vector2f(-speed, 0.f);
		}
	}

	bullet->addComponent<CTransform>(pos, direction);
	bullet->addComponent<CSpawnPosition>(pos);
}

void Scene_Touhou::despawnAllBullets() {
	auto boss = _entityManager.getEntities("bossEnemy").front();
	auto bossPos = boss->getComponent<CTransform>().pos;

	m_expandingCircle.setPosition(bossPos);
	m_expandingCircle.setRadius(0.f);
	m_isExpandingCircleActive = true;

	// Stop all bullets
	for (auto const& bullet : _entityManager.getEntities("EnemyBullet")) {
		bullet->getComponent<CTransform>().vel = sf::Vector2f(0.f, 0.f);
	}
}

void Scene_Touhou::sSpawnEnemies() {
	// spawn enemies when they are half a window above the current camera/view
	auto spawnLine = _worldView.getCenter().y - _game->window().getSize().y;

	while (!_spawnPoints.empty() && _spawnPoints.top().y > spawnLine) {
		spawnBoss(_spawnPoints.top());
		spawnEnemyPlanes(_spawnPoints.top());
		_spawnPoints.pop();
	}
}

void Scene_Touhou::spawnBoss(SpawnPoint sp) {
	auto& center = _worldView.getCenter();
	sf::Vector2f viewHalfSize = _game->windowSize() / 2.f;

	auto top = center.y - viewHalfSize.y;
	auto left = center.x - (viewHalfSize.x / 2.f);
	auto right = center.x + (viewHalfSize.x / 3.f);
	auto bot = center.y + viewHalfSize.y;

	auto centerX = left + (right - left) / 2.f;

	sf::Vector2f pos{ centerX, top };
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
	}
}

void Scene_Touhou::spawnEnemyPlanes(SpawnPoint sp) {
	std::uniform_int_distribution numPlanes(2, 5);
	int number = numPlanes(rng);
	sf::Vector2f pos{ 0.f, sp.y };
	auto width = _worldBounds.width;
	auto spacer = width / (number + 1);

	for (int i{ 1 }; i <= number; ++i) {
		pos.x = i * spacer;
		auto enemyPlane = _entityManager.addEntity("enemy");
		auto& tfm = enemyPlane->addComponent<CTransform>(pos, sf::Vector2f{ 0.f, _config.enemySpeed });
		tfm.angle = 180;

		if (sp.type == "Avenger")
		{
			enemyPlane->addComponent<CGun>();

			auto bb = enemyPlane->
				addComponent<CAnimation>(Assets::getInstance().getAnimation(sp.type)).animation.getBB();

			enemyPlane->addComponent<CBoundingBox>(bb);
			enemyPlane->addComponent<CHealth>(100);
			enemyPlane->addComponent<CAutoPilot>();
		}
	}
}

void Scene_Touhou::sAutoPilot(sf::Time dt) {
	for (auto e : _entityManager.getEntities("enemy")) {
		if (e->hasComponent<CAutoPilot>()) {
			auto& ai = e->getComponent<CAutoPilot>();
			ai.countdown -= dt;
			if (ai.countdown < sf::Time::Zero) {
				ai.currentLeg = (ai.currentLeg + 1) % autopilot_directions.size();
				ai.countdown = autopilot_directions[ai.currentLeg].dt;
				auto& tfm = e->getComponent<CTransform>();
				if (e->getTag() == "bossEnemy") {
					auto& tfmBoss = e->getComponent<CTransform>();
					// Only move randomly if below y = 1200.f
					if (tfmBoss.pos.y < 1200.f) {
						// Generate a random angle between -30 to 30 degrees
						float randomAngle = getRandomFloat(-30.f, 30.f);

						// Set velocity based on the new random movement
						tfmBoss.vel = length(tfmBoss.vel) * uVecBearing(90 + autopilot_directions[ai.currentLeg].bearing + randomAngle);
					}
				}
				else {
					tfm.vel = length(tfm.vel) * uVecBearing(90 + autopilot_directions[ai.currentLeg].bearing);
				}
			}
		}
	}
}

sf::FloatRect Scene_Touhou::getBattlefieldBounds() const {
	auto center = _worldView.getCenter();
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

sf::Vector2f Scene_Touhou::findClosestEnemy(sf::Vector2f mPos) {
	float closest = std::numeric_limits<float>::max();
	sf::Vector2f posClosest{ 0.f, 0.f };
	for (auto& e : _entityManager.getEntities("enemy")) {
		if (e->getComponent<CTransform>().has) {
			auto ePos = e->getComponent<CTransform>().pos;
			float distToEnemy = dist(mPos, ePos);
			if (distToEnemy < closest) {
				closest = distToEnemy;
				posClosest = ePos;
			}
		}
	}
	return posClosest;
}

void Scene_Touhou::sGuideMissiles(sf::Time dt) {
	const float approachRate = 500.f;
	for (auto e : _entityManager.getEntities("missile")) {
		if (e->getComponent<CTransform>().has) {
			auto& tfm = e->getComponent<CTransform>();
			auto ePos = findClosestEnemy(tfm.pos);

			auto targetDir = normalize(ePos - tfm.pos);
			tfm.vel = _config.missileSpeed * normalize(approachRate * dt.asSeconds() * targetDir + tfm.vel);
			tfm.angle = bearing(tfm.vel) + 90;
		}
	}
}

void Scene_Touhou::checkPickupCollision() {
	for (auto e : _entityManager.getEntities("Pickup")) {
		// player collides with pickup;
		auto overlap = Physics::getOverlap(_player, e);
		if (overlap.x > 0 && overlap.y > 0) {
			auto pickupType = e->getComponent<CState>().state;
			if (pickupType == "HealthRefill") _player->getComponent<CHealth>().hp += 50;
			if (pickupType == "FireRate") {
				auto& rate = _player->getComponent<CGun>().fireRate;
				if (rate < 10)
					rate += 1;
			}
			if (pickupType == "FireSpread") {
				auto& spread = _player->getComponent<CGun>().spreadLevel;
				if (spread < 3)
					spread += 1;
			}
			if (pickupType == "MissileRefill") _player->getComponent<CMissiles>().missileCount += 2;
			e->destroy();
		}
	}
}

void Scene_Touhou::checkPlaneCollision() {
	for (auto e : _entityManager.getEntities("enemy")) {
		// planes have collided
		auto overlap = Physics::getOverlap(_player, e);
		if (overlap.x > 0 && overlap.y > 0) {
			auto& pHP = _player->getComponent<CHealth>().hp;
			auto& eHP = e->getComponent<CHealth>().hp;

			// however many HP the enemy plane has left,
			// that's how much damage it inflicts on players plane
			int tmpHP = pHP;
			pHP -= eHP;
			eHP -= tmpHP;

			checkIfDead(e);
			checkIfDead(_player);
		}
	}
}

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
				e->getComponent<CHealth>().hp -= 100;
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

void Scene_Touhou::checkMissileCollision() {
	// Player Missile collision
	for (auto& m : _entityManager.getEntities("missile")) {
		for (auto& e : _entityManager.getEntities("enemy")) {
			auto overlap = Physics::getOverlap(m, e);
			if (overlap.x > 0 && overlap.y > 0) {
				e->getComponent<CHealth>().hp = -10;
				m->destroy();
				checkIfDead(e);
				return;
			}
		}
	}
}

void Scene_Touhou::startAnimation(sPtrEntt e, std::string animation) {
}

void Scene_Touhou::checkIfDead(sPtrEntt e) {
	std::uniform_int_distribution<int> flip(1, 2);

	// when plane entities dies run an explosion animation before destroying the entity
	if (e->hasComponent<CHealth>()) {
		if (e->getComponent<CHealth>().hp <= 0) {
			e->getComponent<CTransform>().vel = sf::Vector2f(0, 0);
			e->removeComponent<CHealth>();
			e->removeComponent<CBoundingBox>();
			if (e->getTag() == "enemy") {
				dropPickup(e->getComponent<CTransform>().pos);
			}
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

void Scene_Touhou::resetGameState() {
	for (auto const& e : _entityManager.getEntities()) {
		e->destroy();
	}
	_entityManager.update();
	firstTimePassing1200 = true;
	amountOfBullets = false;
	shooting = false;
	isSpread4 = false;
	lastCheckedHP = 50000;
	_isPaused = false;
}

void Scene_Touhou::restartGame(const std::string& levelPath) {
	resetGameState();
	init(levelPath);
}

void Scene_Touhou::dropPickup(sf::Vector2f pos) {
	// todo

	static const std::string pickups[] =
	{ "FireRate", "FireSpread", "HealthRefill", "MissileRefill" };

	std::uniform_int_distribution<int> d1(1, 3);  // chance drop a pickup
	std::uniform_int_distribution<int> d2(0, 3);  // which pickup to drop

	if (d1(rng) < 4)  // 2/3 chance to drop a pickup
	{
		auto pickupType = pickups[d1(rng)];
		auto p = _entityManager.addEntity("Pickup");
		p->addComponent<CTransform>(pos);
		auto bb = p->addComponent<CAnimation>(Assets::getInstance()
			.getAnimation(pickupType)).animation.getBB();
		p->addComponent<CBoundingBox>(bb);
		p->addComponent<CState>().state = pickupType;
	}
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

	if (m_isExpandingCircleActive) {
		float radius = m_expandingCircle.getRadius();
		radius += m_expandingCircleSpeed * dt.asSeconds();
		m_expandingCircle.setRadius(radius);
		m_expandingCircle.setOrigin(radius, radius);

		float outlineThickness = m_expandingCircle.getOutlineThickness();

		// Check for collisions with bullets
		for (auto const& bullet : _entityManager.getEntities("EnemyBullet")) {
			auto bulletPos = bullet->getComponent<CTransform>().pos;
			float distance = dist(m_expandingCircle.getPosition(), bulletPos);
			if (distance >= radius - outlineThickness && distance <= radius + outlineThickness) {
				bullet->destroy();
			}
		}

		// Stop the animation if the circle is too large
		if (radius > _worldView.getSize().x) {
			m_isExpandingCircleActive = false;
		}
	}
}