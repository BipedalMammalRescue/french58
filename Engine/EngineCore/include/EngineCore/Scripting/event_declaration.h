#pragma once

#define DECLARE_SE_EVENT_0(name, returnType, runCore)\
class name : public Engine::Core::Scripting::ApiEvent_0\
{\
public:\
    void Run(const Runtime::ServiceTable* services, const void* moduleState, Runtime::EventWriter* writer) const override\
    {\
        runCOre(services, moduleState, writer);\
    }\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};

#define DECLARE_SE_EVENT_1(name, tp1, runCore) \
class name : Engine::Core::Scripting::ApiEvent_1<tp1>\
{\
protected:\
    void RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1, Engine::Core::Runtime::EventWriter* writer) const override \
    {\
        runCore(services, moduleState, p1, writer);\
    }\
public:\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};