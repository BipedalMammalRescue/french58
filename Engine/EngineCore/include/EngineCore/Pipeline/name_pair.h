#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include <md5.h>

#define HASH_NAME(name) Engine::Core::Pipeline::NamePair { name, md5::compute(name) }

namespace Engine::Core::Pipeline {

struct NamePair
{
    const char* DisplayName;
    HashId Hash;
};

}