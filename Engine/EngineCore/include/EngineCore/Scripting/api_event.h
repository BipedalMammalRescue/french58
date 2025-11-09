#pragma once

#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Scripting/api_data.h"
#include <cstddef>

namespace Engine::Core::Runtime {
    class ServiceTable;
}

namespace Engine::Core::Scripting {

class ApiEventBase 
{
public:
    virtual const char* GetName() const = 0;
    virtual size_t GetParamCount() const = 0;
};

class ApiEvent_0 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 0; }
    virtual void Run(const Runtime::ServiceTable*, const void*, Runtime::EventWriter*) const = 0;
};

class ApiEventBase_1 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 1; }
    virtual void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, Runtime::EventWriter* writer) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
};

template <typename TP1>
class ApiEvent_1 : public ApiEventBase_1
{
protected:
    virtual void RunCore(const Runtime::ServiceTable* services, const void* moduleState, const TP1* p1, Runtime::EventWriter* writer) const = 0;

public:
    void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, Runtime::EventWriter* writer) const override
    {
        if (!IsTypeCompiliant<TP1>(services, moduleState, &p1))
            return;

        RunCore(services, moduleState, FromApiData<TP1>(&p1), writer);
    }

    ApiDataDefinition GetP1Type() const override 
    {
        return GetApiDataDefinition<TP1>();
    }
};

}