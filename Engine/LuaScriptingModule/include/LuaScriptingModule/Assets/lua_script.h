#pragma once

#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"

namespace Engine::Extension::LuaScriptingModule::Assets {

Engine::Core::Runtime::CallbackResult LoadLuaScript(Core::Pipeline::IAssetEnumerator *inputStreams, Core::Runtime::ServiceTable *services, void *moduleState);
Engine::Core::Runtime::CallbackResult UnloadLuaScript(Core::Pipeline::HashId *ids, size_t count, Core::Runtime::ServiceTable *services, void *moduleState);

}
