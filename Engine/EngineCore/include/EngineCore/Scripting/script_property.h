#pragma once

#include "EngineCore/Scripting/script_object.h"

namespace Engine::Core::Scripting {

struct ScriptProperty
{
    const char* Name;
    const ScriptObject* Type;
};

}