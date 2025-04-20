#pragma once

#include "DependencyInjection/buildtime_services.h"
#include "Pipeline/Scripting/variant.h"
#include <iostream>
#include <ostream>

namespace Engine {
namespace Core {
namespace Pipeline {

// when client code uses this type, they should just define their data in static function variables
struct ComponentProperty
{
    const char *Name;
    Scripting::DataType Type;
};

// list of name-type pairs representing values to be edited and constructed at edit time
struct ComponentDefinition
{
    // full name should take advantage of each module's namespace, need to be an UUID
    // TODO: in editor implementation this can be converted into a numeric identifier
    const char *FullName = nullptr;

    // display name is used in editor for human eyes to parse
    const char *DisplayName = nullptr;

    // properties are basic values that can be processed by an editor
    // TODO: during building this property list can be used to validate input structure
    ComponentProperty *Properties = nullptr;
    size_t PropertyCount = 0;
};

struct ComponentBuilder
{
    void *(*InitializeBuild)(DependencyInjection::BuildtimeServies *services);
    void *(*UpdateBuild)(const Scripting::Variant *fieldv, size_t fieldc, void *prevState,
                         DependencyInjection::BuildtimeServies *services);
    void (*FinailizeBuild)(void *state, DependencyInjection::BuildtimeServies *services, std::ostream &output);
};

struct ComponentPipeline
{
    ComponentDefinition Definition;
    ComponentBuilder Builder;
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
