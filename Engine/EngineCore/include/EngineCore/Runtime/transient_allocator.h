#pragma once

#include <cstddef>

namespace Engine::Core::Runtime {

// TODO: this is a very very rudimentary implementation
class TransientAllocator
{


public:
    size_t Allocate(size_t desiredSize);
    size_t Return(size_t id);
};

}