#pragma once

#include <EngineCore/Scripting/api_event.h>

#define DECLARE_SE_EVENT_0(name, returnType, runCore)\
class name : public Engine::Core::Scripting::ApiEvent_0\
{\
public:\
    void Run(const Runtime::ServiceTable* services, const void* moduleState, Runtime::EventWriter* writer, int path) const override\
    {\
        runCOre(services, moduleState, writer, path);\
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
    void RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1, Engine::Core::Runtime::EventWriter* writer, int path) const override \
    {\
        runCore(services, moduleState, p1, writer, path);\
    }\
public:\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};

#define DECLARE_SE_EVENT_2(name, tp1, tp2, runCore) \
class name : Engine::Core::Scripting::ApiEvent_2<tp1, tp2>\
{\
protected:\
    void RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1, const tp2* p2, Engine::Core::Runtime::EventWriter* writer, int path) const override \
    {\
        runCore(services, moduleState, p1, p2, writer, path);\
    }\
public:\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};

#define DECLARE_SE_EVENT_3(name, tp1, tp2, tp3, runCore) \
class name : Engine::Core::Scripting::ApiEvent_3<tp1, tp2, tp3>\
{\
protected:\
    void RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1, const tp2* p2, const tp3* p3, Engine::Core::Runtime::EventWriter* writer, int path) const override \
    {\
        runCore(services, moduleState, p1, p2, p3, writer, path);\
    }\
public:\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};

#define DECLARE_SE_EVENT_4(name, tp1, tp2, tp3, tp4, runCore) \
class name : Engine::Core::Scripting::ApiEvent_4<tp1, tp2, tp3, tp4>\
{\
protected:\
    void RunCore(const Engine::Core::Runtime::ServiceTable* services, const void* moduleState, const tp1* p1, const tp2* p2, const tp3* p3, const tp4* p4, Engine::Core::Runtime::EventWriter* writer, int path) const override \
    {\
        runCore(services, moduleState, p1, p2, p3, p4, writer, path);\
    }\
public:\
    const char* GetName() const override { return #name; }\
    static const Engine::Core::Scripting::ApiEventBase* GetEvent()\
    { \
        static const name query; \
        return &query; \
    } \
};