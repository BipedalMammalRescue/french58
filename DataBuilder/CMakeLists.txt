add_executable(DataBuilder
	src/application.cpp
)

target_include_directories(DataBuilder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(DataBuilder PRIVATE cxx_std_17)