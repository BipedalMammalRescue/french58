#pragma once

#include "EngineCore/Ecs/entity.h"

#include <glm/glm.hpp>
#include <istream>
#include <vector>

namespace Engine::Core::Configuration {

struct ConfigurationProvider;

}

namespace Engine::Core::Runtime {

class GameLoop;

class WorldState 
{
private:
    friend class GameLoop;
    friend struct RootModuleState;
    Configuration::ConfigurationProvider* m_Configs;

    std::vector<Ecs::Entity> m_Entities;

    WorldState(Configuration::ConfigurationProvider* configs) : m_Configs(configs) {}

public:
    void AddEntity(Ecs::Entity* entities, size_t count);
    bool LoadEntities(std::istream* input);
};

}