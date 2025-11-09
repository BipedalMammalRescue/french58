#include "LuaScriptingModule/Assets/lua_script.h"
#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "LuaScriptingModule/lua_scripting_module.h"

using namespace Engine::Extension::LuaScriptingModule;

Engine::Core::Runtime::CallbackResult Assets::LoadLuaScript(Core::Pipeline::IAssetEnumerator *inputStreams, Core::Runtime::ServiceTable *services, void *moduleState)
{
    auto state = static_cast<LuaScriptingModuleState*>(moduleState);

    state->GetLoadedScripts().reserve(inputStreams->Count());
    while (inputStreams->MoveNext())
    {
        state->GetLoadedScripts()[inputStreams->GetCurrent().ID] = {
            std::vector<unsigned char>(std::istreambuf_iterator<char>(*inputStreams->GetCurrent().Storage), {}),
            {}
        };
    }

    return Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::UnloadLuaScript(Core::Pipeline::HashId *ids, size_t count, Core::Runtime::ServiceTable *services, void *moduleState)
{
    auto state = static_cast<LuaScriptingModuleState*>(moduleState);

    for (size_t i = 0; i < count; i++)
    {
        state->GetLoadedScripts().erase(ids[i]);
    }

    return Core::Runtime::CallbackSuccess();
}