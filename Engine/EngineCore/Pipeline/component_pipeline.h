#include "component_reflection.h"
#include <iostream>
#include <istream>
#include <ostream>

namespace Engine {
namespace Core {
namespace Pipeline {

struct ComponentPipeline
{
    ComponentReflection Reflection;
    void (*Builder)(const EditorComponent *objects, size_t objectCount, std::ostream &output);
    void (*Loader)(std::istream &input, void *state);
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
