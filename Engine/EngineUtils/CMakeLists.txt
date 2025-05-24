add_library(EngineUtils STATIC
	engine_utils.cpp)

target_include_directories(EngineUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(EngineUtils PRIVATE cxx_std_17)
