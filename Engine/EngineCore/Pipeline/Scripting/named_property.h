#pragma once

#include "variant.h"

namespace Engine::Core::Pipeline::Scripting {

struct NamedProperty
{
    const char *Name;
    Scripting::DataType Type;
};

} // namespace Engine::Core::Pipeline::Scripting