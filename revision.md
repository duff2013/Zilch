><b>Updated (9/22/17 v3.6)</b><br>
* increase sketch stack sizes.
* minor fixes for starting tasks.

><b>Updated (4/8/17 v3.5)</b><br>
* Fixed many bugs and speedup of memory manager.
* Fixed destroyable tasks, now deletes memory correctly.
* Fixed inline asm for starting tasks and returning tasks.
* Only using MSP stack pointer now, will switch to PSP in future.

><b>Updated (3/30/17 v3.4)</b><br>
* Fixed documentation of examples and readme.

><b>Updated (3/29/17 v3.3)</b><br>
* Offical Zilch release update.

><b>Updated (3/14/17 v3.2)</b><br>
* Code cleanup, removed unused code.

><b>Updated (3/9/17 v3.1)</b><br>
* Now not using Eli Bendersky's memory manager, inhouse mem manger used now.
* Tasks can be freed along with memory.

><b>Updated (1/21/17 v3.0)</b><br>
* Now uses memory pool for stack space.
* Now uses Eli Bendersky's memory manager.
* Updated examples to reflect new API.
* Kernal task uses MSP, all user tasks use PSP stack pointers.
* Kernal task monitors os memory.

><b>Updated (12/3/16 v2.1)</b><br>
* Optimize LC context switch'.

><b>Updated (12/2/16 v2.0)</b><br>
* Teensy LC support'.

><b>Updated (12/1/16 v1.4)</b><br>
* Fixed 'restart' task'.
* add examples for restarted and paused tasks.

><b>Updated (11/30/16 v1.3)</b><br>
* Added 'pause', 'resume' and 'state' functions.
* Sync still needs work but is there too.

><b>Updated (11/29/16 v1.2)</b><br>
* Now using PSP stack pointer.
* Restarting a returned task works now.
* Got rid of unused code.

><b>Updated (11/22/16 v1.1)</b><br>
* Now loop is not used.
* Faster context switch.
* 'OS' starts with call to begin function.

><b>Updated (11/17/16 v1.0)</b><br>
* Reworked and hopefully better version.

><b>Updated (11/13/16 v0.5)</b><br>
* Fixed task count that was incrementing wrong when creating new task.

><b>Updated (10/13/15 v0.5)</b><br>
* Fixed warning from task locking code.
