#pragma once

#include "EngineCore/Pipeline/component_definition.h"

namespace Engine::Extension::LuaScriptingModule::Components {

bool CompileScriptNode(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadScriptNode(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

}