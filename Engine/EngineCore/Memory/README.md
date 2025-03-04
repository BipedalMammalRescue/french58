# Core::Memory Namespace

This namespace contains everything related to SigourneyEngine's custom memory management system.

It started with HighIntegrityAllocator, which was developed for the now-depreciated custom low-level scripting language.

I still think there's value to write and perfect a custom allocation system, thus I kept it and will be use it for a few data structures in the mission critical code paths, such as search trees.

## B+ Tree (bp_tree.h, bp_tree.cpp)

The B+ Tree data structure is one of my favorite implementations of a search tree.

This implementation is largely based on prof. John Cho's lecture on database management system indexing.

## NOTES

TODO: regarding the new allocator, user probably CANNOT access the values in the allocator using pointers, but they must use ID to access them.
TODO: how to implement data relation in this case? it won't make as much sense to build an index tree into this kind of allocator. (maybe there's a map data structure that works by burning through its nodes instead of jumping between them?)

TODO: need to change leaf-node deletion algorithm, when merging with right sibling it should copy right sibling into the target for a cleaner code path;
TODO: probably should change how bp_tree destructs itself? having a reference to the allocator on every node is quite wasting; the internal data structures can have a dispose method for freeing memory; this also helps with the not-so-safe delete method on allocator.

