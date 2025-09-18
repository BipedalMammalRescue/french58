add_library(EngineCore STATIC
	src/logger_service.cpp
	src/platform_access.cpp
	src/renderer_service.cpp
	src/asset_manager.cpp
    src/memory_manager.cpp)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineCore PRIVATE cxx_std_17)
