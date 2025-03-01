# Refactor Homogeneous Storage Algorithm

While the current implementation achieves O(1) insertion and deletion, it introduces a big amount of overhead per element;
this is fine for large structures like a B+ tree node, but for smaller structures like runtime component types and some GPU resource representation types it can be rather wasteful.
Instead of writing a new "small-data" stroage class, the homogeneous stroage class should just be refactored into a less wasteful usage.

1. Eliminate explicit free pointer: the pointer from one free block to the next should be written inside a freed payload region.
2. Owner backtrack: instead of putting a full-length pointer to the owner storage instance, the size of allocated pointer should be stored in a more compact format. TODO: figure out how?