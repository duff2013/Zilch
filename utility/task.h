/*
 ||
 || @file 		task.h
 || @version 	0.4
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

#ifdef __cplusplus
extern "C" {
#endif
    inline uint32_t sys_acquire_lock( volatile unsigned int *lock_var );
    inline uint32_t sys_release_lock( volatile unsigned int *lock_var );
    inline void TaskUseInterrupt(enum IRQ_NUMBER_t interruptName);
#ifdef __cplusplus
}
#endif

#define TYPE uint32_t scratch = 0
#define TASK_LOCK( lock ) \
for ( TYPE, __ToDo = sys_acquire_lock( &lock );  __ToDo;  __ToDo = sys_release_lock( &lock ) )
//////////////////////////////////////////////////////////////////////
// Task locking routines, inspired by PJRC.com SPI transactions.
//////////////////////////////////////////////////////////////////////
static uint8_t  interruptMasksUsed;
static uint32_t interruptSave[(NVIC_NUM_INTERRUPTS+31)/32];
static uint32_t interruptMask[(NVIC_NUM_INTERRUPTS+31)/32];
//-----------------------------------------------------------------------
extern inline void TaskUseInterrupt( enum IRQ_NUMBER_t interruptName ) {
    uint32_t n = ( uint32_t )interruptName;
    if (n >= NVIC_NUM_INTERRUPTS) return;
    interruptMasksUsed |= (1 << (n >> 5));
    interruptMask[n >> 5] |= (1 << (n & 0x1F));
}
//-----------------------------------------------------------------------
extern inline uint32_t sys_acquire_lock( volatile unsigned int *lock ) {
    do { yield( ); }
    while ( !__sync_bool_compare_and_swap( lock, 0, 1 ) );
    if ( interruptMasksUsed ) {
        if ( interruptMasksUsed & 0x01 ) {
            interruptSave[0] = NVIC_ICER0 & interruptMask[0];
            NVIC_ICER0 = interruptSave[0];
        }
#if NVIC_NUM_INTERRUPTS > 32
        if ( interruptMasksUsed & 0x02 ) {
            interruptSave[1] = NVIC_ICER1 & interruptMask[1];
            NVIC_ICER1 = interruptSave[1];
        }
#endif
#if NVIC_NUM_INTERRUPTS > 64 && defined( NVIC_ISER2 )
        if ( interruptMasksUsed & 0x04 ) {
            interruptSave[2] = NVIC_ICER2 & interruptMask[2];
            NVIC_ICER2 = interruptSave[2];
        }
#endif
#if NVIC_NUM_INTERRUPTS > 96 && defined( NVIC_ISER3 )
        if ( interruptMasksUsed & 0x08 ) {
            interruptSave[3] = NVIC_ICER3 & interruptMask[3];
            NVIC_ICER3 = interruptSave[3];
        }
#endif
    }
    return *lock;
}
//-----------------------------------------------------------------------
extern inline uint32_t sys_release_lock( volatile unsigned int *lock ) {
    asm volatile ( "" ::: "memory" );
    *lock = 0;
    if ( interruptMasksUsed ) {
        if ( interruptMasksUsed & 0x01 ) {
            NVIC_ISER0 = interruptSave[0];
        }
#if NVIC_NUM_INTERRUPTS > 32
        if ( interruptMasksUsed & 0x02 ) {
            NVIC_ISER1 = interruptSave[1];
        }
#endif
#if NVIC_NUM_INTERRUPTS > 64 && defined( NVIC_ISER2 )
        if ( interruptMasksUsed & 0x04 ) {
            NVIC_ISER2 = interruptSave[2];
        }
#endif
#if NVIC_NUM_INTERRUPTS > 96 && defined( NVIC_ISER3 )
        if ( interruptMasksUsed & 0x08 ) {
            NVIC_ISER3 = interruptSave[3];
        }
#endif
    }
    return *lock;
}
//-----------------------------------------------------------------------
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
