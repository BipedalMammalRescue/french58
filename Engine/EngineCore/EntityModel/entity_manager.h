#pragma once

namespace Engine {
namespace Core {

namespace Memory {
class HighIntegrityAllocator;
}

namespace EntityModel {

struct Transform;

struct Entity
{
private:
    Transform* m_Transform;

public:
    const Transform* GetTransform() const;
};


// TODO: need to consult someone smarter than me, is the "data in custom allocator pointer in STL" approach valid? how much difference does it make to store only part of the data in custom allocator?

class EntityManager 
{
private:
    Memory::HighIntegrityAllocator* m_Allocator;

public:
    EntityManager(Memory::HighIntegrityAllocator* allocator)
        : m_Allocator(allocator) 
    {}
    
    ~EntityManager() = default;
};

}
}
}
