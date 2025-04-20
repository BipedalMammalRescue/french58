#include "parser.h"
#include "Pipeline/Scripting/variant.h"
#include "Pipeline/component_pipeline.h"
#include <unordered_map>
#include <vector>

using namespace DataBuilders::EntityBuilder;
using namespace Engine::Core;

// TODO: the parse method should be implemented using XML

bool Parser::Validate() const
{
    std::unordered_map<uint64_t, Pipeline::Scripting::DataType> componentDefMap;

    for (const Component &component : m_Components)
    {
        // validate individual component: match their fields with the fields provided in the reflection
        uint64_t componentId = component.TypeCode;
        std::unordered_map<uint64_t, Pipeline::ComponentDefinition *>::const_iterator iterator =
            m_NamedComponents.find(componentId);

        // early termination: the component type is no longer supported
        // TODO: change this to use a validation results struct which accumulates all the errors
        if (iterator == m_NamedComponents.end())
            return false;

        // construct a map for expected fields
        componentDefMap.clear();
        for (size_t i = 0; i < iterator->second->PropertyCount; i++)
        {
            const Pipeline::ComponentProperty &expectProperty = iterator->second->Properties[i];
            uint64_t expectId = m_Hasher->IdString(expectProperty.Name);
            componentDefMap[expectId] = expectProperty.Type;
        }

        // compare each of the data fields to expect fields
        for (size_t i = component.Fields.Start; i < component.Fields.End; i++)
        {
            const Field &dataField = m_RawFields[i];
            Pipeline::Scripting::DataType dataType = dataField.Value.Type;

            const auto &expectType = componentDefMap.find(dataField.Key);
            if (expectType == componentDefMap.end() || expectType->second != dataType)
                return false;
        }
    }

    return true;
}
