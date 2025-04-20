# dependencies
add_subdirectory(lib/tinyobjloader)

# builders
# add_subdirectory(MeshBuilder)
# target_link_libraries(MeshBuilder PUBLIC tinyobjloader)

add_subdirectory(EntityBuilder)

# TODO: data builders should just grab every file in the source and build them into engine formats
