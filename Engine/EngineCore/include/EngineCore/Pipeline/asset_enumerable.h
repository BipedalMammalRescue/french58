#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include <istream>

namespace Engine::Core::Pipeline {

struct RawAsset
{
    std::istream* Storage;
    HashId ID;
};

class AssetEnumerable 
{
private:
    RawAsset* m_Storage;
    int m_Count;
    int m_Cursor;

public:
    AssetEnumerable(RawAsset* inStorage, int inCount) : m_Storage(inStorage), m_Count(inCount), m_Cursor(-1) {}
    size_t Count() { return m_Count; }
    bool MoveNext();
    RawAsset* GetCurrent();
};

}