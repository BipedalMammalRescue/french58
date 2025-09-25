# Memory Structure

## Static, Non-Streaming Scenes

Sigourney Engine is not designed for scene/level streaming, therefore all memory is allocated up-front when a scene loads.
This design provides many obvious simplifications of the memory management mechanisms needed for the engine.

## Root Stack

Root stack (or root allocator as it's sometimes referred to in code) is the memory buffer that's responsible for most tasks the core engine library carries out. 
It's a contiguous buffer alloated upfront when the game starts, where various runtime services will request memory from during the execution of the game.

## Asset Buffer

Asset buffer is the first block of memory in the root stack. 
It includes two sections: asset data and asset index.
Asset index is a translation tool that translates unsigned 64-bit integer identifiers to asset memory addresses.

## Scene Buffer

Scene buffer is allocated right after asset buffer.
More to add when scene loading is implemented.