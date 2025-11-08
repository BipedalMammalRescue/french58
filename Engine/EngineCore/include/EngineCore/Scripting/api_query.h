#pragma once

#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Scripting/api_data.h"
#include <cstddef>

namespace Engine::Core::Scripting {

class ApiQueryBase 
{
public:
    virtual size_t GetParamCount() const = 0;
    virtual ApiDataDefinition GetReturnType() const = 0;
};

class ApiQueryBase_0 : public ApiQueryBase
{
public:
    inline size_t GetParamCount() const override { return 0; }
    virtual ApiData Run(const Runtime::ServiceTable* services, const void* moduleState) const = 0;
};

template <typename TRet>
class ApiQuery_0 : public ApiQueryBase_0
{
protected:
    virtual ReturnContainer<TRet> RunCore(const Runtime::ServiceTable* services, const void* moduleState) = 0;

public:
    ApiData Run(const Runtime::ServiceTable* services, const void* moduleState) const override
    {
        ReturnContainer<TRet> result = RunCore(services, moduleState);
        return ToApiData(result.Get());
    }
};

class ApiQueryBase_1 : public ApiQueryBase
{
public:
    inline size_t GetParamCount() const override { return 1; }
    virtual ApiData Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1) const = 0;
    virtual ApiDataDefinition GetP1Type() const = 0;
};

template <typename TRet, typename TP1>
class ApiQuery_1 : public ApiQueryBase_1
{
protected:
    virtual ReturnContainer<TRet> RunCore(const Runtime::ServiceTable* services, const void* moduleState, const TP1* p1) = 0;

public:
    ApiData Run(const Runtime::ServiceTable* services, const void* moduleState, ApiData p1) const override
    {
        if (!IsTypeCompiliant<TP1>(&p1))
            return { ApiDataType::Invalid };

        ReturnContainer<TRet> result = RunCore(services, moduleState, FromApiData<TP1>(&p1));
        return ToApiData(result.Get());
    }
};

}