#pragma once

#include "EngineCore/Ecs/entity.h"

#include <glm/glm.hpp>
#include <unordered_map>

namespace Engine::Core::Runtime {

class GameLoop;

class WorldState 
{
private:
    friend class GameLoop;
    // TODO: this is a temp solution
    std::unordered_map<int, Ecs::Entity> m_Entities;

public:
    bool GetEntityPosition(int id, glm::vec3& outPosition);
    void AddEntity(Ecs::Entity* entities, size_t count);
};

}