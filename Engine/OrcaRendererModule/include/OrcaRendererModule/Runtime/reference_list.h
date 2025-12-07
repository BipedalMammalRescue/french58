#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Pipeline/hash_id.h"

#include <vector>

namespace Engine::Extension::OrcaRendererModule::Runtime {

// Light weight data structure that manages a series of unique
template <typename TElement> class ReferenceList
{
public:
    struct Node
    {
        TElement *Asset;
    };

private:
    // TODO: need a way to track freed storage nodes
    std::vector<Node> m_Storage;
    Core::Containers::Uniform::AnnotationSortedArray<Core::Pipeline::HashId, size_t> m_Index;

public:
    // Ensures the existence of a reference object in the storage/index, add an invalid one if not
    // found. The return value is an index into the storage buffer.
    size_t CreateReference(Core::Pipeline::HashId name)
    {
        auto foundItem = m_Index.GetOrAdd({.Key = name, .Value = m_Storage.size()});

        if (foundItem->Value == m_Storage.size())
        {
            m_Storage.push_back({nullptr});
        }

        return foundItem->Value;
    }

    // Overwrite the valued referenced under the given name, adds the new value if not found.
    // Returns the overwritten value if any.
    TElement *UpdateReference(Core::Pipeline::HashId name, TElement *newValue)
    {
        auto foundItem = m_Index.GetOrAdd({.Key = name, .Value = m_Storage.size()});

        TElement *oldValue = nullptr;

        if (foundItem->Value == m_Storage.size())
        {
            m_Storage.push_back({newValue});
        }
        else
        {
            oldValue = m_Storage[foundItem->Value].Asset;
            m_Storage[foundItem->Value] = {newValue};
        }

        return oldValue;
    }

    TElement *Get(size_t index)
    {
        if (index == m_Storage.size())
            return nullptr;

        return m_Storage[index].Asset;
    }

    size_t GetCount() const
    {
        return m_Storage.size();
    }
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime