#include "LuaScriptingModule/Assets/lua_script.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "LuaScriptingModule/lua_scripting_module.h"

using namespace Engine::Extension::LuaScriptingModule;

Engine::Core::Runtime::CallbackResult Assets::ContextualizeLuaScript(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    for (size_t i = 0; i < contextCount; i++)
    {
        Core::AssetManagement::AssetLoadingContext &currentContext = outContext[i];

        currentContext.Buffer.Type = Core::AssetManagement::LoadBufferType::TransientBuffer;
        currentContext.Buffer.Location.TransientBufferSize = currentContext.SourceSize;
    }

    return Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::IndexLuaScript(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext)
{
    auto state = static_cast<LuaScriptingModuleState*>(moduleState);

    auto code = services->TransientAllocator->GetBuffer(inContext->Buffer.Location.TransientBufferId);
    if (code == nullptr)
    {
        state->GetLogger()->Error("Failed to load lua script {}, transient buffer rejected.", inContext->AssetId);
        return Core::Runtime::CallbackSuccess();
    }

    auto foundScript = state->GetLoadedScripts().find(inContext->AssetId);
    if (foundScript != state->GetLoadedScripts().end())
    {
        state->GetExecutor()->LoadScript(code, inContext->SourceSize, foundScript->second);
    }
    else 
    {
        int newIndex = state->IncrementScriptCounter();
        state->GetExecutor()->LoadScript(code, inContext->SourceSize, newIndex);
        state->GetLoadedScripts()[inContext->AssetId] = { newIndex };
    }

    return Core::Runtime::CallbackSuccess();
}