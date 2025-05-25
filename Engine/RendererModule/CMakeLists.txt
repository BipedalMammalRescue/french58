add_library(RendererModule STATIC
	renderer_module.cpp
	Assets/shader.cpp)
	
target_include_directories(RendererModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
