add_library(EngineCore STATIC
	src/logger_service.cpp
	src/graphics_layer.cpp
    src/asset_enumerable.cpp
    src/root_module.cpp
    src/spatial_component.cpp
    src/world_state.cpp
    src/module_manager.cpp
    src/camera_component.cpp
    src/game_loop.cpp)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineCore PRIVATE cxx_std_17)
