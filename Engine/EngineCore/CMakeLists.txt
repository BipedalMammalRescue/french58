add_library(EngineCore STATIC
	AssetManagement/asset_manager.cpp
	AssetManagement/byte_stream.cpp
	EntityModel/entity_manager.cpp
	Hosting/engine_host.cpp
	Logging/logger_service.cpp
	Memory/high_integrity_allocator.cpp
	Memory/homogeneous_storage.cpp
	Modularization/module_manager.cpp
	Platform/platform_access.cpp
	Reflection/data_type.cpp
	Reflection/type_info_registra.cpp
	Rendering/renderer_service.cpp)

target_include_directories(EngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(EngineCore PRIVATE cxx_std_17)
