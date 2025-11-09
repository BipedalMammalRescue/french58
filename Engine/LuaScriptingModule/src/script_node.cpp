#include "LuaScriptingModule/Components/script_node.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "LuaScriptingModule/lua_scripting_module.h"
#include <md5.h>
#include <optional>

using namespace Engine::Extension::LuaScriptingModule;

bool Components::CompileScriptNode(Core::Pipeline::RawComponent input, std::ostream* output)
{
    std::optional<Core::Pipeline::HashId> scriptId;
    std::unordered_map<Core::Pipeline::HashId, Core::Pipeline::Variant> parameters;

    for (size_t i = 0; i < input.FieldC; i ++)
    {
        if (input.FieldV[i].Name == md5::compute("Script"))
        {
            if (input.FieldV[i].Payload.Type != Core::Pipeline::VariantType::Path)
            {
                return false;
            }

            scriptId = input.FieldV[i].Payload.Data.Path;
        }
        else 
        {
            parameters[input.FieldV[i].Name] = input.FieldV[i].Payload;
        }
    }

    if (!scriptId.has_value())
    {
        return false;
    }

    output->write((char*)&input.Entity, sizeof(input.Entity));

    output->write((char*)&scriptId->Hash, sizeof(scriptId->Hash));
    
    int parameterCount = parameters.size();
    output->write((char*)&parameterCount, sizeof(parameterCount));

    for (const auto& parameter : parameters)
    {
        output->write((char*)&parameter.first, sizeof(parameter.first));
        output->write((char*)&parameter.second, sizeof(parameter.second));
    }

    return true;
}

Engine::Core::Runtime::CallbackResult Components::LoadScriptNode(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<LuaScriptingModuleState*>(moduleState);

    for (size_t i = 0; i < count; i++)
    {
        int entity;
        input[i].read((char*)&entity, sizeof(entity));

        Core::Pipeline::HashId scriptPath;
        input[i].read((char*)&scriptPath, sizeof(scriptPath));

        int parameterCount;
        input[i].read((char*)&parameterCount, sizeof(parameterCount));
        std::vector<NamedScriptParameter> params;
        params.reserve(parameterCount);
        for (int paramIndex = 0; paramIndex < parameterCount; paramIndex ++)
        {
            Core::Pipeline::HashId name;
            Core::Pipeline::Variant data;
            input[i].read((char*)&name, sizeof(name));
            input[i].read((char*)&data, sizeof(data));
        }

        auto foundScript = state->GetLoadedScripts().find(scriptPath);
        if (foundScript == state->GetLoadedScripts().end())
            continue;
        foundScript->second.Nodes.push_back({ entity, std::move(params) });
    }

    return Core::Runtime::CallbackSuccess();
}
