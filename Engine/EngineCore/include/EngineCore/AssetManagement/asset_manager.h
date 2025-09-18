#pragma once
#include <fstream>

namespace Engine::Core::Runtime {

class MemoryManager;

}

namespace Engine::Core::AssetManagement {

class AssetManager
{
private:
    Runtime::MemoryManager *m_MemoryManager;
    size_t m_Buffer;

public:
    AssetManager(Runtime::MemoryManager *memoryManager) : m_MemoryManager(memoryManager) {}

    std::ifstream OpenAsset(const unsigned char* id);
};

} // namespace Engine::Core::AssetManagement