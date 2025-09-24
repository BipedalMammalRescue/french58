#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include <istream>

namespace Engine::Core::Pipeline {

struct RawAsset
{
    std::istream* Storage;
    HashId ID;
};

class IAssetEnumerator 
{
public:
    virtual size_t Count() = 0;
    virtual bool MoveNext() = 0;
    virtual RawAsset GetCurrent() = 0;
};

}