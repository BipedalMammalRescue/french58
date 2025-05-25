#pragma once

#include <EngineCore/DependencyInjection/buildtime_services.h>
#include <EngineCore/Pipeline/Scripting/variant.h>
#include <EngineCore/Pipeline/component_pipeline.h>
#include <EngineCore/Pipeline/engine_assembly.h>
#include <EngineUtils/String/unified_id.h>
#include <cstddef>
#include <istream>
#include <unordered_map>
#include <vector>

namespace DataBuilders {
namespace EntityBuilder {

class Parser
{
  private:
    Engine::Core::Pipeline::ModuleAssembly *m_Assembly;

    struct Range
    {
        size_t Start; // inclusive
        size_t End;   // exclusive
    };

    // An entity is two ranges: a range of children entities and a range of components
    struct Entity
    {
        Range Children;
        Range Components;
    };

    // A component is a range into the raw collection of fields
    struct Component
    {
        // type code is the 64-bit hash of the string full name of a component
        uint64_t TypeCode;
        Range Fields;
    };

    // A filed is a key-value pair; in source superentities the key is a string while the value is an arbitrary
    // engine-literal, here after parsing they are normalized into a 64-bit integer and a variant
    struct Field
    {
        uint64_t Key;
        Engine::Core::Pipeline::Scripting::Variant Value;
    };

    // from the assembly
    std::unordered_map<uint64_t, Engine::Core::Pipeline::ComponentDefinition *> m_NamedComponents;

    // from the input file:
    std::vector<Entity> m_Entities;
    std::vector<Component> m_Components;
    std::vector<Field> m_RawFields;

    // helper functions that make traversing this rather confusing data structure easier
    inline const Field *GetField(const Component &component, size_t index) const
    {
        size_t fieldIndex = component.Fields.Start + index;
        return (fieldIndex >= component.Fields.End) ? nullptr : &m_RawFields[fieldIndex];
    }

  public:
    Parser() = default;

    // validate the components are well-formatted
    bool Validate() const;

    // clears the current information, takes in a new text input, and refills self with new data
    void Parse(std::istream &input);
};

} // namespace EntityBuilder
} // namespace DataBuilders
