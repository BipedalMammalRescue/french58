add_library(
    LiveLinkModule
    src/live_link_module.cpp
)

target_include_directories(LiveLinkModule PUBLIC include)

target_link_libraries(LiveLinkModule PUBLIC EngineCore)
target_link_libraries(Engine PUBLIC LiveLinkModule)
