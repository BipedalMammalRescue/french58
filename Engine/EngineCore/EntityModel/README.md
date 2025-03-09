# Entity Model

Namespace for utilities used for managing entities.

In the runtime, SigourneyEngine doesn't have a hard definition of an Entity, which make it differ slightly from a typical ECS solution.

However, for serialization purposes, it's very helpful to have the concept of entity-component grouping as a serialization strategy: its both more intuitive for a human user and more efficient to perform editing operations on; this object-oriented representation of game state will be compiled into multiple data streams that's normalized and stored on disk.

