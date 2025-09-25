#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/root_module.h"

using namespace Engine::Core::Runtime;

bool ModuleManager::LoadModule(const ModuleInstance& instance)
{
    auto foundCollision = m_LoadedModules.find(instance.Definition.Name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[instance.Definition.Name] = instance;
    return true;
}

bool ModuleManager::LoadModule(const ModuleInstance&& instance)
{
    auto foundCollision = m_LoadedModules.find(instance.Definition.Name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[instance.Definition.Name] = instance;
    return true;
}

const void* ModuleManager::FindModule(const Pipeline::HashId& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second.State;
}

const void* ModuleManager::FindModule(const Pipeline::HashId&& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second.State;
}

const RootModuleState* ModuleManager::GetRootModule() const
{
    return static_cast<const RootModuleState*>(FindModule(RootModuleState::GetDefinition().Name));
}