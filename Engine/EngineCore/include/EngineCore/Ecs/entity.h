#pragma once

#include <glm/glm.hpp>

namespace Engine::Core::Ecs {

// Entity represents the engine-side slip of a unit of game state.
struct Entity
{
    int ID;
    int Parent = -1;
};

}