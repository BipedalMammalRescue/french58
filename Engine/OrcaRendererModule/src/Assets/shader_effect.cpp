#include "OrcaRendererModule/Assets/shader.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"

using namespace Engine::Extension::OrcaRendererModule::Assets;

Engine::Core::Runtime::CallbackResult Engine::Extension::OrcaRendererModule::Assets::ContextualizeShaderEffect(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    Engine::Extension::OrcaRendererModule::ModuleState* state = static_cast<Engine::Extension::OrcaRendererModule::ModuleState*>(moduleState);

    
}

Engine::Core::Runtime::CallbackResult Engine::Extension::OrcaRendererModule::Assets::IndexShaderEffect(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext)
{
    Engine::Extension::OrcaRendererModule::ModuleState* state = static_cast<Engine::Extension::OrcaRendererModule::ModuleState*>(moduleState);

}