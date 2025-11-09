#pragma once

#define DECLARE_SE_OBJECT(name)\
template<> Engine::Core::Scripting::ApiDataDefinition GetApiDataDefinition<name>()\
{\
    static const char typeName[] = #name;\
    static const Engine::Core::Scripting::OpaqueObjectMetadata typeMetadata = { typeName };\
    return { .Type = ApiDataType::Object, .SubType.Object = &typeMetadata };\
}

#define DECLARE_SE_API_0(name, returnType, runCore)\
class name : public Engine::Core::Scripting::ApiQuery_0<returnType>\
{\
protected:\
    Engine::Core::Scripting::ReturnContainer<returnType> RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState) const override \
    { return { runCore(services, moduleState) }; } \
public:\
    const char* GetName() const override \
    {\
        return #name;\
    }\
    static const Engine::Core::Scripting::ApiQueryBase* GetQuery()\
    {\
        static const name query;\
        return &query;\
    }\
}

#define DECLARE_SE_API_1(name, returnType, tp1, runCore) \
class name : public Engine::Core::Scripting::ApiQuery_1<returnType, tp1> \
{ \
protected: \
    Engine::Core::Scripting::ReturnContainer<returnType> RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1) const override \
    { return { runCore(services, moduleState, p1) }; } \
public: \
    const char* GetName() const override \
    {\
        return #name; \
    }\
    static const Engine::Core::Scripting::ApiQueryBase* GetQuery()\
    { \
        static const name query; \
        return &query; \
    } \
}