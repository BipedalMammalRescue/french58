# Sigourney Engine

A collection of programming language and game engine-related projects made for fun.

## Parser Framework and Semantic Machine (Obselete)

A general-purpose parser that can be used to define and parse context-free grammar.
The Semantic Machine is a use case where I defined a simple functional language that uses the C-style invocation (foo.bar()) instead of LiSP style ((bar; foo)).

This section of the engine is no longer used, scripting language will be redesigned to be pure-data.

## EngineCore

The engine core runtime support library. 
By itself EngineCore doesn't do anything, it only supplies the necessary data structures and platform protocols to run a game.

## Game

The entry point project that leads to running the game. Depends on EngineCore and every xxModule project.

## "XxxModule" C++ Projects

Modules are high-level engine extensions; each module defines its own asset and component types, as well as providing one or more Module classes to be registered with the engine.

# Acknowledgements

As this project has become my exploration of setting up a completely FOSS workspace for my personal work, I would like to include a section acknowledging the open source projects I used in Sigourney Engine's development (the list may not be complete):

CMake: Sigourney Engine's build system, as well as almost all of her dependencies.

NeoVim (and NVChad): Editor and IDE used for this project.

