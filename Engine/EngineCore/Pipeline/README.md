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
