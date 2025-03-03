# Core::Memory Namespace

This namespace contains everything related to SigourneyEngine's custom memory management system.

It started with HighIntegrityAllocator, which was developed for the now-depreciated custom low-level scripting language.

I still think there's value to write and perfect a custom allocation system, thus I kept it and will be use it for a few data structures in the mission critical code paths, such as search trees.

## B+ Tree (bp_tree.h, bp_tree.cpp)

The B+ Tree data structure is one of my favorite implementations of a search tree.

This implementation is largely based on prof. John Cho's lecture on database management system indexing.

## NOTES

TODO: need to change leaf-node deletion algorithm, when merging with right sibling it should copy right sibling into the target for a cleaner code path;
TODO: probably should change how bp_tree destructs itself? having a reference to the allocator on every node is quite wasting; the internal data structures can have a dispose method for freeing memory; this also helps with the not-so-safe delete method on allocator.

