/*
 ||
 || @file 		task.h
 || @version 	0.3
 || @author 	Colin Duffy
 || @contact 	cmduffy@engr.psu.edu
 || @author 	Warren Gay
 || @contact 	VE3WWG
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

typedef void ( * task_func_t )( void *arg );
typedef void ( * loop_func_t )( void );


enum TaskState {
    TaskCreated,	// task created, but not yet started
    TaskPause,	    // task is paused
    TaskExecuting,	// task is executing
    TaskReturned,	// task has terminated
    TaskInvalid		// task is invalid 
};

#if defined(KINETISK)
typedef struct {
    // Saved Registers
    uint32_t        sp;         // Saved sp register
    uint32_t        r2;         // after the function has begun
    uint32_t        r3;         // Saved r3 etc.
    uint32_t        r4;
    uint32_t        r5;
    uint32_t        r6;
    uint32_t        r7;
    uint32_t        r8;
    uint32_t        r9;
    uint32_t        r10;
    uint32_t        r11;
    uint32_t        r12;
    void            *lr;        // Return address (pc)
    void            *arg;		// Startup arg value
    uint32_t        stack_size;	// Stack size for this main/coroutine
    uint32_t        initial_sp;	// Initial stack pointer (for restart)
    task_func_t     func_ptr;   // Task function
    enum TaskState  state;      // Current task state
    uint32_t        address;    // Address for swap fifo
} stack_frame_t;
#elif defined(KINETISL)
typedef struct {
    // Saved Registers
    uint32_t        r4;
    uint32_t        r5;
    uint32_t        r6;
    uint32_t        r7;
    uint32_t        r8;
    uint32_t        r9;
    uint32_t        sl;
    uint32_t        fp;
    uint32_t        sp;         // Saved sp register
    void            *lr;        // Return address (pc)
    task_func_t     func_ptr;   // Task function
    void            *arg;		// Startup arg value
    uint32_t        stack_size;	// Stack size for this main/coroutine
    uint32_t        initial_sp;	// Initial stack pointer (for restart)
    enum TaskState  state;      // Current task state
    uint32_t        address;    // Address for swap fifo
} stack_frame_t;
#endif
#endif
