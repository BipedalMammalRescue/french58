add_library(RendererModule STATIC
	src/renderer_module.cpp
	src/shader.cpp
	src/mesh.cpp
	src/material.cpp)
	
target_include_directories(RendererModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
