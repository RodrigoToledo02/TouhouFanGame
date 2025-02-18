//
// Created by David Burchill on 2023-09-27.
//

#ifndef BREAKOUT_COMPONENTS_H
#define BREAKOUT_COMPONENTS_H

#include "Animation.h"
#include <memory>
#include <SFML/Graphics.hpp>
#include "Utilities.h"

struct Component
{
	bool		has{ false };
	Component() = default;
};

struct CSpawnPosition : public Component {
	sf::Vector2f initialPos;

	CSpawnPosition() = default;

	explicit CSpawnPosition(sf::Vector2f pos) : initialPos(pos) {}
};

struct CAnimation : public Component {
	Animation   animation;

	CAnimation() = default;
	explicit CAnimation(const Animation& a) : animation(a) {}
};

struct CAutoPilot : public Component
{
	size_t currentLeg{ 0 };
	sf::Time countdown{ sf::Time::Zero };

	CAutoPilot() = default;
};

struct CMissiles : public Component {
	size_t    missileCount{ 15 };

	CMissiles() = default;
};

struct CGun : public Component {
	bool isFiring{ false };
	sf::Time countdown{ sf::Time::Zero };
	int fireRate{ 1 };
	int spreadLevel{ 0 };

	CGun() = default;
};

struct CSprite : public Component {
	sf::Sprite sprite;

	CSprite() = default;
	explicit CSprite(const sf::Texture& t)
		: sprite(t) {
		centerOrigin(sprite);
	}
	CSprite(const sf::Texture& t, sf::IntRect r)
		: sprite(t, r) {
		centerOrigin(sprite);
	}
};

struct CTransform : public Component
{
	sf::Transformable  tfm;
	sf::Vector2f	pos{ 0.f, 0.f };
	sf::Vector2f	prevPos{ 0.f, 0.f };
	sf::Vector2f	vel{ 0.f, 0.f };
	sf::Vector2f	scale{ 1.f, 1.f };

	float   angVel{ 0 };
	float	angle{ 0.f };

	CTransform() = default;
	explicit CTransform(const sf::Vector2f& p) : pos(p) {}
	CTransform(const sf::Vector2f& p, const sf::Vector2f& v)
		: pos(p), prevPos(p), vel(v) {
	}
};

struct CCollision : public Component
{
	float radius{ 0.f };

	CCollision() = default;
	explicit CCollision(float r)
		: radius(r) {
	}
};

struct CBoundingBox : public Component
{
	sf::Vector2f size{ 0.f, 0.f };
	sf::Vector2f halfSize{ 0.f, 0.f };
	bool isSmall{ false };

	CBoundingBox() = default;
	explicit CBoundingBox(const sf::Vector2f& s) : size(s), halfSize(0.5f * s)
	{
	}
	CBoundingBox(float w, float h) : size(sf::Vector2f{ w,h }), halfSize(0.5f * size)
	{
	}
};

struct CInput : public Component
{
	bool up{ false };
	bool left{ false };
	bool right{ false };
	bool down{ false };

	bool spinr{ false };
	bool spinl{ false };

	bool lshift{ false };

	CInput() = default;
};

struct CScore : public Component
{
	int score{ 0 };
	explicit CScore(int s = 0) : score(s) {}
};

struct CHealth : public Component {
	int         hp{ 1 };

	CHealth() = default;
	explicit CHealth(int hp) : hp(hp) {}
};

struct CState : public Component {
	std::string state{ "none" };

	CState() = default;
	explicit CState(const std::string& s) : state(s) {}
};

#endif //BREAKOUT_COMPONENTS_H
