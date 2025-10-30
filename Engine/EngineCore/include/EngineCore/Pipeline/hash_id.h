#pragma once

#include <array>
#include <cstddef>
#include <functional>

namespace Engine::Core::Pipeline {

class HashId
{
public:
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

    inline bool operator==(const std::array<unsigned char, 16>& other) const 
    { 
        return LowQuad() == *((size_t*)other.data()) && HighQuad() == *((size_t*) (other.data() + 8));  
    }

    inline bool operator==(const std::array<unsigned char, 16>&& other) const 
    { 
        return LowQuad() == *((size_t*)other.data()) && HighQuad() == *((size_t*) (other.data() + 8));  
    }

    inline bool operator!=(const HashId& other) const 
    { 
        return LowQuad() != other.LowQuad() || HighQuad() != other.HighQuad(); 
    }

    inline bool operator!=(const std::array<unsigned char, 16>& other) const 
    { 
        return LowQuad() != *((size_t*)other.data()) || HighQuad() != *((size_t*) (other.data() + 8));  
    }

    inline bool operator!=(const std::array<unsigned char, 16>&& other) const 
    { 
        return LowQuad() != *((size_t*)other.data()) || HighQuad() != *((size_t*) (other.data() + 8));  
    }

    HashId() = default;
    HashId(const std::array<unsigned char, 16>& hash) : Hash(hash) {}
    HashId(const std::array<unsigned char, 16>&& hash) : Hash(hash) {}
};

struct HashIdTuple
{
    HashId First;
    HashId Second;

    inline bool operator==(const HashIdTuple& other) const 
    { 
        return First == other.First && Second == other.Second;
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

template <>
struct std::hash<Engine::Core::Pipeline::HashIdTuple>
{
    std::size_t operator()(const Engine::Core::Pipeline::HashIdTuple& k) const
    {
        return ((k.First.HighQuad() & 0xFFFFFFFF) << 32) + (k.Second.HighQuad() & 0xFFFFFFFF);
    }
};