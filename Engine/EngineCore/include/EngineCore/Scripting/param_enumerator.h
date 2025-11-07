#pragma once

namespace Engine::Core::Scripting {

class IParamEnumerator
{
protected:
    virtual bool GetNextCore(void* destination) = 0;

public:
    template <typename TData>
    bool GetNext(TData* destination)
    {
        return GetNextCore(destination);
    }
};

}