add_library(EngineCore STATIC
	src/logger_service.cpp
	src/graphics_layer.cpp
    src/game_loop.cpp
    src/asset_enumerable.cpp)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineCore PRIVATE cxx_std_17)
