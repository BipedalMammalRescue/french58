add_library(EngineCore STATIC
	src/logger_service.cpp
    src/logger.cpp
	src/graphics_layer.cpp
    src/root_module.cpp
    src/spatial_component.cpp
    src/world_state.cpp
    src/module_manager.cpp
    src/camera_component.cpp
    src/game_loop.cpp
    src/variant.cpp
    src/event_manager.cpp
    src/event_stream.cpp
    src/event_writer.cpp
    src/module_thread_worker.cpp
    )

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineCore PRIVATE cxx_std_17)
