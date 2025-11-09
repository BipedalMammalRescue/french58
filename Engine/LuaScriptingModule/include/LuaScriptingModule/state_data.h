#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"

namespace Engine::Extension::LuaScriptingModule {

struct NamedScriptParameter
{
    Core::Pipeline::HashId Name;
    Core::Pipeline::Variant Data;
};

struct InstancedScriptNode
{
    int Entity;
    std::vector<NamedScriptParameter> Parameters;
};

struct InstancedScript
{
    int ScriptIndex;
    std::vector<InstancedScriptNode> Nodes;
};

}