#pragma once

#include "Scripting/named_property.h"
#include "Scripting/variant.h"
#include "EngineCore/DependencyInjection/buildtime_services.h"
#include <ostream>

namespace Engine::Core::Pipeline {

// list of name-type pairs representing values to be edited and constructed at edit time
struct ComponentDefinition
{
    const char *Name = nullptr;
    Scripting::NamedProperty *Properties = nullptr;
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

} // namespace Engine::Core::Pipeline