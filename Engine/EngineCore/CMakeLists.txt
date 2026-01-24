add_library(EngineCore STATIC
    src/asset_manager.cpp
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
    src/task_manager.cpp
    src/input_manager.cpp
    src/network_layer.cpp
    src/transient_allocator.cpp
    src/index_queue.cpp
    src/Rendering/render_target.cpp
    src/Rendering/gpu_resource.cpp
    src/Rendering/render_context.cpp
    src/Rendering/render_thread.cpp
    src/Rendering/Resources/shader.cpp
    src/Rendering/Resources/swapchain.cpp
    src/Rendering/Lib/vk_queue_selector.cpp
    src/Rendering/Lib/vk_mem_alloc.cpp
    src/Rendering/transfer_manager.cpp
    src/Rendering/Resources/transient_image.cpp
)

find_package(Vulkan)
target_link_libraries(EngineCore PRIVATE Vulkan::Vulkan)
target_link_libraries(EngineCore PUBLIC spdlog::spdlog)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineCore PRIVATE cxx_std_17)
