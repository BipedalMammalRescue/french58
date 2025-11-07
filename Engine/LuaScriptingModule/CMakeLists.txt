add_executable(LuaInterpreter 
    src/lua_interpreter.cpp 
    src/lua_data_accesses.cpp
    src/lua_return_writer.cpp
    src/lua_param_enumerator.cpp
)

target_include_directories(LuaInterpreter PUBLIC include)
target_link_libraries(LuaInterpreter PUBLIC EngineCore)
target_link_libraries(LuaInterpreter PUBLIC lua_static)