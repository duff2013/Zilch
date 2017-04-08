Zlich_Beta v3.5
==========

Lean and mean scheduler library for the Teensy 3.2 currently. Zilch sits in between the single task big loop program and a RTOS. Based on fibers, it performs a context switch every time the 'yield' function is called. Since a lot of Teensyduino's core has 'yield' placed throughout it this library works quite well. Some things that make it useful is that each task only runs at the lowest priority possible on cortex processor in round robin fashion. This makes it good for Audio projects since no task will preempt and block any Audio library processing interrupts. 
