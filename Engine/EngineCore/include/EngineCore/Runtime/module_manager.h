#pragma once

#include "EngineCore/Pipeline/hash_id.h"

#include <unordered_map>

namespace Engine::Core::Pipeline {
struct ModuleDefinition;
}

namespace Engine::Core::Runtime {

struct RootModuleState;

// Allow 
class ModuleManager
{
private:
    std::unordered_map<Pipeline::HashId, void*> m_LoadedModules;

private:
    friend class GameLoop;
    bool LoadModule(const Pipeline::HashId& name, void* instance);
    bool LoadModule(const Pipeline::HashId&& name, void* instance);
    bool LoadModule(const std::array<unsigned char, 16>& name, void* instance);
    bool LoadModule(const std::array<unsigned char, 16>&& name, void* instance);

public:
    const void* FindModule(const Pipeline::HashId& name) const;
    const void* FindModule(const Pipeline::HashId&& name) const;
    const RootModuleState* GetRootModule() const;
};

}