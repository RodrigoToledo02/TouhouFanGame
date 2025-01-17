//
// Created by David Burchill on 2023-09-27.
//

#include "Entity.h"

Entity::Entity(size_t id, const std::string &tag)
        : _tag(tag), _id(id) {

}

void Entity::destroy() {
    _active = false;
}

const size_t &Entity::getId() const {
    return _id;
}

const std::string &Entity::getTag() const {
    return _tag;
}

bool Entity::isActive() const {
    return _active;
}
