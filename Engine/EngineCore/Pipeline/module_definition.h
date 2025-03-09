#include "component_pipeline.h"

namespace Engine {
namespace Core {
namespace Pipeline {

// includes everything defined in a module
struct ModuleDefinition
{
    const char *Name;
    ComponentPipeline *Pipelines;
    size_t PipelineCount;
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
