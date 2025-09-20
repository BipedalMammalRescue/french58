#pragma once

#include "EngineCore/Ecs/entity.h"

#include <glm/glm.hpp>
#include <istream>
#include <unordered_map>

namespace Engine::Core::Configuration {

struct ConfigurationProvider;

}

namespace Engine::Core::Runtime {

class GameLoop;

class WorldState 
{
private:
    friend class GameLoop;
    // TODO: this is a temp solution
    std::unordered_map<int, Ecs::Entity> m_Entities;
    Configuration::ConfigurationProvider* m_Configs;

    WorldState(Configuration::ConfigurationProvider* configs) : m_Configs(configs) {}

public:
    bool GetEntityPosition(int id, glm::vec3& outPosition);
    void AddEntity(Ecs::Entity* entities, size_t count);
    bool LoadEntities(std::istream* input);
};

}