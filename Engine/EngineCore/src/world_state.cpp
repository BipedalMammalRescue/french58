#include "EngineCore/Runtime/world_state.h"

using namespace Engine::Core::Runtime;

bool WorldState::GetEntityPosition(int id, glm::vec3& outPosition)
{
    auto foundEntity = m_Entities.find(id);
    if (foundEntity == m_Entities.end())
        return false;
    outPosition = foundEntity->second.Position;
    return true;
}

void WorldState::AddEntity(Ecs::Entity* entities, size_t count)
{
    for (size_t i = 0; i < count; i++) 
    {
        m_Entities[entities[i].ID] = entities[i];
    }
}