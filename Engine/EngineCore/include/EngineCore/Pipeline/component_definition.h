#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include <ostream>
#include <istream>
#include <array>

namespace Engine::Core::Runtime {
struct ServiceTable;
}

namespace Engine::Core::Pipeline {

struct Variant;

struct Field 
{
    HashId Name;
    Variant* Payload;
};

struct RawComponent
{
    int Entity;
    Field* FieldV;
    size_t FieldC;
};

struct ComponentDefinition
{
    std::array<unsigned char, 16> Name;
    bool (*Compile)(RawComponent input, std::ostream* output);
    void (*Load)(size_t count, std::istream* input, Runtime::ServiceTable* services, void* moduleState);
};

}