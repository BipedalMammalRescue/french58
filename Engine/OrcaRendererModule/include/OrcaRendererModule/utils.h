#pragma once

namespace Engine::Extension::OrcaRendererModule {

template <typename TData> class DoubleBuffer
{
private:
    TData m_SideA;
    TData m_SideB;

    bool m_IsSideA;

public:
    const TData *Get() const
    {
        return m_IsSideA ? &m_SideA : &m_SideB;
    }

    void Update(const TData &newData)
    {
        if (m_IsSideA)
        {
            m_SideB = newData;
            m_IsSideA = false;
        }
        else
        {
            m_SideA = newData;
            m_IsSideA = true;
        }
    }
};

} // namespace Engine::Extension::OrcaRendererModule