add_executable(Playground app.cpp)

target_include_directories(Playground PUBLIC ../Engine/lib/komihash)
target_include_directories(Playground PUBLIC ../Engine/lib/json/include)
