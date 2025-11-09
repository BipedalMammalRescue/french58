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
    virtual void Run(const Runtime::ServiceTable*, const void*, Runtime::EventWriter*, int) const = 0;
};

class ApiEventBase_1 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 1; }
    virtual void Run(const Runtime::ServiceTable*, const void*, ApiData, Runtime::EventWriter*, int) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
};

template <typename TP1>
class ApiEvent_1 : public ApiEventBase_1
{
protected:
    virtual void RunCore(const Runtime::ServiceTable*, const void*, const TP1*, Runtime::EventWriter*, int) const = 0;

public:
    void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, Runtime::EventWriter* writer, int path) const override
    {
        if (!IsTypeCompiliant<TP1>(services, moduleState, &p1))
            return;

        RunCore(services, moduleState, FromApiData<TP1>(&p1), writer, path);
    }

    ApiDataDefinition GetP1Type() const override 
    {
        return GetApiDataDefinition<TP1>();
    }
};


class ApiEventBase_2 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 2; }
    virtual void Run(const Runtime::ServiceTable*, const void*, ApiData, ApiData, Runtime::EventWriter*, int) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
    virtual ApiDataDefinition GetP2Type() const = 0;
};

template <typename TP1, typename TP2>
class ApiEvent_2 : public ApiEventBase_2
{
protected:
    virtual void RunCore(const Runtime::ServiceTable*, const void*, const TP1*, const TP2*, Runtime::EventWriter*, int) const = 0;

public:
    void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, ApiData p2, Runtime::EventWriter* writer, int path) const override
    {
        if (!IsTypeCompiliant<TP1>(services, moduleState, &p1))
            return;

        if (!IsTypeCompiliant<TP2>(services, moduleState, &p2))
            return;

        RunCore(services, moduleState, FromApiData<TP1>(&p1), FromApiData<TP2>(&p2), writer, path);
    }

    ApiDataDefinition GetP1Type() const override 
    {
        return GetApiDataDefinition<TP1>();
    }

    ApiDataDefinition GetP2Type() const override 
    {
        return GetApiDataDefinition<TP2>();
    }
};


class ApiEventBase_3 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 3; }
    virtual void Run(const Runtime::ServiceTable*, const void*, ApiData, ApiData, ApiData, Runtime::EventWriter*, int) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
    virtual ApiDataDefinition GetP2Type() const = 0;
    virtual ApiDataDefinition GetP3Type() const = 0;
};

template <typename TP1, typename TP2, typename TP3>
class ApiEvent_3 : public ApiEventBase_3
{
protected:
    virtual void RunCore(const Runtime::ServiceTable*, const void*, const TP1*, const TP2*, const TP3*, Runtime::EventWriter*, int) const = 0;

public:
    void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, ApiData p2, ApiData p3, Runtime::EventWriter* writer, int path) const override
    {
        if (!IsTypeCompiliant<TP1>(services, moduleState, &p1))
            return;

        if (!IsTypeCompiliant<TP2>(services, moduleState, &p2))
            return;

        if (!IsTypeCompiliant<TP3>(services, moduleState, &p3))
            return;

        RunCore(services, moduleState, FromApiData<TP1>(&p1), FromApiData<TP2>(&p2), FromApiData<TP3>(&p3), writer, path);
    }

    ApiDataDefinition GetP1Type() const override 
    {
        return GetApiDataDefinition<TP1>();
    }

    ApiDataDefinition GetP2Type() const override 
    {
        return GetApiDataDefinition<TP2>();
    }

    ApiDataDefinition GetP3Type() const override 
    {
        return GetApiDataDefinition<TP3>();
    }
};


class ApiEventBase_4 : public ApiEventBase
{
public:
    inline size_t GetParamCount() const override { return 4; }
    virtual void Run(const Runtime::ServiceTable*, const void*, ApiData, ApiData, ApiData, ApiData, Runtime::EventWriter*, int) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
    virtual ApiDataDefinition GetP2Type() const = 0;
    virtual ApiDataDefinition GetP3Type() const = 0;
    virtual ApiDataDefinition GetP4Type() const = 0;
};

template <typename TP1, typename TP2, typename TP3, typename TP4>
class ApiEvent_4 : public ApiEventBase_4
{
protected:
    virtual void RunCore(const Runtime::ServiceTable*, const void*, const TP1*, const TP2*, const TP3*, const TP4*, Runtime::EventWriter*, int) const = 0;

public:
    void Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1, ApiData p2, ApiData p3, ApiData p4, Runtime::EventWriter* writer, int path) const override
    {
        if (!IsTypeCompiliant<TP1>(services, moduleState, &p1))
            return;

        if (!IsTypeCompiliant<TP2>(services, moduleState, &p2))
            return;

        if (!IsTypeCompiliant<TP3>(services, moduleState, &p3))
            return;
    
        if (!IsTypeCompiliant<TP4>(services, moduleState, &p4))
            return;

        RunCore(services, moduleState, FromApiData<TP1>(&p1), FromApiData<TP2>(&p2), FromApiData<TP3>(&p3), FromApiData<TP4>(&p4), writer, path);
    }

    ApiDataDefinition GetP1Type() const override 
    {
        return GetApiDataDefinition<TP1>();
    }

    ApiDataDefinition GetP2Type() const override 
    {
        return GetApiDataDefinition<TP2>();
    }

    ApiDataDefinition GetP3Type() const override 
    {
        return GetApiDataDefinition<TP3>();
    }

    ApiDataDefinition GetP4Type() const override 
    {
        return GetApiDataDefinition<TP4>();
    }
};

}