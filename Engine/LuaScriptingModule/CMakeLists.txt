add_library(LuaScriptingModule STATIC
    src/api.cpp
    src/lua_executor.cpp
    src/lua_script.cpp
    src/lua_scripting_module.cpp
    src/script_node.cpp
)

target_include_directories(LuaScriptingModule PUBLIC include)
target_link_libraries(LuaScriptingModule PUBLIC lua_static)
target_link_libraries(LuaScriptingModule PUBLIC EngineCore)
target_link_libraries(Engine PUBLIC LuaScriptingModule)

add_executable(LuaInterpreter 
    src/lua_interpreter.cpp
)

target_link_libraries(LuaInterpreter PUBLIC Engine)