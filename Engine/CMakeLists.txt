cmake_minimum_required(VERSION 3.21)
project(Engine)

# shared dependencies for engine and extension modules
add_subdirectory(lib/SDL)
add_subdirectory(lib/glm)
add_subdirectory(lib/tinyobjloader)
add_subdirectory(lib/constexpr-md5-cpp)
add_subdirectory(lib/concurrentqueue)
add_subdirectory(lib/lua-5.3.6)
add_subdirectory(lib/SDL_net)
add_subdirectory(lib/spdlog)

add_subdirectory(EngineUtils)

# the engine itself without any extension modules
add_subdirectory(EngineCore)
target_compile_definitions(EngineCore PUBLIC GLEW_STATIC)

target_link_libraries(EngineCore PUBLIC EngineUtils)
target_link_libraries(EngineCore PUBLIC SDL3::SDL3)
target_link_libraries(EngineCore PUBLIC glm)
target_link_libraries(EngineCore PUBLIC constexpr_md5)
target_link_libraries(EngineCore PUBLIC concurrentqueue)
target_link_libraries(EngineCore PUBLIC SDL3_net-shared)

# declare engine project early
add_library(Engine STATIC module_assembly.cpp)
set_target_properties(Engine PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(Engine PUBLIC EngineCore)

# define extension modules here; they should all depend on at least the engine core target
# add_subdirectory(RendererModule)
add_subdirectory(ExampleGameplayModule)
add_subdirectory(InputModule)
add_subdirectory(LuaScriptingModule)
add_subdirectory(LiveLinkModule)
# add_subdirectory(OrcaRendererModule)

# tools
add_subdirectory(BuildComponent)

# tests
add_executable(EngineTests Tests/app.cpp)
target_link_libraries(EngineTests PUBLIC EngineCore)
