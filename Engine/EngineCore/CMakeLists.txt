add_library(EngineCore STATIC
	Logging/logger_service.cpp
	Platform/platform_access.cpp
	Rendering/renderer_service.cpp)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(EngineCore PRIVATE cxx_std_17)
