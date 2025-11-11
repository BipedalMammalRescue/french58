# Streamlined Asset Management

I decided to overhaul the asset management system during the development of hot-reloading.
While the old system works very well for sequential, all-frontal loading, the possibility of hot-reloading makes the state of date less discrete.
In short, instead of having data as always-ready, hot-reloading poses the new state that a piece of data can be ready or loading;
this coincides with the problems solved and met by an concurrent asset loading system: the game needs to be running when data is being loaded.

## The old system

The old asset management system, specifically the runtime, is as follows:

1. every asset is a discrete blob
1. the life time of an asset has only two actions: load and unload (and unload isn't even well-defined since the engine can't switch scenes yet)
1. asset is always ready when it's needed (achieved by partial ordering between assets and loading all assets referenced by a scene before showing the scene itself)

It's incredibly simple to implement and allows full autonomous resource management by the modules.
The drawback, however, is that there's no room for concurrent behavior, i.e. the game needs to wait whenever a piece of data needs to be loaded.

## The proposed new system

Instead of having an asset be in the discrete state of on-disk and ready, the new system would have the following stages of asset loading:

1. Contextualize
1. Load
1. Index

### Sequential stage vs. concurrent stage

Before diving into the actual behaviors, I need to define the terminology that's going to be heavily used: sequential vs. concurrent stages.

A sequential stage is when the engine is running one engine callback at a time (typically a sequential callback delegate type).
It DOES NOT mean the stage is limited to single thread, as the delegate will have access to the task manager and can spawn and manage its own tasks.

On the other hand, an concurrent stage is when the engine runs every callback registered during that stage on the task system.
It DOES NOT mean this stage is automatically optimally parallelized, e.g. if you write a module that processes 1000000 prime numbers on a single thread the engine still needs to wait for that.

These are just the parallelization models the engine provides for the modules to choose from.

In the context of asset loading, therefore, a sequential stage is when one asset loading callback is execute at a time and an concurrent stage is when the engine sends every task/callback into the task system and have them run individually.

### Contextualize (sequential)

The first step in loading assets is contextualization.
The engine opens required files / loads necessary metadata (e.g. file size, probably the most important one) and asks the asset type's contextualizer callback to generate one or more ASSET LOADING CONTEXTS.
This is the stage where the module code needs to allocate memory for where the loaded file should reside.
As the size is provided, the module is allowed to specify the following:

1. Request a *TRANSIENT BUFFER* from the engine runtime.
This buffer is allocated in a large chunk and only available to use during this loading process.
1. Allocate *MODULE BUFFER* from module state.
Module code is required to make the pointer point to a sufficiently sized buffer allocated from the module state.

Either way, the module DOES NOT participate in the actual file I/O operations.
The engine will make sure the file is fully loaded into the destination buffer.

The loading context may include more information in the future, such as segmented loading by loading data into multiple buffers based on range.
The idea is that it's a piece of normalized data that instructs the engine to *store* the loaded asset (as opposed to loading it, as file accesses are fully abstracted out by design).

### Load (concurrent)

The engine fully manages this stage.
Based on the loading context returned from the contexualize stage, the engine runtime reads the file and puts them into the destination buffers.
Once the buffers are filled, the context is sent back to the next stage.

The engine runtime implementation decides how to execute this stage.
For the current iteration (and most likely all the way before forking into the next project), assets are stored on disk as loose files, and the engine opens all of them, and run every loading context concurrently. 

### Index (sequential)

After a loading context is fully loaded, it's returned to the main thread and ready to be indexed.
This is what the Game Engine Architecture book call "post-loading initialization".
I decided to reinvent a well-known term mostly because the design pattern I'm most familiar with is to store data in normalized blobs and construct indices for them as needed.
Either way, this is the part in the asset life cycle when control returns to the module code: the data is loaded into the destination buffer already, and the module can now modify its runtime state to reflect them.
This is also the last time *TRANSIENT BUFFERS* are valid, so things like GPU uploads may happen in this stage.

Note that it seems to make more sense to add a stage between load and indexing that allows the module code to do something with the loaded data.
I may revisit and add that design once the new asset management model is implemented and tested.

## What changed

Besides the obvious addition of a massive amount of complexity, the biggest change is the addition of a transitional state in the asset lifecycle: 
instead of waiting the asset to be fully loaded (load), each module now needs to mutate its state in preparation for asset (contextualize) then acknowledge the availability of asset (index). 

Asynchronous asset loading schemes are therefore greatly facilitated because the game will continue running, in a maybe undesirable but reasonable state, while the asset is *becoming* available.

## Acknowledgements

1. [This Bitsquid blog post about rendering resources](https://bitsquid.blogspot.com/2017/02/stingray-renderer-walkthrough-2.html).
While it's not talking about the exact same problem, the idea of a resource loading context as a way to explicitly configure engine behavior without directly interacting with IO operations is a major inspiration for the new design.
1. Game Engine Architecture.
Specifically the resource management chapter for obvious reasons.
