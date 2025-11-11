#pragma once

#include <cstddef>
#include <vector>

namespace Engine::Core::Runtime {

struct TransientBufferGroup
{
    unsigned char* Buffer;
    size_t MemberCount;
};

class AssetManager
{
private:
    friend class GameLoop;
    std::vector<TransientBufferGroup> m_TransientBuffers;

public:
    void Contextualize();
    void Load();
    void Index();
};

}