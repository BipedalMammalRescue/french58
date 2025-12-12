#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Pipeline/hash_id.h"

#include <optional>
#include <vector>

namespace Engine::Extension::OrcaRendererModule::Runtime {

// Light weight data structure that manages a series of unique.
// Main purpose is to allow client code to use a key-like data structure (in this case the engine
// concept HashId), search once for a *stable* reference key, then
template <typename TElement> class ReferenceList
{
public:
    struct Node
    {
        TElement Asset;
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
            m_Storage.push_back({});
        }

        return foundItem->Value;
    }

    // Overwrite the valued referenced under the given name, adds the new value if not found.
    // Returns the overwritten value if any.
    std::optional<TElement> UpdateReference(Core::Pipeline::HashId name, const TElement &newValue)
    {
        auto foundItem = m_Index.GetOrAdd({.Key = name, .Value = m_Storage.size()});

        std::optional<TElement> oldValue;

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

    void ReserveExtra(size_t extraCount)
    {
        m_Storage.reserve(m_Storage.size() + extraCount);
        m_Index.ReserveExtra(extraCount);
    }

    TElement Get(size_t index)
    {
        assert(index <= m_Storage.size());
        return m_Storage[index].Asset;
    }

    size_t GetCount() const
    {
        return m_Storage.size();
    }
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime