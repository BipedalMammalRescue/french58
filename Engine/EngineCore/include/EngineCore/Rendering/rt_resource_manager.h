#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

template <typename TRes> class RtResourceManager
{
private:
    std::vector<TRes> m_Storage;

    struct DisposeRequest
    {
        int Frame;
        TRes Resource;
    };
    std::queue<DisposeRequest> m_DisposePrepQueue;
    std::queue<DisposeRequest> m_DisposeFreeQueue;

public:
    template <typename TEquate> void Assign(uint32_t id, TRes resource, int frame, TEquate equate)
    {
        if (id < m_Storage.size() && !equate(m_Storage[id], resource))
        {
            // queue this resource for disposal
            m_DisposePrepQueue.push({frame, m_Storage[id]});
        }

        m_Storage.reserve(id + 1);
        m_Storage[id] = resource;
    }

    template <typename TDispose> void PollFree(int frame, TDispose dispose)
    {
        while (!m_DisposeFreeQueue.empty() && m_DisposeFreeQueue.front().Frame == frame)
        {
            dispose(m_DisposeFreeQueue.front().Resource);
            m_DisposeFreeQueue.pop();
        }

        // move resources from prep queue to free queue
        while (!m_DisposePrepQueue.empty() && m_DisposePrepQueue.front().Frame == frame)
        {
            m_DisposeFreeQueue.push(m_DisposePrepQueue.front());
            m_DisposePrepQueue.pop();
        }
    }

    size_t GetCount()
    {
        return m_Storage.size();
    }

    TRes Get(uint32_t id)
    {
        return m_Storage[id];
    }
};

} // namespace Engine::Core::Rendering