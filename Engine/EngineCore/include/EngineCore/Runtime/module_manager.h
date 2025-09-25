#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_definition.h"

#include <unordered_map>

namespace Engine::Core::Pipeline {
struct ModuleDefinition;
}

namespace Engine::Core::Runtime {

struct RootModuleState;

struct ModuleInstance
{
    Pipeline::ModuleDefinition Definition;
    void* State;
};

// Allow 
class ModuleManager
{
private:
    std::unordered_map<Pipeline::HashId, ModuleInstance> m_LoadedModules;

private:
    friend class GameLoop;
    bool LoadModule(const ModuleInstance& instance);
    bool LoadModule(const ModuleInstance&& instance);

public:
    const void* FindModule(const Pipeline::HashId& name) const;
    const void* FindModule(const Pipeline::HashId&& name) const;
    const RootModuleState* GetRootModule() const;
};

}