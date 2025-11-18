add_library(
    LiveLinkModule STATIC
    src/live_link_module.cpp
    src/live_link_session.cpp
)

target_include_directories(LiveLinkModule PUBLIC include)

target_link_libraries(LiveLinkModule PUBLIC EngineCore)

target_link_libraries(Engine PUBLIC LiveLinkModule)

add_executable(SampleSDL3Net src/sample.cpp)
target_link_libraries(SampleSDL3Net PUBLIC LiveLinkModule)