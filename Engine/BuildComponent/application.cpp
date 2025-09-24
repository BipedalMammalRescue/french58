#include <EngineCore/Pipeline/component_definition.h>
#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Pipeline/variant.h>
#include <EngineCore/Pipeline/module_assembly.h>

#include <iostream>
#include <unordered_map>

using namespace Engine::Core;

int main()
{
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> componentDefinitions;

    // initialize a map of all component builders
    Pipeline::ModuleAssembly modules = Pipeline::ListModules();
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.ComponentCount; j ++)
        {
            Pipeline::ComponentDefinition component = module.Components[j];
            Pipeline::HashIdTuple tuple { module.Name, component.Name };
            componentDefinitions[tuple] = component;
        }
    }

    std::array<unsigned char, 16> module;
    std::array<unsigned char, 16> type;

    std::cin.read((char*)module.data(), 16);
    std::cin.read((char*)type.data(), 16);

    auto foundComponent = componentDefinitions.find({module, type});
    if (foundComponent == componentDefinitions.end())
    {
        std::cerr << "Error: input component type not found." << std::endl;
        return 1;
    }

    Pipeline::ComponentDefinition targetComponentType = foundComponent->second;

    int componentCount = 0;
    std::cin.read((char*)&componentCount, sizeof(int));

    // read input from stdin
    std::vector<Pipeline::Field> fieldReadBuffer;
    for (int i = 0; i < componentCount; i++)
    {
        int componentId = 0;
        int entityId = 0;
        int fieldCount = 0;

        std::cin
            .read((char*)&componentId, sizeof(int))
            .read((char*)&entityId, sizeof(int))
            .read((char*)&fieldCount, sizeof(int));

        fieldReadBuffer.reserve(fieldCount);
        fieldReadBuffer.clear();

        for (int j = 0; j < fieldCount; j++)
        {
            std::array<unsigned char, 16> fieldName;
            Pipeline::Variant fieldData;
            std::cin.read((char*)fieldName.data(), 16);
            std::cin.read((char*)&fieldData, sizeof(Pipeline::Variant));
            fieldReadBuffer.push_back({fieldName, fieldData});
        }

        Pipeline::RawComponent nextInput {entityId, fieldReadBuffer.data(), fieldReadBuffer.size()};
        bool success = targetComponentType.Compile(nextInput, &std::cout);

        if (!success)
        {
            // TODO: logging?
            std::cerr << "Error: component failed to compile." << std::endl;
            return 1;
        }
    }

    return 0;
}