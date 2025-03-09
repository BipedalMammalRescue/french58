#include "Scripting/variant.h"
#include <vector>

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
struct ComponentReflection
{
    // full name should take advantage of each module's namespace, need to be an UUID
    // PERF: it'll certainly be faster if the key is a number, but assigning number at runtime means less stable data
    // naming convention, thus I'm making it a c_string
    const char *FullName;

    // display name is used in editor for human eyes to parse
    const char *DisplayName;

    ComponentProperty *Properties;
    size_t PropertyCount;
};

// dynmaic collection of name-value pairs representing an instance of a constructed component
struct EditorComponent
{
    std::vector<Scripting::Variant> Properties;
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
