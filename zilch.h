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
 *  zilch.h
 *  Teensy 3.x/LC
 ***********************************************************************************/

#ifndef ZILCH_h
#define ZILCH_h
#ifdef __cplusplus
#include "utility/task.h"
#include "utility/mem_manger.h"
/**************************************************
 * This allows yield calls in a ISR not to lockup,
 * the kernel. Uncomment if any ISR calls yield in
 * its handler code.
 **************************************************/
//#define USE_INTERRUPTS

#define MPSS(STACK_SIZE) (((STACK_SIZE+28)/8)+1)*8

class Zilch {
private:
public:
    Zilch                       ( uint32_t override_pattern = 0xCDCDCDCD ) ;
    TaskState create            ( task_func_t task, size_t stack_size, void *arg );
    TaskState createDestroyable ( task_func_t task, size_t stack_size, void *arg );
    void      begin             ( void );
    void      sync              ( void );
    void      restartAll        ( void );
    TaskState pause             ( task_func_t task );
    TaskState resume            ( task_func_t task );
    TaskState restart           ( task_func_t task );
    TaskState state             ( task_func_t task );
    uint32_t  freeMemory        ( task_func_t task );
    void      setMemoryWaterMark( uint16_t waterMark );
};
#endif
#endif
