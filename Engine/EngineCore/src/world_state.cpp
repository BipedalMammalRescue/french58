#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Ecs/entity.h"
#include "SDL3/SDL_timer.h"

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
    // reset state
    m_Entities.clear();
    
    // NOTE: the entire region of entities is 4-byte aligned
    unsigned int count = 0;
    input->read((char*)&count, sizeof(unsigned int));
    m_Entities.reserve(count);

    // load all entities and leave the input stream as is
    for (unsigned int i = 0; i < count; i++)
    {
        Ecs::Entity nextEntity;
        input->read((char*)&nextEntity, sizeof(Ecs::Entity));
        m_Entities.push_back(nextEntity);
    }

    return true;
}

void WorldState::Tick()
{
    m_DeltaTime = (SDL_GetTicksNS() - m_TotalTime * 1000000) / 1000000;
    m_TotalTime = (float)SDL_GetTicksNS() / 1000000;
}