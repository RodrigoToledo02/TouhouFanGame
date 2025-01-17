//
// Created by David Burchill on 2023-09-28.
//

#include "EntityManager.h"
#include "Entity.h"

EntityManager::EntityManager() : _totalEntities(0)  {}


std::shared_ptr<Entity> EntityManager::addEntity(const std::string &tag) {
    // create a new Entity object
    auto e = std::shared_ptr<Entity>(new Entity(_totalEntities++, tag));

    // store it in entities vector
    _EntitiesToAdd.push_back(e);
    // return shared pointer to it
    return e;
}


EntityVec &EntityManager::getEntities(const std::string &tag) {
    return _entityMap[tag];
}


void EntityManager::update() {

    // dgb to remove later
    std::for_each(_entities.begin(), _entities.end(), [](auto e) {
       if (!(e->isActive())) {
           std::cout << "Destroying entity " << e->getTag() << std::endl;
       }
   });
    // dgb_end

    // Remove dead entities
    removeDeadEntities(_entities);
    for (auto& [_, entityVec] : _entityMap)
        removeDeadEntities(entityVec);


    // add new entities
    for (auto e : _EntitiesToAdd)
    {
        _entities.push_back(e);
        _entityMap[e->getTag()].push_back(e);
    }
    _EntitiesToAdd.clear();
}


EntityVec &EntityManager::getEntities() {
    return _entities;
}


void EntityManager::removeDeadEntities(EntityVec &v) {

    v.erase(std::remove_if(v.begin(), v.end(), [](auto e) {return!(e->isActive()); }), v.end());
}
