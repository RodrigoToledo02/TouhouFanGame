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
	loadLevel(levelPath);
	registerActions();

	sf::Vector2f spawnPos{ _worldView.getSize().x / 2.f, _worldBounds.height - _worldView.getSize().y / 2.f };

	_worldView.setCenter(spawnPos);

	spawnPlayer(spawnPos);

	MusicPlayer::getInstance().play("gameTheme");
	MusicPlayer::getInstance().setVolume(15);
}

void Scene_Touhou::update(sf::Time dt) {
	sUpdate(dt);
}

void Scene_Touhou::sDoAction(const Command& command) {
	// On Key Press
	if (command.type() == "START") {
		if (command.name() == "PAUSE") { setPaused(!_isPaused); }
		else if (
			command.name() == "QUIT") {
			_game->quitLevel();
		}
		else if (
			command.name() == "BACK") {
			_game->backLevel();
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
			backgroundToggle = !backgroundToggle;
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
	bb.isSmall = set;
}

void Scene_Touhou::drawAABB(std::shared_ptr<Entity> e) {
	if (_drawAABB) {
		auto box = e->getComponent<CBoundingBox>();
		if (box.isSmall) box.size *= .5f;
		sf::RectangleShape rect;
		rect.setSize(sf::Vector2f{ box.size.x, box.size.y });
		centerOrigin(rect);
		rect.setPosition(e->getComponent<CTransform>().pos);
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color{ 0, 255, 0 });
		rect.setOutlineThickness(2.f);
		_game->window().draw(rect);
	}
}

void Scene_Touhou::drawCameraView() {
	if (_drawCam) {
		auto& pos = _worldView.getCenter();
		auto size = _game->window().getSize();
		sf::RectangleShape rect;
		rect.setSize(sf::Vector2f(size.x - 10, size.y - 10));
		centerOrigin(rect);
		rect.setPosition(pos);
		rect.setFillColor(sf::Color(0, 0, 0, 0));
		rect.setOutlineColor(sf::Color{ 0, 0, 0 });
		rect.setOutlineThickness(8.f);
		_game->window().draw(rect);
	}
}

void Scene_Touhou::drawHP(sPtrEntt e) {
	sf::Text text = sf::Text("HP: ", Assets::getInstance().getFont("Arial"), 15);
	if (e->hasComponent<CHealth>()) {
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

	// draw bkg first
	if (backgroundToggle) {
		for (auto& e : _entityManager.getEntities("bkg")) {
			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				_game->window().draw(sprite);
			}
		}
	}
	else {
		for (auto& e : _entityManager.getEntities("bkg1")) {
			if (e->getComponent<CSprite>().has) {
				auto& sprite = e->getComponent<CSprite>().sprite;
				_game->window().draw(sprite);
			}
		}
	}

	// draw pickups
	for (auto& e : _entityManager.getEntities("Pickup")) {
		drawEntt(e);
		drawAABB(e);
	}

	// draw all entities
	for (auto& e : _entityManager.getEntities()) {
		if (!e->hasComponent<CAnimation>() || e->getTag() == "bkg" || e->getTag() == "Pickup")
			continue;
		drawEntt(e);
		drawAABB(e);
		drawHP(e);
		drawAmmo(e);
	}
	drawCameraView();
}

void Scene_Touhou::registerActions() {
	registerAction(sf::Keyboard::Z, "ZOOMOUT");
	registerAction(sf::Keyboard::X, "ZOOMIN");

	registerAction(sf::Keyboard::P, "PAUSE");
	registerAction(sf::Keyboard::Escape, "BACK");
	registerAction(sf::Keyboard::Q, "QUIT");

	registerAction(sf::Keyboard::C, "TOGGLE_COLLISION");
	registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
	registerAction(sf::Keyboard::V, "TOGGLE_CAMOUTLINE");

	registerAction(sf::Keyboard::M, "LAUNCH");
	registerAction(sf::Keyboard::LShift, "GRAZE");
	registerAction(sf::Keyboard::N, "SWITCH");

	registerAction(sf::Keyboard::A, "LEFT");
	registerAction(sf::Keyboard::Left, "LEFT");
	registerAction(sf::Keyboard::D, "RIGHT");
	registerAction(sf::Keyboard::Right, "RIGHT");
	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::Up, "UP");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Down, "DOWN");
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
	auto& gun = _player->addComponent<CGun>();
	gun.fireRate = -2;
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

	auto left = center.x - viewHalfSize.x;
	auto right = center.x + viewHalfSize.x;
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
	bool bossHasPassed1200 = false;
	int bossSpreadLevel = 1;

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
			auto bullet = e->getTag() == "EnemyBullet";

			if (bullet)
			{
				if (bossSpreadLevel == 4) {
					int bulletCount = _entityManager.getEntities("EnemyBullet").size();
					if (bulletCount <= 20)
					{
						tfm.vel.y = 0;
						tfm.pos.y += tfm.vel.y * dt.asSeconds();
						tfm.angle += (tfm.angVel += 10) * dt.asSeconds();
					}
					else
					{
						tfm.vel.y = 200.f;
						tfm.pos.y += tfm.vel.y * dt.asSeconds();
						tfm.angle += (tfm.angVel += 10) * dt.asSeconds();
					}
				}
				else if (bossSpreadLevel == 3) {
					tfm.pos.y += (tfm.vel.y * 0.5) * dt.asSeconds();
					tfm.angle += (tfm.angVel += 10) * dt.asSeconds();
				}
				else if (bossSpreadLevel == 2) {
					tfm.pos.y += (tfm.vel.y * 0.5) * dt.asSeconds();
					tfm.angle += (tfm.angVel += 10) * dt.asSeconds();
				}
				else
				{
					tfm.pos.y += (tfm.vel.y * 0.7) * dt.asSeconds();
					tfm.angle += (tfm.angVel += 10) * dt.asSeconds();
				}
			}
			else if (boss)
			{
				if (tfm.pos.y < 1100.f && !bossHasPassed1200) {
					tfm.pos += tfm.vel * dt.asSeconds();
					tfm.angle += tfm.angVel * dt.asSeconds();
				}
				else if (!bossHasPassed1200) {
					bossHasPassed1200 = true;
					if (firstTimePassing1200) {
						e->addComponent<CGun>();
						tfm.vel.x += 25.f;
						firstTimePassing1200 = false;
					}
					const auto updatedX = tfm.vel.x + 25.f;
					tfm.pos.y += tfm.vel.y * dt.asSeconds();
					tfm.pos.x += tfm.vel.x * dt.asSeconds();
					tfm.angle += tfm.angVel * dt.asSeconds();

					if (tfm.pos.x < 100.f || tfm.pos.x > 650.f) {
						tfm.vel.x = -tfm.vel.x;
					}
					if (tfm.pos.y < 1100.f || tfm.pos.y > 1300.f) {
						tfm.vel.y = -tfm.vel.y;
					}

					// Clamp to ensure boss stays within range
					tfm.pos.x = std::clamp(tfm.pos.x, 100.f, 650.f);
					tfm.pos.y = std::clamp(tfm.pos.y, 1100.f, 1300.f);
				}
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
	SoundPlayer::getInstance().removeStoppedSounds();
	_entityManager.update();
	_worldView.move(0.f, 0.f);

	sAnimation(dt);
	sAutoPilot(dt);
	sMovement(dt);
	adjustPlayerPosition();
	sCollisions();
	sSpawnEnemies();
	sGunUpdate(dt);
	sGuideMissiles(dt);
	destroyOutsideBattlefieldBounds();
}

