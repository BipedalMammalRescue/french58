#pragma once

namespace Engine::Core::Rendering {

template <typename TData, unsigned int TFrames> class MultiBufferResource
{
private:
    TData m_Data[TFrames];
    unsigned int m_CurrentFrame = 0;

public:
    static unsigned int Size()
    {
        return TFrames;
    }

    TData Get() const
    {
        return m_Data[m_CurrentFrame];
    }

    void Set(TData data, unsigned int frame)
    {
        m_Data[frame] = data;
    }

    void Activate(unsigned int frame)
    {
        m_CurrentFrame = frame;
    }
};

} // namespace Engine::Core::Rendering