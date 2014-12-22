/*
 ||
 || @file 		task.h
 || @version 	0.2
 || @author 	Colin Duffy
 || @contact 	cmduffy@engr.psu.edu
 || @author 	Warren Gay
 || @contact 	VE3WWG
 ||
 || @description
 || This is a very light weight fibers (aka coroutines) library,
 || managed by a singular instance of class Fibers.
 ||
 || @license
 || | Copyright (c) 2014 Colin Duffy, (C) Warren Gay VE3WWG
 || | This library is free software; you can redistribute it and/or
 || | modify it under the terms of the GNU Lesser General Public
 || | License as published by the Free Software Foundation; version
 || | 2.1 of the License.
 || |
 || | This library is distributed in the hope that it will be useful,
 || | but WITHOUT ANY WARRANTY; without even the implied warranty of
 || | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 || | Lesser General Public License for more details.
 || |
 || | You should have received a copy of the GNU Lesser General Public
 || | License along with this library; if not, write to the Free Software
 || | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 || #
 ||
 */

#ifndef TASK_h
#define TASK_h

#include "Arduino.h"

#define MAX_TASKS 32

#ifdef __cplusplus
extern "C" {
#endif
    inline uint32_t sys_acquire_lock( volatile unsigned int *lock_var );
    inline uint32_t sys_release_lock( volatile unsigned int *lock_var );
    void TaskUseInterrupt(enum IRQ_NUMBER_t interruptName);
#ifdef __cplusplus
}
#endif
#define TYPE uint32_t scratch = 0
#define TASK_LOCK( lock ) \
for ( TYPE, __ToDo = sys_acquire_lock( &lock );  __ToDo;  __ToDo = sys_release_lock( &lock ) )

typedef void ( * task_func_t )( void *arg );
typedef void ( * loop_func_t )( void );

enum TaskState {
    TaskCreated,	// task created, but not yet started
    TaskPause,	    // task is paused
    TaskExecuting,	// task is executing
    TaskReturned,	// task has terminated
    TaskInvalid		// task is invalid 
};
#endif