void Scene_Touhou::onEnd() {
	_game->changeScene("MENU", nullptr, false);
	firstTimePassing1200 = true;
	amountOfBullets = false;
	shooting = false;
	isSpread4 = false;
	restartGame(_levelPath);
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
			SoundPlayer::getInstance().play("LaunchMissile", pos);

			SoundPlayer::getInstance().play("LaunchMissile", pos);
		}
	}
}

void Scene_Touhou::fireBullet() {
	if (_player->hasComponent<CGun>())
		_player->getComponent<CGun>().isFiring = true;
}

void Scene_Touhou::sGunUpdate(sf::Time dt) {
	static sf::Time spreadChangeTimer = sf::seconds(5.f + (std::rand() % 6));
	for (auto e : _entityManager.getEntities()) {
		auto bullet = e->getTag() == "EnemyBullet";
		if (e->hasComponent<CGun>() && e->hasComponent<CTransform>()) {
			bool isEnemy = (e->getTag() == "enemy");
			bool isBossEnemy = (e->getTag() == "bossEnemy");

			auto& gun = e->getComponent<CGun>();
			gun.countdown -= dt;

			if (isBossEnemy && e->hasComponent<CHealth>()) {
				int bossCurrentHP = e->getComponent<CHealth>().hp;

				static int lastCheckedHP = bossCurrentHP;

				if ((lastCheckedHP - bossCurrentHP) >= 10000) {
					gun.spreadLevel = 1 + (std::rand() % 4);
					std::cout << "New spread level: " << gun.spreadLevel << "\n";

					lastCheckedHP = bossCurrentHP;
				}
			}

			if (isEnemy || isBossEnemy)
				gun.isFiring = true;

			//
			// when firing
			//
			if (gun.isFiring && gun.countdown < sf::Time::Zero) {
				gun.isFiring = false;
				gun.countdown = _config.fireInterval / (1.f + gun.fireRate);

				auto pos = e->getComponent<CTransform>().pos;
				switch (gun.spreadLevel) {
				case 0:
					spawnBullet(pos + sf::Vector2f(-20.f, 0.f), isBossEnemy);
					spawnBullet(pos + sf::Vector2f(20.f, 0.f), isBossEnemy);
					break;
				case 1:
					gun.fireRate = 1.5;
					for (int i = -5; i <= 5; ++i) {  // This creates 5 lines
						for (int j = 0; j < 10; ++j) {  // 20 bullets per line
							spawnBullet(pos + sf::Vector2f(i * 100.f, -700.f + j * 50.f), isBossEnemy);
						}
					}
					break;

				case 2:
				{
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

							spawnBullet(bulletPos, isBossEnemy);
						}
					}
				}
				break;

				case 3:
					spawnBullet(pos + sf::Vector2f(0.f, 35.f), isBossEnemy);
					spawnBullet(pos + sf::Vector2f(20.f, 0.f), isBossEnemy);
					spawnBullet(pos + sf::Vector2f(-20.f, 0.f), isBossEnemy);

					for (int i = 1; i <= 5; ++i) {
						spawnBullet(pos + sf::Vector2f(-20.f - (i * 50.f), -i * 50.f), isBossEnemy);
					}

					for (int i = 1; i <= 5; ++i) {
						spawnBullet(pos + sf::Vector2f(20.f + (i * 50.f), -i * 50.f), isBossEnemy);
					}

					break;
				case 4:
				{
					isSpread4 = true;
					if (_entityManager.getEntities("EnemyBullet").size() <= 20) {
						gun.fireRate = 2;
						// Semi - circle
						const int numBullets = 10;

						const float angleStep = 180.f / (numBullets - 1);

						const float radius = 100.f;

						for (int i = 0; i < numBullets; ++i)
						{
							float angle = 0.f + i * angleStep;

							sf::Vector2f bulletPos = pos + sf::Vector2f(std::cos(degToRad(angle)) * radius,
								std::sin(degToRad(angle)) * radius);

							spawnBullet(bulletPos, isBossEnemy);
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

void Scene_Touhou::spawnBullet(sf::Vector2f pos, bool isEnemy) {
	float speed;
	if (isEnemy) {
		speed = _config.bulletSpeed;
		SoundPlayer::getInstance().play("EnemyGunfire", pos);
	}
	else {
		speed = -_config.bulletSpeed;
		SoundPlayer::getInstance().play("Damage01", pos);
	}

	auto bullet = _entityManager.addEntity(isEnemy ? "EnemyBullet" : "PlayerBullet");
	auto bb = bullet->addComponent<CAnimation>(Assets::getInstance()
		.getAnimation("Bullet")).animation.getBB();
	bullet->addComponent<CBoundingBox>(bb);
	bullet->addComponent<CTransform>(pos, sf::Vector2f(0.f, speed));
	bullet->addComponent<CSpawnPosition>(pos);
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
	sf::Vector2f pos{ 350.f, sp.y };
	auto bossEnemy = _entityManager.addEntity("bossEnemy");
	auto& tfm = bossEnemy->addComponent<CTransform>(pos, sf::Vector2f{ 0.f, _config.enemySpeed });
	auto& type = sp.type;
	type = "Boss";

	if (sp.type == "Boss")
	{
		auto bb = bossEnemy->
			addComponent<CAnimation>(Assets::getInstance().getAnimation(sp.type)).animation.getBB();
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
		// player collids with pickup;
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
			auto overlap = Physics::getOverlap(bullet, e);
			if (overlap.x > 0 && overlap.y > 0) {
				e->getComponent<CHealth>().hp -= 10;
				bullet->destroy();
				checkIfDead(e);
			}
		}
	}

	// Enemy Bullets
	for (auto& bullet : _entityManager.getEntities("EnemyBullet")) {
		auto overlap = Physics::getOverlap(_player, bullet);
		if (overlap.x > 0 && overlap.y > 0) {
			_player->getComponent<CHealth>().hp -= 10;
			bullet->destroy();
			checkIfDead(_player);
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
			e->addComponent<CAnimation>(Assets::getInstance().getAnimation("explosion"));
			e->getComponent<CTransform>().vel = sf::Vector2f(0, 0);
			e->removeComponent<CHealth>();
			e->removeComponent<CBoundingBox>();
			e->addComponent<CState>().state = "Dead";
			if (flip(rng) == 1) {
				SoundPlayer::getInstance().play("Explosion1", e->getComponent<CTransform>().pos);
			}
			else {
				SoundPlayer::getInstance().play("Explosion2", e->getComponent<CTransform>().pos);
			}

			if (e->getTag() == "enemy")
				dropPickup(e->getComponent<CTransform>().pos);
			if (e->getTag() == "player") {
				restartGame(_levelPath);
			}
		}
	}
}

void Scene_Touhou::restartGame(const std::string& levelPath) {
	for (auto& e : _entityManager.getEntities()) {
		e->destroy();
	}
	_entityManager.update();
	firstTimePassing1200 = true;
	amountOfBullets = false;
	shooting = false;
	isSpread4 = false;
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
}