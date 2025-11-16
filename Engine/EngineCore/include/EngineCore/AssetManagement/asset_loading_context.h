#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/transient_allocator.h"

namespace Engine::Core::AssetManagement {

enum class LoadBufferType
{
    TransientBuffer,
    ModuleBuffer,
    Invalid
};

struct LoadBuffer
{
    union {
        int TransientBufferSize;
        Runtime::TransientBufferId TransientBufferId;
        void* ModuleBuffer; // this buffer is fully allocated before the contextualizer exits
    } Location;
    LoadBufferType Type;
};

struct AssetLoadingContext
{
    bool ReplaceExisting;
    size_t SourceSize;
    Pipeline::HashIdTuple AssetGroupId;
    Pipeline::HashId AssetId;
    LoadBuffer Buffer;
    void* UserData;
};

}