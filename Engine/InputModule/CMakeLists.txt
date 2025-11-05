add_library(InputModule STATIC
    src/input_module.cpp
    src/input_action.cpp
)

target_include_directories(InputModule PUBLIC include)
target_link_libraries(InputModule PUBLIC EngineCore)
target_link_libraries(Engine PUBLIC InputModule)