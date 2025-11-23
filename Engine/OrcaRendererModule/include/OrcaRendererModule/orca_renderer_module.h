#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule {

class ModuleState 
{
private:
    Core::Logging::Logger m_Logger;

public:
    inline Core::Logging::Logger* GetLogger() { return &m_Logger; }

    void AddOrReplaceShader(Core::Pipeline::HashId id, SDL_GPUShader* shader);
    SDL_GPUShader* GetShader(Core::Pipeline::HashId id);
};

}