# French58

A 3D game engine powered by SDL3, inspired by Bitsquid and Godot Engine.

## EngineCore

The engine core runtime support library. 
By itself EngineCore doesn't do anything, it only supplies the necessary data structures and platform protocols to run a game.

## Game

The entry point project that leads to running the game. Depends on EngineCore and every xxModule project.

## "XxxModule" C++ Projects

Modules are high-level engine extensions; each module defines its own asset and component types, as well as providing one or more Module classes to be registered with the engine.

# Acknowledgements

CMake: C++ code build system, as well as almost all of her dependencies.

Mu-Thr: Data build system SDK used to construct data build tools including EntityBuilder and AssetBuilder.

vscode: Primary IDE.

clangd, clang: compiler and IDE feature support.

.Net: most tools here are written in C# and runs on .Net rutnime.
