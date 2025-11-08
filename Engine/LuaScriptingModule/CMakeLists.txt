add_executable(LuaInterpreter 
    src/lua_interpreter.cpp
    src/lua_executor.cpp
)

target_include_directories(LuaInterpreter PUBLIC include)
target_link_libraries(LuaInterpreter PUBLIC Engine)
target_link_libraries(LuaInterpreter PUBLIC lua_static)