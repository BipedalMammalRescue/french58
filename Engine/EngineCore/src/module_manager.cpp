#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/root_module.h"

using namespace Engine::Core::Runtime;

bool ModuleManager::LoadModule(const Pipeline::HashId& name, void* instance)
{
    auto foundCollision = m_LoadedModules.find(name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[name] = instance;
    return true;
}

bool ModuleManager::LoadModule(const Pipeline::HashId&& name, void* instance)
{
    auto foundCollision = m_LoadedModules.find(name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[name] = instance;
    return true;
}

bool ModuleManager::LoadModule(const std::array<unsigned char, 16>& name, void* instance)
{
    auto foundCollision = m_LoadedModules.find(name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[name] = instance;
    return true;
}

bool ModuleManager::LoadModule(const std::array<unsigned char, 16>&& name, void* instance)
{
    auto foundCollision = m_LoadedModules.find(name);
    if (foundCollision != m_LoadedModules.end())
        return false;
    m_LoadedModules[name] = instance;
    return true;
}

const void* ModuleManager::FindModule(const Pipeline::HashId& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second;
}

const void* ModuleManager::FindModule(const Pipeline::HashId&& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second;
}

const RootModuleState* ModuleManager::GetRootModule() const
{
    return static_cast<const RootModuleState*>(FindModule(RootModuleState::GetDefinition().Name));
}