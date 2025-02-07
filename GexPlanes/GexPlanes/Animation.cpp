//
// Created by David Burchill on 2024-11-26.
//

#include "Animation.h"
#include "Utilities.h"

Animation::Animation(const std::string& name,
	const sf::Texture& t,
	std::vector<sf::IntRect> frames,
	sf::Time tpf,
	bool repeats)
	: _name(name)
	, _frames(frames)
	, _timePerFrame(tpf)
	, _isRepeating(repeats)
	, _countDown(sf::Time::Zero)
	, _sprite(t, _frames[0]) {
	centerOrigin(_sprite);

	std::cout << name << " tpf: " << _timePerFrame.asMilliseconds() << "ms\n";
}

void Animation::update(sf::Time dt) {
	_countDown -= dt;
	if (_countDown < sf::Time::Zero) {
		_countDown = _timePerFrame;
		_currentFrame += 1;

		if (_currentFrame == _frames.size() && !_isRepeating)
			return;  // on the last frame of non-repeating animation, leave it
		else
			_currentFrame = (_currentFrame % _frames.size());

		_sprite.setTextureRect(_frames[_currentFrame]);
		centerOrigin(_sprite);
	}
}

bool Animation::hasEnded() const {
	return (_currentFrame >= _frames.size());
}

const std::string& Animation::getName() const {
	return _name;
}

sf::Sprite& Animation::getSprite() {
	return _sprite;
}

sf::Vector2f Animation::getBB() const {
	return static_cast<sf::Vector2f>(_frames[_currentFrame].getSize());
}