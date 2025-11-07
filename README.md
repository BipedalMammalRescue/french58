# Sigourney Engine

A collection of programming language and game engine-related projects made for fun.

## EngineCore

The engine core runtime support library. 
By itself EngineCore doesn't do anything, it only supplies the necessary data structures and platform protocols to run a game.

## Game

The entry point project that leads to running the game. Depends on EngineCore and every xxModule project.

## "XxxModule" C++ Projects

Modules are high-level engine extensions; each module defines its own asset and component types, as well as providing one or more Module classes to be registered with the engine.

# Acknowledgements

As this project has become my exploration of setting up a completely FOSS workspace for my personal work, I would like to include a section acknowledging the open source projects I used in Sigourney Engine's development (the list may not be complete):

CMake: Sigourney Engine's code build system, as well as almost all of her dependencies.
Mu-Thr: Data build system SDK used by Sigourney Engine.
vscode: Primary IDE.
clangd, clang: compiler and IDE feature support.
