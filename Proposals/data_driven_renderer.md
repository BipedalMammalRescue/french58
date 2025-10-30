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