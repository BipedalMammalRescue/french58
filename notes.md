# Delivered

Load data using asset manager with reflection.

Rendering a simple, flat cube.

Porting codebase to SDL3 GPU.

# P1

Working implementation of Entity-Component-System
    B+ Tree for indexing components
    ranged B+ tree operations for Entity queries
    re-design allocator to support low-per-element overhead

Dynamic camera struct (how does glm's code produce its perspective matrix?).

Add texture sampler to pipeline; supply filler texture if texture isn't included (magenta?).

Phong shading for a singular light source (using a temp method to directly specify from renderer module).

# P2

Finish ECS data pipeline (transform, entity manager, component manager).

Think memory management strategy through: whether there is a need for custom allocator? whether to mix use of custom allocator with STL?

# P3

Add UTF-8 decoding when dealing with text files.

Add special UTF-8 decoded string field type support in reflection.
