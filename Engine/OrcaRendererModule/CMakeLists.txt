add_library(OrcaRendererModule STATIC
	src/module_definition.cpp
    src/Assets/shader_effect.cpp
    src/Assets/shader.cpp
    src/Runtime/render_context.cpp)



target_include_directories(OrcaRendererModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(OrcaRendererModule PUBLIC EngineCore)
target_link_libraries(Engine PUBLIC OrcaRendererModule)