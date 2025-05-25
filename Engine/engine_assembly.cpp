#include <EngineCore/Pipeline/Scripting/variant.h>
#include <EngineCore/Pipeline/component_pipeline.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Pipeline/engine_assembly.h>

using namespace Engine::Core::Pipeline;

std::vector<ModuleDefinition *> EngineAssembly::s_Modules;

const char g_PropNameOne[] = "Prop_Name_1";
const char g_PropNameTwo[] = "Prop_Name_2";
const char g_ComponentName[] = "Component_Name";
const char g_ModuleName[] = "Module_Name";

Scripting::NamedProperty properties[] = {{g_PropNameOne, Scripting::DataType::BYTE},
                                  {g_PropNameTwo, Scripting::DataType::INT32}};

ComponentDefinition g_DummyComponent = {g_ComponentName, properties, 2};

ComponentBuilder g_DummyBuilder = {nullptr, nullptr, nullptr};

ComponentPipeline g_DummyPipeline = {g_DummyComponent, g_DummyBuilder};

ModuleDefinition g_DummyModule = {g_ModuleName, &g_DummyPipeline, 1};

int registraId = EngineAssembly::AddModule(&g_DummyModule);
