#include "LuaScriptingModule/Components/script_node.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "LuaScriptingModule/lua_scripting_module.h"
#include <iostream>
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
                std::cerr << "script path is not a path" << std::endl;
                return false;
            }

            scriptId = input.FieldV[i].Payload.Data.Path;
            break;
        }
        else 
        {
            parameters[input.FieldV[i].Name] = input.FieldV[i].Payload;
        }
    }

    if (!scriptId.has_value())
    {
        std::cerr << "script path not specified" << std::endl;
        return false;
    }

    output->write((char*)&input.Id, sizeof(input.Id));
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

Engine::Core::Runtime::CallbackResult Components::LoadScriptNode(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<LuaScriptingModuleState*>(moduleState);

    for (size_t i = 0; i < count; i++)
    {
        int id = stream.Read<int>();

        int entity = stream.Read<int>();

        Core::Pipeline::HashId scriptPath = stream.Read<Core::Pipeline::HashId>();

        // load parameters
        int parameterCount = stream.Read<int>();
        for (int paramIndex = 0; parameterCount > 0 && paramIndex < parameterCount; paramIndex ++)
        {
            Core::Pipeline::HashId name= stream.Read<Core::Pipeline::HashId>();
            Core::Pipeline::Variant data = stream.Read<Core::Pipeline::Variant>();

            state->GetExecutor()->SetParameter(name, id, data);
        }

        state->GetLogger()->Information("Loading script node {}:{}, script: {}", entity, id, scriptPath);

        auto foundScript = state->GetLoadedScripts().find(scriptPath);
        if (foundScript == state->GetLoadedScripts().end())
        {
            auto newScriptIndex = state->IncrementScriptCounter();
            state->GetLoadedScripts()[scriptPath] = newScriptIndex;
            state->GetNodes().push_back({ id, entity, newScriptIndex });
        }
        else 
        {
            state->GetNodes().push_back({ id, entity, foundScript->second });
        }
    }

    return Core::Runtime::CallbackSuccess();
}
