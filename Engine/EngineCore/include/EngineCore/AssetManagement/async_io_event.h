#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/transient_allocator.h"

namespace Engine::Core::AssetManagement {

enum class EventType
{
    Entity,
    Asset
};

class AsyncIoEvent
{
public:
    virtual EventType GetType() const = 0;
};

class AsyncAssetEvent : public AsyncIoEvent
{
private:
    bool m_Available;
    bool m_Broken;
    AssetManagement::AssetLoadingContext m_LoadingContext;

public:
    AsyncAssetEvent(const AssetManagement::AssetLoadingContext& source)
        : m_Available(false), m_Broken(false), m_LoadingContext(source)
    {}

    bool IsAvailable() const { return m_Available; }
    bool IsBroken() const { return m_Broken; }
    AssetManagement::AssetLoadingContext* GetContext() { return &m_LoadingContext; }

    void MakeAvailable() { m_Available = true; }
    void MakeBroken() { m_Broken = true; }

    EventType GetType() const override { return EventType::Asset; }
};

class AsyncEntityEvent : public AsyncIoEvent 
{
private:
    Runtime::TransientBufferId m_Buffer;
    size_t m_Length;
    Pipeline::HashId m_Id;

public:
    AsyncEntityEvent(Runtime::TransientBufferId bufferId, size_t length, Pipeline::HashId id)
        : m_Buffer(bufferId),
        m_Length(length),
        m_Id(id)
    {}

    EventType GetType() const override { return EventType::Entity; }

    Runtime::TransientBufferId GetBuffer() const { return m_Buffer; }
    size_t GetLegnth() const { return m_Length; }
    Pipeline::HashId GetId() const { return m_Id; }
};

}