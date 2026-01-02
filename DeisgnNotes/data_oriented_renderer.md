# Data-Oriented Renderer

After learning more about graphics programming I decided to ditch the original idea of the data-driven renderer.
In fact the entire renderer has been re-architectured:
the new design has the renderer sitting in the core engine library; in fact it will be so far the most complicated and equally important component of the core engine.

This implementation is based on [this GDC talk](https://www.youtube.com/watch?v=1Sb3s7Xie4M&t=2387s).

## Frame graph and the critical path

The aforementioned talk goes through the basic idea of a frame graph, where multiple stages of rendering is connected (and partially ordered) by the input and output resources they reference.

I'm going to make some simplifications:

1. A rendering resource is reduced to just a *render target* now. It can be anything from a color attachment to a depth stencil attachment to whatever compute shaders are allowed to have.
1. The core engine library owns part of the graph directly. This is what a "critical path" refers to here, a small set of pre-defined render targets and a handful of render passes that eventually get something onto the swpachain for submission.
1. Engine modules use a *fucking complicated* set of callbacks, much like the event callbacks, to add render targets (transient) and render passes to the renderer, making the full render graph.

There are *a lot* of systems and small features that need to be put in place for this design to even work at all:

1. Resource management, as will be mentioned below.
1. Graph algorithms; I used to do this stuff in college but god knows how much knowledge I've returned to Dr. Darwiche after my capstone project.
1. Memory allocation: Frostbite is a commercial engine made by big brain people. How tf am I going to manage the transient memory pool for render targets?

## Two frames in flight

Look this up on the internet. 
It's very straightforward.
The only note I should add is that I'm treating this as a synchronization mechanism only for the command buffer.
I'm sure this statement isn't entirely true but it's a simplification helpful for my work.

## Multi-threaded, multi-frame renderer resource management

From the renderer's point of view, the engine is divided into a gameplay thread and a render thread. 
Both of these would be considered "simulation" (I think) when we pull the camera back to include the accelerator's role in all these.
Resources such as storage buffers and image texture samplers now becomes a problem:

1. If the simulation only ever allocate new resources, not exceeding the bindless array length, and never modifies the uploaded resources, then there's actually never a problem.
1. If the simulation needs to *replace* or *destroy* an existing resource, we run the risk of destroying a resource that's still referenced by a frame in flight. 

There's no reliable way to tell when the last command that referenced a certain resource has done rendering, therefore we need a *latent* resource management scheme.

Assuming we have X frames in flight (this number will almost always be 2, I just want to keep it parametrized as a way to force consistency across the engine), on frame x we replaced texture #n with a new one. 
The simulation code will do whatever it needs to make the new image available, then call the API to replace the descriptor in the texture array with the new image descriptor.

The old descriptor, however, is not immediately destroyed, but instead added to a recycle queue, with a special tag saying it's freed on frame #n (note this frame number is circular).
At the *beginning* of every frame (say #m), before the renderer waits for the current command buffer to finish (the previous incarnation of frame #m is still running probably), one of the simulation threads goes in and finally destroys every object in the recycle queue with frame id #m.
This is because when the frames in flight finally rotates back to frame #m, it means every command buffer before the previous incarnation of #m has done rendering.

As to how tf am I going to update storage buffers, no idea just yet.
Those aren't objects that are wholesale destroyed and replaced, but I guess life will find a way.

## Gameplay vs render thread vs GPU

The gameplay thread and the render thread will have *two* different copies of the game state.
Previously in the engine every module is asked to submit a initalization function that creates a module state; that state is distinctively the gameplay state, and the game loop itself runs on the gameplay state.
In order to render anything with the engine, a render-capable module needs to submit a new piece of state creation function (with the gameplay state as one of its inputs).
The new state is *only* accessed on the renderer thread.
During the gameplay thread's execution it collects a series of state change events.
At the end of the gameplay frame, the gameplay thread signals a set of semaphores to notify to the renderer to process the fresh event stream that contains the new updates.
This double-buffered event stream is the *only* means for the gameplay state to communicate with the render state.

Note that this also means we allow the gameplay thread to be one frame ahead of the rendering thread, almost always.
This has nothing to do with the differentials between the render thread and the GPU, which is managed by the frames in flight thing mentioned earlier.

Once the states are synchronized by processing the event stream, the render thread runs a set of callbacks supplied by each render-capable module to populate a command buffer, after the render thread has waited for that command buffere to be done.
Then the command buffer is submitted and presented to the swapchain, all that jazz.
