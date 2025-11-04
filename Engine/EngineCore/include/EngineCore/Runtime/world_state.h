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

    float m_TotalTime = 0;
    float m_DeltaTime = 0;
    void Tick();

    WorldState(Configuration::ConfigurationProvider* configs) : m_Configs(configs) {}

public:
    void AddEntity(Ecs::Entity* entities, size_t count);
    bool LoadEntities(std::istream* input);

    // total elapsed time
    inline float GetTotalTime() const
    {
        return m_TotalTime;
    }

    // time since the last logical frame
    inline float GetDeltaTime() const
    {
        return m_DeltaTime;
    }
};

}