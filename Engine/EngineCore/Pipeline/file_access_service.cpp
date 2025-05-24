#include "file_access_service.h"

using namespace Engine::Core::Pipeline;

void Engine::Core::Pipeline::FileAccessService::Index(const char *rootPath)
{
    // TODO
}

const char *Engine::Core::Pipeline::FileAccessService::GetPath(uint64_t identifier) const
{
    auto cacheEntry = m_IndexCache.find(identifier);
    return cacheEntry == m_IndexCache.end() ?
        nullptr
        : cacheEntry->second.c_str();
}
