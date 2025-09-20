#pragma once

#include <array>
#include <cstddef>
#include <functional>

namespace Engine::Core::Pipeline {

struct HashId
{
    std::array<unsigned char, 16> Hash;

    inline size_t LowQuad() const 
    { 
        return *((size_t*)Hash.data()); 
    }

    inline size_t HighQuad() const 
    { 
        return *((size_t*) (Hash.data() + 8)); 
    }

    inline bool operator==(const HashId& other) const 
    { 
        return LowQuad() == other.LowQuad() && HighQuad() == other.HighQuad(); 
    }
};

}

template <>
struct std::hash<Engine::Core::Pipeline::HashId>
{
    std::size_t operator()(const Engine::Core::Pipeline::HashId& k) const
    {
        return k.LowQuad();
    }
};