#pragma once

namespace Engine::Core::Scripting {

class IParamReader
{
protected:
    virtual bool GetCore(void* destination) = 0;

public:
    template <typename TData>
    bool Get(TData* destination)
    {
        return GetCore(destination);
    }
};

}