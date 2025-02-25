#pragma once

#include "Configuration/compile_time_flags.h"

namespace Engine {
namespace Core {
namespace EntityModel {

// template <typename TComponent>
class BpTree
{
private:
    struct BpNode;

    struct BpLink
    {
        void* Less = nullptr;
        unsigned int Key = 0;
    };

    struct BpNode
    {
        BpLink Links[Configuration::COMPONENT_TREE_K_ARY];
        BpNode* More = nullptr;
    };
};


// A component tree is two b+ trees: one uses component ID as index, and the other uses a combination of entity and component ID: leftshift entity id (uint32) by 4 bytes then add component ID (uint32) to form a uint64, and use that as a new key; this introduces a new problem: entity lookup is now a range-lookup instead of an entry-lookup.
// TODO: figure out how to implement range lookup in b+ tree.
class ComponentTree 
{

};

}
}
}
