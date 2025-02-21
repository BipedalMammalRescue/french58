# dependencies
add_subdirectory(lib/tinyobjloader)

# builders
add_subdirectory(MeshBuilder)
target_link_libraries(MeshBuilder PUBLIC tinyobjloader)
