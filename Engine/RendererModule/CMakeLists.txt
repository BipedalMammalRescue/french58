add_library(RendererModule STATIC
	src/renderer_module.cpp)

target_include_directories(RendererModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(RendererModule PUBLIC EngineCore)
target_link_libraries(RendererModule PUBLIC tinyobjloader)
target_link_libraries(Engine PUBLIC RendererModule)

add_executable(MeshBuilder src/mesh_builder.cpp)
target_link_libraries(MeshBuilder PUBLIC RendererModule)