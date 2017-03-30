/***********************************************************************************
 * Lightweight Scheduler Library for Teensy LC/3.x
 * Copyright (c) 2016, Colin Duffy https://github.com/duff2013
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ***********************************************************************************
 *  task.h
 *  Teensy 3.x/LC
 ***********************************************************************************/

#ifndef TASK_h
#define TASK_h

#include "Arduino.h"
/*****************************************************
 * This allows yield calls in a ISR not to lockup,
 * the kernel. Uncomment if any ISR calls yield in
 * its handler code.
 *****************************************************/
//#define USE_INTERRUPTS
/*****************************************************
 *----------------End Editable Options---------------*
 *****************************************************/

typedef void ( * task_func_t )( void *arg );

#ifdef __cplusplus
extern "C" {
#endif
    inline uint32_t sys_acquire_lock( volatile unsigned int *lock_var );
    inline uint32_t sys_release_lock( volatile unsigned int *lock_var );
#ifdef __cplusplus
}
#endif

#define TYPE uint32_t
#define TASK_LOCK( lock ) \
for ( TYPE __ToDo = sys_acquire_lock( &lock );  __ToDo;  __ToDo = sys_release_lock( &lock ) )
//////////////////////////////////////////////////////////////////////
// Get lock
//////////////////////////////////////////////////////////////////////
extern inline uint32_t sys_acquire_lock( volatile unsigned int *lock ) {
    
    do {
        //__enable_irq();
        yield( );
        //__disable_irq();
    }
    while ( !__sync_bool_compare_and_swap( lock, 0, 1 ) );
    //__enable_irq();
    return *lock;
}
//////////////////////////////////////////////////////////////////////
// Release lock
//////////////////////////////////////////////////////////////////////
extern inline uint32_t sys_release_lock( volatile unsigned int *lock ) {
    //__disable_irq();
    asm volatile ( "nop" ::: "memory" );
    *lock = 0;
    //yield( );
    //__enable_irq();
    return *lock;
}
//////////////////////////////////////////////////////////////////////
// Task Struct - save register
//////////////////////////////////////////////////////////////////////
enum TaskState {
    TaskCreated,        // task created, but not yet started
    TaskPaused,         // task is paused
    TaskExecuting,      // task is executing
    TaskReturned,       // task has returned
    TaskDestroyable,	// task can terminate
    TaskInvalid         // task is invalid 
};
#endif
