#include "entity_manager.h"
#include "transform.h"

using namespace Engine;
using namespace Engine::Core::EntityModel;

const Transform* Entity::GetTransform() const
{
    return m_Transform;
}
