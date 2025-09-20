#include "EngineCore/Pipeline/asset_enumerable.h"

using namespace Engine::Core::Pipeline;

bool AssetEnumerable::MoveNext()
{
    m_Cursor++;
    return m_Cursor >= m_Count;
}

RawAsset *AssetEnumerable::GetCurrent()
{
    return (m_Cursor >= m_Count || m_Cursor < 0) ? nullptr : m_Storage + m_Cursor;
}