/*
 ||
 || @file 		zilch.cpp
 || @version 	0.2
 || @author 	Colin Duffy
 || @contact 	cmduffy@engr.psu.edu
 || @author 	Warren Gay
 || @contact 	ve3wwg@gmail.com
 ||
 || @description
 || Light weight task scheduler library, based off the awesome fibers
 || library by Warren Gay. https://github.com/ve3wwg/teensy3_fibers
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

#include "zilch.h"
#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif
    void       init_stack   ( uint32_t main_stack, uint32_t pattern_override );
    void       task_create  ( task_func_t func, uint32_t stack_size, volatile void *arg );
    TaskState  task_state   ( task_func_t func );
    TaskState  main_state   ( loop_func_t func );
    TaskState  task_sync    ( task_func_t func );
    TaskState  task_restart ( task_func_t func );
    TaskState  task_pause   ( task_func_t func );
    TaskState  task_resume  ( task_func_t func );
    uint32_t*  task_size    ( task_func_t func );
    uint32_t*  main_size    ( loop_func_t func );
#ifdef __cplusplus
}
#endif

Zilch::Zilch( uint16_t main_stack_size, const uint32_t pattern ) {
    NVIC_SET_PRIORITY(IRQ_SOFTWARE, 0xFF); // 0xFF = lowest priority
    NVIC_ENABLE_IRQ(IRQ_SOFTWARE);
    init_stack( main_stack_size, pattern );
}

TaskState Zilch::create( task_func_t task, size_t stack_size, volatile void *arg ) {
    int s_size = stack_size;
    // Round stack size to a word multiple
    s_size = ( stack_size + sizeof (uint32_t) ) / sizeof (uint32_t) * sizeof (uint32_t);
    task_create( task, s_size, arg );
}

TaskState Zilch::state( task_func_t task ) {
    TaskState p = task_state( task );
    return p;
}

TaskState Zilch::resume( task_func_t task ) {
    TaskState p = task_resume( task );
    return p;
}

TaskState Zilch::pause( task_func_t task ) {
    TaskState p = task_pause( task );
    return p;
}

TaskState Zilch::state( loop_func_t task ) {
    TaskState p = main_state( task );
    return p;
}

TaskState Zilch::sync( task_func_t task) {
    TaskState p = task_sync( task );
    return p;
}

TaskState Zilch::restart( task_func_t task ) {
    TaskState p = task_restart( task );
    return p;
}

uint32_t *Zilch::size( task_func_t task ) {
    return task_size(task);
}

uint32_t *Zilch::size( loop_func_t task ) {
    uint32_t* tmp = main_size(task);
    return tmp;
}

