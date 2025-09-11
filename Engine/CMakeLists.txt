cmake_minimum_required(VERSION 3.21)
project(Engine)

# shared dependencies for engine and extension modules
add_subdirectory(lib/SDL)
add_subdirectory(lib/json)
add_subdirectory(lib/glm)
add_subdirectory(lib/tinyobjloader)

add_subdirectory(EngineUtils)

# the engine itself without any extension modules
add_subdirectory(EngineCore)
target_compile_definitions(EngineCore PUBLIC GLEW_STATIC)
target_link_libraries(EngineCore PUBLIC EngineUtils)
target_link_libraries(EngineCore PUBLIC SDL3::SDL3)
target_link_libraries(EngineCore PUBLIC nlohmann_json)
target_link_libraries(EngineCore PUBLIC glm)
target_include_directories(EngineCore PUBLIC lib/komihash)

# declare engine project early
add_library(Engine STATIC engine_assembly.cpp)
set_target_properties(Engine PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(Engine PUBLIC EngineCore)

# define extension modules here; they should all depend on at least the engine core target
add_subdirectory(RendererModule)

# tests
add_executable(EngineTests Tests/app.cpp)
target_link_libraries(EngineTests PUBLIC EngineCore)
