//
// Created by David Burchill on 2023-09-27.
//

#ifndef BREAKOUT_ENTITY_H
#define BREAKOUT_ENTITY_H

#include <tuple>
#include <string>

#include "Components.h"
// forward declarations
class EntityManager;

using ComponentTuple = std::tuple<CSpawnPosition, CSprite, CMissiles, CGun, CHealth, CState, CAnimation, CTransform, CAutoPilot, CBoundingBox, CInput, CScore>;

class Entity {
private:
	friend class EntityManager;
	Entity(size_t id, const std::string& tag);      // private ctor, entities can only be created by EntityManager

	const size_t            _id{ 0 };
	const std::string       _tag{ "Default" };
	bool                    _active{ true };
	ComponentTuple          _components;

public:

	void                    destroy();
	const size_t& getId() const;
	const std::string& getTag() const;
	bool                    isActive() const;

	// Component API
	template<typename T>
	inline bool hasComponent() const {
		return getComponent<T>().has;
	}

	template<typename T, typename... TArgs>
	inline T& addComponent(TArgs &&... mArgs) {
		auto& component = getComponent<T>();
		component = T(std::forward<TArgs>(mArgs)...);
		component.has = true;
		return component;
	}

	template<typename T>
	inline bool removeComponent() {
		return getComponent<T>().has = false;
	}

	template<typename T>
	inline T& getComponent() {
		return std::get<T>(_components);
	}

	template<typename T>
	inline const T& getComponent() const {
		return std::get<T>(_components);
	}
};

#endif //BREAKOUT_ENTITY_H
