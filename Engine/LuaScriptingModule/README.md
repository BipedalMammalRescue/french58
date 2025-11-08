# Lua Scripting

Before I find a suitable visual scripting framework to wrap the engine API into, lua will take up the missing piece.

## Migration to visual scripting

The engine API and the modular design of the engine makes it that multiple scripting languages will be able to coexist,
therefore on the engine side there won't be much of a migration per se.

In fact I'm very inclining to keep multiple scripting systems using the same engine api.

## Why not C++?

Native scripting also exists in the form of injecting event systems through the event manager (in module code) or registering event systems at the game loop (game code).
The down side of these is that they don't have access to data, as the build system only builds a piece of data based on cross references in other data files.
Hard-coding a reference to a piece of data is therefore a very possible occurance of crashing the game due to missing data.

### C++ scripting and data access

There is a viable workaround for data access in C++: add a field in the component to hold reference to a piece of data, then have the event system loop through the components from the module's state.

For obvious reasons I prefer to have a data-driven approach for any inter-module logic.