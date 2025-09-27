add_executable(Playground app.cpp)

target_link_libraries(Playground PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
