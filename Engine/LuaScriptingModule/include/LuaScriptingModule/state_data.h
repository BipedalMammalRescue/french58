#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"

namespace Engine::Extension::LuaScriptingModule {

struct InstancedScriptNode
{
    int Entity;
    int ScriptIndex;
    std::unordered_map<Core::Pipeline::HashId, Core::Pipeline::Variant> Parameters;
};

}