cmake_minimum_required(VERSION 3.21)
project(Engine)

# libraries the engine depends on
add_subdirectory(lib/SDL)
add_subdirectory(lib/json)
add_subdirectory(lib/glm)

# the engine itself without any extension modules
add_subdirectory(EngineCore)
target_compile_definitions(EngineCore PUBLIC GLEW_STATIC)
target_link_libraries(EngineCore PUBLIC SDL3::SDL3)
target_link_libraries(EngineCore PUBLIC nlohmann_json)
target_link_libraries(EngineCore PUBLIC glm)
target_include_directories(EngineCore PUBLIC lib/komihash)

# define extension modules here; they should all depend on at least the engine core target
# add_subdirectory(RendererModule)
# target_link_libraries(RendererModule PUBLIC EngineCore)

# quick and dirty trick to bundle together everything defined thus far:
add_library(Engine STATIC engine_assembly.cpp)
set_target_properties(Engine PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(Engine PUBLIC EngineCore)
# target_link_libraries(Engine PUBLIC RendererModule)

# tests
add_executable(EngineTests Tests/app.cpp)
target_link_libraries(EngineTests PUBLIC EngineCore)
