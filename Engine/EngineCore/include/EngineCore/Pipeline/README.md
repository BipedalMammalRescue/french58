# Engine::Core::Pipeline

Namespace containing code used to define and execute the high-level data pipelines used in Sigourney Engine.

Pipelines' definition and naming are rather explicit, as explained with more detail below.

## Entity-Component Pipeline

Data used for scene description.

Roughly, the pipeline has these stages:
Scene File -> Editor Component -> Component Stream -> Runtime State.

Each state is respectively solved by:
Component Reflection,
Component Builder,
Component Loader.

This design leaves the runtime data management completely to client code.

## Asset Pipeline 

Data processed and compressed by the engine.

On the surface these data are very similar to component data, what differs is their lifetime: resource files usually have a wider lifetime, exiting across scene loads.
These data types' builders are also expected to have a much wider range of side-effects; while component data are expected to be simple data types that are serialized into component array or even field arrays,
assets such as mesh could be loaded, then immediately freed from memory after being uploaded to the GPU.

This pipeline has the following stages:
Raw Asset -> Built Asset -> Load Asset -> Void

Each stage is solved by:
Data Builder,
Data Loader,
Data Unloader (this is special, since we expect these assets to have side-effects, they need custom unload logic)
