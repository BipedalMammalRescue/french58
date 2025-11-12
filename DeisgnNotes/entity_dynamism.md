# Entity Dynamism

Entities can be loaded (queued) and unloaded at any time.
The ability to signal a load is available to every user-implemented module, and subsequently usable in API scripting, including live-link.

## Additive entity composition and Root Entity

Entities can be loaded into strictly a tree structure.
Every, but one, entity has one and only parent entity, and numerous child entities.
The only exception in this scheme is the *ROOT ENTITY*, which is the first loaded entity in the game; it may be configured through data.
Root entity contains all the information that will be persistent throughout the entire game's lifetime.
Note that this lifetime is not necessarily the full timespan of the process, as the game runs through the gameloop object that can have multiple modes.

Some sample uses of the root entity includes:

1. Loading screen. 
If the programmer chooses to, she may activate a loading screen while waiting for another scene is loading.
Note that this black-screen-loading scheme is in fact a special case of the asynchronous asset/entity scheme: the game is always running and always in a correct-but-undesirable state.

1. Megapacks of shared resources. 
The root entity may contain special components that references data packas such as texture packas or audio packs.
As long as the module is implemented accordingly, the long lifetime of a root entity's asset can be used to front-load resources.

Every other entity is appended to the root entity.
Module code will be responsible for figuring out how data is appended for assets and components they author.

## Arbitrary entity removal

While entities need to be loaded in chunks as they are defined in entity files, they can be removed at any order.
Removing an entity also removes every child entity under it in the scene tree.

Like the exception in the above section, root entity cannot be removed.
However, any entity loaded with the root entity can be removed at any order, just like any other entity loaded afterwards.

## Entity initialization

Before entities hits the first presentation frame and after it's fully initialized, the gameplay programmer may update its state through evnets.
It's the equivalent of loading entity with a spatial offset in some other engines.

### Loading entities that don't have a spatial relation

Ehh, if you designed an entity without a spatial relation then I can only assume you know what you are doing and respect that:
the engine ignores attempts to update an non-existent spatial relation, unless the event is for *ATTACHING* a new component.

## Attach, detatch, entity, component

Both entity and component can be moved around in the scene tree.
Relations between entities are handled by the engine runtime, while components are generated as events and need to be explicitly handled by the module state.

Because these events aren't IO related, they can be processed at any time during the scene.
It's up to the module how to update the states.
E.g. the root module will re-calculate the parental offsets of the spatial relations for every re-attached entity.