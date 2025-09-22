#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Ecs/entity.h"

using namespace Engine::Core::Runtime;

void WorldState::AddEntity(Ecs::Entity* entities, size_t count)
{
    for (size_t i = 0; i < count; i++) 
    {
        m_Entities.push_back(entities[i]);
    }
}

bool WorldState::LoadEntities(std::istream* input) 
{
    // confirm version
    int version = 0;
    input->read((char*)&version, sizeof(int));
    if (version != m_Configs->EntityVersion)
        return false;

    // reset state
    m_Entities.clear();
    
    // NOTE: the entire region of entities is 4-byte aligned
    unsigned int count = 0;
    input->read((char*)&count, sizeof(unsigned int));
    m_Entities.reserve(count);

    // load all entities and leave the input stream as is
    Ecs::Entity loadingBuffer[Configuration::EntityLoadBatchSize * sizeof(Ecs::Entity)];
    while (count > 0) 
    {
        size_t targetLoadCount = std::min((size_t)count, Configuration::EntityLoadBatchSize);
        input->read((char*)loadingBuffer, targetLoadCount * sizeof(Ecs::Entity));
        AddEntity(loadingBuffer, targetLoadCount);
    }

    return true;
}