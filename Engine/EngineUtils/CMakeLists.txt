add_library(EngineUtils STATIC
    src/hex_strings.cpp)

target_include_directories(EngineUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(EngineUtils PRIVATE cxx_std_17)
