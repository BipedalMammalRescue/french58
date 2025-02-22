add_executable(Game
	application.cpp
	DummyAssetType.cpp
	DummySinkModule.cpp
	)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})