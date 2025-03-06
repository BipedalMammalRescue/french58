# Core::Memory Namespace

This namespace contains everything related to SigourneyEngine's custom memory management system.

It started with HighIntegrityAllocator, which was developed for the now-depreciated custom low-level scripting language.

I still think there's value to write and perfect a custom allocation system, thus I kept it and will be use it for a few data structures in the mission critical code paths, such as search trees.

## B+ Tree (bp_tree.h, bp_tree.cpp)

The B+ Tree data structure is one of my favorite implementations of a search tree.

This implementation is largely based on prof. John Cho's lecture on database management system indexing.

## NOTES

How to implement a good and simple solution to the delta problem?
The template strategy class should be responsible for deciding what happens first 
Problem: derivatives aren't defined on 0.

KX + B 

the function returns the *next* allocation total to go from there;
