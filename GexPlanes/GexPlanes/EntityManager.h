//
// Created by David Burchill on 2023-09-28.
//

#ifndef BREAKOUT_ENTITYMANAGER_H
#define BREAKOUT_ENTITYMANAGER_H


#include <map>
#include <vector>
#include <string>
#include <memory>

//forward declare
class Entity;

using sPtrEntt  = std::shared_ptr<Entity>;
using EntityVec = std::vector<std::shared_ptr<Entity>>;
using EntityMap = std::map <std::string, EntityVec>;


class EntityManager
{
private:
    EntityVec	    _entities;
    EntityMap	    _entityMap;
    size_t		    _totalEntities{0};
    EntityVec	    _EntitiesToAdd;

    void		    removeDeadEntities(EntityVec& v);

public:
    EntityManager();

    std::shared_ptr<Entity>         addEntity(const std::string& tag);
    EntityVec&                      getEntities();
    EntityVec&                      getEntities(const std::string& tag);

    void                            update();
};


#endif //BREAKOUT_ENTITYMANAGER_H
