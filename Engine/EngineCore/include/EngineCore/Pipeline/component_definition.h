#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"

#include <ostream>

namespace Engine::Core::Runtime {
struct ServiceTable;
}

namespace Engine::Core::Pipeline {

struct Field 
{
    HashId Name;
    Variant Payload;
};

struct RawComponent
{
    int Id;
    int Entity;
    Field* FieldV;
    size_t FieldC;
};

struct ComponentDefinition
{
    NamePair Name;
    bool (*Compile)(RawComponent input, std::ostream* output);
    Runtime::CallbackResult (*Load)(size_t count, Utils::Memory::MemStreamLite& stream, Runtime::ServiceTable* services, void* moduleState);
};

}