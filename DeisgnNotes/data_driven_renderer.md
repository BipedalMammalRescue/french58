# Data-Driven Renderer

The renderer uses information coming from three places to decide how to render a mesh:

1. Pipeline Prototype: defines a series of injected or material-defined parameters and their locations in the shader.
1. Render Pipeline: instantaites a pipeline prototype by adding a vertex shader and a fragment shader to it.
1. Material: defines name-matching parameters to fill in to the pipeline.

During render time, the renderer sorts all objects by pipeline, 
then for each pipeline,
bind the constant factors such as gpu pipeline and statically injected parameters;
then for each object, load the material and fill in the material parameters;
lastly load in the dynamically injected parameters.

## Multi-threaded renderer resource management

Say the game is divided into, in the rendering module's perspective, a simulation thread and a rendering thread.
The simulation thread will be responsible for creating the "actual" resources, such as a texture or a buffer on the GPU, while the renderer thread is responsible for sorting them and accessing them using an efficient indexing scheme.

An ideal workflow would be follows:

1. Simulation thread gets a notification from the engine to index an asset, which is turned into a SDL_GPUTexture pointer
1. Simulation thread sends a command to the rendering thread to create a *renderer resource*, uniquely identified by an integer, and to assign the newly created texture to it with necessary metadata (this increments a counter only used on the simulation thread)
1. Simulation thread then moves on the change its internal bookkeeping to make sure it knows the asset id corresponds to the renderer resource id
1. Renderer thread picks up the resource creation and assignment command (it's always sorted to run before any other thread), for which it will actually write (and allocate if necessary) the resource metadata: the resource metadata is only accessed by the rendering thread
1. During render, any command that involves an object referencing the loaded texture, such as a material, will provide the renderer resource id, from which the renderer thread can use to safely dereference a resource
1. On asset reload, simulation thread creates a new texture, then sends a resource reassign command; since the renderer always sorts resource related commands in the front, the new value will be used on the next frame being rendered.