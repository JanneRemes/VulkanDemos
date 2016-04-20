Demo 03: Double Buffering
=========================

This demo is the same as Demo 02, but it shows how to use dual set of buffers, semaphores and fences to do double buffering, i.e. instead of synchronizing the CPU with the GPU every frame, it does it every two frames, so the CPU is free to calculate the new frame while the GPU is busy rendering/presenting the previous one.

