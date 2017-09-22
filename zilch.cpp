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
 *  zilch.cpp
 *  Teensy 3.x/LC
 ***********************************************************************************/

#include "zilch.h"
#include "utility/task.h"
#include "utility/mem_manager.h"
#include "Arduino.h"

struct stack_frame_t {
    uint32_t        *sp;            // Saved sp register
    uint32_t        r4;
    uint32_t        r5;
    uint32_t        r6;
    uint32_t        r7;
    uint32_t        r8;
    uint32_t        r9;
    uint32_t        r10;
    uint32_t        r11;
    uint32_t        *r12;           // Scratch Register holds stack frame
    uint32_t        *lr;            // Return address (pc)
    uint32_t        address;        // Address for swap fifo
    uint32_t        *stack_top;     // Top of the stack(for restart)
    uint32_t        *stack_bottom;  // Bottom of the stack
    uint32_t        free_memory;    // Estimated memory usage
    task_func_t     ptr;            // Task function
    void            *arg;           // Startup arg value
    enum TaskState  state;          // Current task state
    stack_frame_t   *next;          // points to next tasks memory section
};

typedef struct {
    uint32_t                memory_fill_pattern;
    uint32_t                memory_water_mark;
    volatile stack_frame_t  *current_frame;
    stack_frame_t           *root_frame;
    uint8_t                 num_task;
    boolean                 begin;
    boolean                 tasks_to_destroy;
    mem_manager             mem;
} os_t;

#ifdef __cplusplus
extern "C" {
#endif
    void      init_stack  ( uint32_t memory_fill );
    stack_frame_t * task_create ( task_func_t func, mem_block_t *mem, void *arg );
    void      start_os                 ( void );
    void      task_sync                ( void );
    void      task_restart_all         ( void );
    TaskState task_state               ( task_func_t func );
    TaskState task_restart             ( task_func_t func );
    TaskState task_pause               ( task_func_t func );
    TaskState task_resume              ( task_func_t func );
    TaskState task_stop                ( task_func_t func );
    uint32_t  task_memory              ( task_func_t func );
    void      destroy_task             ( int index );
    __attribute__((noinline))
    stack_frame_t *remove_task_from_runlist2( volatile stack_frame_t *t );
    __attribute__((noinline))
    stack_frame_t *remove_task_from_runlist ( task_func_t func );
    __attribute__((noinline))
    stack_frame_t *add_task_to_runlist      ( task_func_t func );
    
#ifdef __cplusplus
}
#endif

static os_t os __attribute__ ((aligned (4)));

static void kernal( void *arg );

Zilch::Zilch( uint32_t override_pattern ) {
    os.memory_water_mark = 4;
    init_stack( override_pattern );
}

TaskState Zilch::create( task_func_t task, size_t stack_size, void *arg ) {
    mem_block_t *block;
    if ( os.root_frame == NULL ) {
        block = os.mem.alloc( 512, os.memory_fill_pattern );
        if ( block == NULL ) return TaskInvalid;
        task_create( kernal, block, arg );
        os.num_task = 1;
    }
    uint32_t num = os.num_task; // get current number of tasks
    block = os.mem.alloc( stack_size, os.memory_fill_pattern );
    if ( block == NULL ) return TaskInvalid;
    stack_frame_t *p = task_create( task, block, arg );
    os.num_task = ++num;// total number of tasks
    return p->state;
}

TaskState Zilch::createDestroyable ( task_func_t task, size_t stack_size, void *arg ) {
    mem_block_t *block;
    if ( os.root_frame == NULL ) {
        block = os.mem.alloc( 512, os.memory_fill_pattern );
        if ( block == NULL ) return TaskInvalid;
        task_create( kernal, block, arg );
        os.num_task = 1;
    }
    uint32_t num = os.num_task; // get current number of tasks
    block = os.mem.alloc( stack_size, os.memory_fill_pattern );
    if ( block == NULL ) return TaskInvalid;
    stack_frame_t *p = task_create( task, block, arg );
    p->state = TaskDestroyable;
    p->address = 0xFFFFFFFF;
    os.num_task = ++num;// total number of tasks
    return p->state;
}

void Zilch::begin( void ) {
    start_os( );
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

void Zilch::sync( void) {
    task_sync( );
}

TaskState Zilch::restart( task_func_t task ) {
    TaskState p = task_restart( task );
    return p;
}

TaskState Zilch::stop( task_func_t task ) {
    //task_restart_all( );
}

void Zilch::restartAll( void ) {
    task_restart_all( );
}

uint32_t Zilch::freeMemory( task_func_t task ) {
    return task_memory( task );
}

void Zilch::lowMemoryWaterMark( uint16_t threshold ) {
    os.memory_water_mark = threshold;
}

void Zilch::printMemoryHeader( void ) {
    Serial.print("Pool Address: ");
    Serial.println((uint32_t)os.mem.pool, HEX);
    for ( int i = 0; i < os.mem.poolSize( ); i++ ) {
        unsigned long mask  = 0x0000000F;
        mask = mask << 28;
        for ( unsigned int n = 8; n > 0; --n ) {
            Serial.print( ( ( ( uint32_t ) os.mem.pool[i] & mask ) >> ( n - 1 ) * 4 ), HEX );
            mask = mask >> 4;
        }
        Serial.print(" ");
        if ( ! ( ( i + 1 ) % 8 ) ) Serial.println( );
    }
}
/******************************************************************************************
  ________          ________          ________          ________          ________
_|        |________|        |________|        |________|        |________|        |________
*******************************************************************************************/
//////////////////////////////////////////////////////////////////////
// Tasks startup routine
//////////////////////////////////////////////////////////////////////
static void kernal( void *arg ) {
    while ( 1 ) {
        volatile stack_frame_t *p;
        for ( p = os.root_frame; p; p = p->next ) {
            
            uint32_t top_address       = (uint32_t)p->stack_top;
            uint32_t bottom_address    = (uint32_t)p->stack_bottom;
            
            uint32_t *top       = p->stack_top;
            uint32_t *bottom    = p->stack_bottom;
            uint32_t free       = 0;
            
            do {
                if ( *bottom++ == os.memory_fill_pattern ) free++;
                else break;
            } while ( top != bottom );
            p->free_memory = p->sp - p->stack_bottom;
            if ( free <= os.memory_water_mark ) {
                Serial.println("Possible Stack Overflow:");
                Serial.print("memory location:\t");
                Serial.println((uint32_t)p, HEX);
                Serial.print("memory free:\t\t");
                Serial.println(free);
                Serial.print("memory pressure:\t");
                Serial.println(p->free_memory);
                Serial.print("stack top:\t\t");
                Serial.println(top_address, HEX);
                Serial.print("stack bottom:\t\t");
                Serial.println(bottom_address, HEX);
                Serial.print("return stack:\t\t");
                Serial.println((uint32_t)p->ptr, HEX);
                task_pause(p->ptr);
            }
            if ( p->next == os.root_frame ) break;
        }
        yield();
    }
}

void hard_fault_isr( void ) {
    Serial.print( "os.current_frame: " );
    Serial.print( (uint32_t)os.current_frame->next, HEX );
    Serial.print( " | os.current_frame->next: " );
    Serial.println( (uint32_t)os.current_frame->next->next, HEX );
    uint32_t reg = 0x02;
    asm volatile( "MRS %[sp], MSP"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print("sp: ");
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r1], r1"   "\n" : [r1] "= r" ( reg ): : );
    Serial.print( "r1: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r2], r2"   "\n" : [r2] "= r" ( reg ): : );
    Serial.print( "r2: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r3], r3"   "\n" : [r3] "= r" ( reg ): : );
    Serial.print( "r3: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r4], r4"   "\n" : [r4] "= r" ( reg ): : );
    Serial.print( "r4: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r5], r5"   "\n" : [r5] "= r" ( reg ): : );
    Serial.print( "r5: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[r6], r6"   "\n" : [r6] "= r" ( reg ): : );
    Serial.print( "r6: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r7"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r7: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r8"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r8: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r9"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r9: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r10"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r10: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r11"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r11: " );
    Serial.println( reg, HEX );
    
    asm volatile( "MOV %[sp], r12"   "\n" : [sp] "= r" ( reg ): : );
    Serial.print( "r12: " );
    Serial.println( reg, HEX );
    
    while ( 1 ) if ( SIM_SCGC4 & SIM_SCGC4_USBOTG ) usb_isr( );
}

void start_os( void ) {
    if ( os.num_task <= 0 ) return;             // if no task return
    os.current_frame = os.root_frame;           // current frame starts as root
    void *arg = os.root_frame->arg;             // get root frame's arg
    stack_frame_t *p = os.current_frame->next;  // p point to the next frame in the list
    os.begin = true;                            // allow context switch
    __disable_irq( );
    // kernal uses msp and all tasks use the psp stack pointer.
    asm volatile(
                 "MSR MSP, %[kernal]"     "\n"
                 :
                 : [kernal] "r" ( os.current_frame->sp )
                 :
                 );
    /*asm volatile(
                 "MSR MSP, %[kernal]"     "\n\t"
                 "MSR PSP, %[task]"       "\n"
                 :
                 : [kernal] "r" ( os.current_frame->sp ), [task] "r" ( p->sp )
                 :
                 );*/
    
    __enable_irq( );
    os.root_frame->ptr( arg );          // call first frame's function, starts scheduler
    os.root_frame->state = TaskInvalid; // update state, after return.
    for (;;) yield( );                  // keep things rolling
}
//////////////////////////////////////////////////////////////////////
// Initialize main stack
//////////////////////////////////////////////////////////////////////
void init_stack( uint32_t memory_fill ) {
    os.memory_fill_pattern = memory_fill; // memory fill pattern
    os.num_task            = 0;           // number of tasks
    os.begin               = false;       // context switch don't start until true
    os.current_frame       = NULL;        // context switch frame pointer
    os.root_frame          = NULL;        // kernal frame pointer
    os.tasks_to_destroy    = false;
}
//////////////////////////////////////////////////////////////////////
// Task's launch pad
//////////////////////////////////////////////////////////////////////
static void task_start( void ) __attribute__((naked));
static void task_start( void ) {
    volatile stack_frame_t *p;
    asm volatile(
                 "push {r2-r12, lr}"     "\n\t"
                 "mov r4, r12"          "\n\t"// r12 points to the stack frame
                 "ldr r3, [r4, #64]"    "\n\t"
                 "mov r0, r3"           "\n\t"// r1 now holds the user supplied task function
                 "ldr r3, [r4, #60]"    "\n\t"
                 "mov r1, r3"           "\n\t"// r0 now holds void *arg for task call
                 "blx r1"               "\n\t"
                 "mov %[result], r4"    "\n"
                 : [result] "=r" ( p )
                 :
                 : "r0", "r1", "r2", "r3", "r4", "r12", "memory"
                 );
    // task is returned remove it from linked list
    p = remove_task_from_runlist2( p );
    // if p == NULL task and memory are removed
    if ( p != NULL ) p->state = TaskReturned;
    // task stops here with a call to yield
    yield( );
}
//////////////////////////////////////////////////////////////////////
// Set up a task to execute, will launch when yield switches in
//////////////////////////////////////////////////////////////////////
stack_frame_t *task_create( task_func_t func, mem_block_t *block, void *arg ) {
    uint32_t frame_size  = ( sizeof( stack_frame_t ) ) >> 2;// size of struct in words
    uint32_t address = os.num_task;                         // each task has unique address
    uint32_t stack_size = block->length - frame_size;
    uint32_t *stack = ( uint32_t * )block->block;
    stack_frame_t *p = ( stack_frame_t * )stack;
    *p = { 0 };
    if ( os.root_frame == NULL ) os.root_frame = p;
    p->sp           = ( uint32_t * )stack + stack_size + frame_size - 1;
    p->r12          = ( uint32_t * )p;
    p->lr           = ( uint32_t * )task_start;
    p->address      = address;
    p->stack_top    = ( uint32_t * )stack + stack_size + frame_size - 1;
    p->stack_bottom = ( uint32_t * )stack + frame_size;
    p->ptr          = func;
    p->arg          = arg;
    p->state        = TaskCreated;
    add_task_to_runlist( p->ptr );
    return p;
}
//////////////////////////////////////////////////////////////////////
// all invocations of yield in teensyduino api go through this now.
//////////////////////////////////////////////////////////////////////
#if defined(KINETISL)
void task_swap( stack_frame_t *prevframe, stack_frame_t *nextframe ) {
    asm volatile (
                  "MRS r3, PSP"         "\n\t" // move sp to r3
                  "STMIA r0!,{r3-r7}"   "\n\t" // Save to r0! from r3-r7
                  "MOV r2,r8"           "\n\t" // move r8 into r2
                  "MOV r3,r9"           "\n\t" // move r9 into r3
                  "MOV r4,sl"           "\n\t" // move sl into r4
                  "MOV r5,fp"           "\n\t" // move fp into r5
                  "MOV r6,ip"           "\n\t" // move ip into r6
                  "MOV r7,lr"           "\n\t" // move lr into r7
                  "STMIA r0!,{r2-r7}"    "\n\t" // Save to r0 from r2-r7*/
                  
                  "ADD r1,#20"          "\n\t" // increment r1 address 20
                  "LDMIA r1!,{r2-r7}"   "\n\t" // load r2-r7 from r1!
                  "MOV r8,r2"           "\n\t" // move r2 into r8
                  "MOV r9,r3"           "\n\t" // move r3 into r9
                  "MOV sl,r4"           "\n\t" // move r4 into sl
                  "MOV fp,r5"           "\n\t" // move r5 into fp
                  "MOV ip,r6"           "\n\t" // move r6 into ip
                  "MOV lr,r7"           "\n\t" // move r7 into lr
                  "SUB r1,#44"          "\n\t" // decrement r1 address 44
                  "LDMIA r1!,{r3-r7}"   "\n\t" // load r3-r7 from r1
                  "MSR PSP, r3"         "\n"   // move r3 into sp
                  );
}
#endif

void yield( void ) __attribute__((noinline));
void yield( void ) {
    
    if ( !os.begin ) return;
    
    volatile stack_frame_t *p1 = os.current_frame;
    volatile stack_frame_t *p2 = os.current_frame->next;
    os.current_frame  = p2;
    /*uint32_t fOut = p1->address;
    uint32_t fIn  = p2->address;
    if ( !fIn || !fOut ) {
        if ( !fIn && !fOut ) {
            
            uint32_t reg = 0x00;
            asm volatile(
                         "MSR CONTROL, %[msp]"  "\n\t"
                         "DSB"                  "\n\t"
                         :
                         : [msp] "r" ( reg )
                         :
                         );
            asm volatile (
                          "MOV r0, %[frameOut]"     "\n\t" // r0 holds p1
                          "MRS r3, MSP"             "\n\t" // move msp into r3
                          "STMIA r0,{r3-r12, lr}"   "\n\t" // Save r3-r12 + lr
                          "MOV r1, %[frameIn]"      "\n\t" // r1 holds p2
                          "LDMIA r1,{r3-r12, lr}"   "\n\t" // Restore r1(sp) and r4-r12, lr
                          "MSR MSP, r3"             "\n"   // Set new msp
                          : [frameOut] "+r" ( p1 )
                          : [frameIn] "r" ( p2 )
                          : "r3"
                          );
        } else if ( !fOut ) {
            
            uint32_t reg = 0x02;
            asm volatile(
                         "MSR CONTROL, %[psp]"  "\n\t"
                         "DSB"                  "\n\t"
                         :
                         : [psp] "r" ( reg )
                         );
            asm volatile (
                          "MOV r0, %[frameOut]"     "\n\t" // r0 holds p1
                          "MRS r3, MSP"             "\n\t" // move msp into r3
                          "STMIA r0,{r3-r12, lr}"   "\n\t" // Save r3-r12 + lr
                          "MOV r1, %[frameIn]"      "\n\t" // r1 holds p2
                          "LDMIA r1,{r3-r12, lr}"   "\n\t" // Restore r1(sp) and r4-r12, lr
                          "MSR PSP, r3"             "\n"   // Set new psp
                          : [frameOut] "+r" ( p1 )
                          : [frameIn] "r" ( p2 )
                          : "r3"
                          );
        } else {
            uint32_t reg = 0x00;
            asm volatile(
                         "MSR CONTROL, %[msp]"  "\n\t"
                         "DSB"                  "\n\t"
                         :
                         : [msp] "r" ( reg )
                         :
                         );
            asm volatile (
                          "MOV r0, %[frameOut]"     "\n\t" // r0 holds p1
                          "MRS r3, PSP"             "\n\t" // move psp into r3
                          "STMIA r0,{r3-r12, lr}"   "\n\t" // Save r3-r12 + lr
                          "MOV r1, %[frameIn]"      "\n\t" // r1 holds p2
                          "LDMIA r1,{r3-r12, lr}"   "\n\t" // Restore r1(sp) and r4-r12, lr
                          "MSR MSP, r3"             "\n"   // Set new msp
                          : [frameOut] "+r" ( p1 )
                          : [frameIn] "r" ( p2 )
                          : "r3"
                          );
        }
        
    } else {
        uint32_t reg = 0x02;
        asm volatile(
                     "MSR CONTROL, %[psp]"  "\n\t"
                     "DSB"                  "\n\t"
                     :
                     : [psp] "r" ( reg )
                     :
                     );
        asm volatile (
                      "MOV r0, %[frameOut]"     "\n\t" // r0 holds p1
                      "MRS r3, PSP"             "\n\t" // move psp into r3
                      "STMIA r0,{r3-r12, lr}"   "\n\t" // Save r3-r12 + lr
                      "MOV r1, %[frameIn]"      "\n\t" // r1 holds p2
                      "LDMIA r1,{r3-r12, lr}"   "\n\t" // Restore r1(sp) and r4-r12, lr
                      "MSR PSP, r3"             "\n"   // Set new psp
                      : [frameOut] "+r" ( p1 )
                      : [frameIn] "r" ( p2 )
                      : "r3"
                      );
    }*/
    
#if defined(KINETISK)
    asm volatile (
                  "MOV r0, %[frameOut]"     "\n\t" // r0 holds p1
                  "MRS r3, MSP"             "\n\t" // move sp into r3
                  "STMIA r0,{r3-r12, lr}"   "\n\t" // Save r3-r12 + lr
                  "MOV r1, %[frameIn]"      "\n\t" // r1 holds p2
                  "LDMIA r1,{r3-r12, lr}"   "\n\t" // Restore r1(sp) and r4-r12, lr
                  "MSR MSP, r3"             "\n"   // Set new sp
                  : [frameOut] "+r" ( p1 )
                  : [frameIn] "r" ( p2 )
                  : "r3"
                  ) ;

    /*asm volatile (
                  "MOV r1, %[begin]"                "\n\t" // r0 holds p1
                  "cbz r1, end"                     "\n\t" // r0 holds p1
                  "MOV r0, %[frameOut]"             "\n\t" // r0 holds p1
                  "MRS r3, MSP"                     "\n\t" // move sp into r3
                  "STMIA r0,{r3-r12, lr}"           "\n\t" // Save r3-r12 + lr
                  "MOV r1, %[frameIn]"              "\n\t" // r1 holds p2
                  "LDMIA r1,{r3-r12, lr}"           "\n\t" // Restore r1(sp) and r4-r12, lr
                  "MSR MSP, r3"                     "\n\t"   // Set new sp
                  "MOV %[frameOut], %[frameIn]"     "\n\t"
                  "end:"                            "\n" // r0 holds p1
                  : [frameOut] "+r" ( os.current_frame )
                  : [frameIn] "r" ( os.current_frame->next ), [begin] "r" ( os.begin )
                  );*/
    
#elif defined(KINETISL)
    task_swap( p1, p2 );
#endif
}
//////////////////////////////////////////////////////////////////////
// pass task state, pass loop state
//////////////////////////////////////////////////////////////////////
TaskState task_state( task_func_t func ) {
    mem_block_t *start = os.mem.allocList( );
    mem_block_t *end = start + 31;
    do {
        if ( start->block != 0 ) {
            stack_frame_t *p = ( stack_frame_t * )start->block;
            if ( p->ptr == func ) return p->state;
        }
    } while ( ++start != end );
    return TaskInvalid;
}
//////////////////////////////////////////////////////////////////////
// routine to block until selected task return's.
//////////////////////////////////////////////////////////////////////
void task_sync( void ) {
    /*task_frame_t *p;
    bool first_state = true;
    uint8_t first_state_address = 0;
    uint8_t last_state_address = 0;
    for (int i = 0; i < os.num_task; i++) {
        p = &os.task[i];
        if ( p->state == TaskCreated ) {
            task_restart( p->ptr );
            //os.current_frame = &os.frame[i];
            //void *arg = p->arg;
            //p->ptr( arg );
        }
    }*/
}
//////////////////////////////////////////////////////////////////////
// restart a task or restart up returned task
//////////////////////////////////////////////////////////////////////
TaskState task_restart( task_func_t func ) {
    stack_frame_t *p = add_task_to_runlist( func );
    if ( p != NULL ) {
        p->state    = TaskCreated;
        p->sp       = p->stack_top;
        p->r12      = ( uint32_t * )p;
        p->lr       = ( uint32_t * )task_start;
        return p->state;
    }
    return TaskInvalid;
}
//////////////////////////////////////////////////////////////////////
// stop a task
//////////////////////////////////////////////////////////////////////
TaskState task_stop( task_func_t func ) {
    mem_block_t *start = os.mem.allocList( );
    mem_block_t *end = start + 31;
    do {
        if ( start->block != 0 ) {
            stack_frame_t *p = ( stack_frame_t * )start->block;
        }
    } while ( ++start != end );
}
//////////////////////////////////////////////////////////////////////
// restart all tasks
//////////////////////////////////////////////////////////////////////
void task_restart_all( void ) {
    mem_block_t *start = os.mem.allocList( );
    mem_block_t *end = start + 31;
    do {
        if ( start->block != 0 ) {
            stack_frame_t *p = ( stack_frame_t * )start->block;
            add_task_to_runlist( p->ptr );
            p->state = TaskCreated;
            p->sp    = p->stack_top;
            p->r12   = ( uint32_t * )p;
            p->lr    = ( uint32_t * )task_start;
        }
    } while ( ++start != end );
}
//////////////////////////////////////////////////////////////////////
// pause running task
//////////////////////////////////////////////////////////////////////
TaskState task_pause( task_func_t func ) {
    stack_frame_t *p = remove_task_from_runlist( func );
    if ( p == NULL ) return TaskInvalid;
    p->state = TaskPaused;
    return p->state;
}
//////////////////////////////////////////////////////////////////////
// start paused task
//////////////////////////////////////////////////////////////////////
TaskState task_resume( task_func_t func ) {
    stack_frame_t *p = add_task_to_runlist( func );
    if ( p == NULL ) return TaskInvalid;
    p->state = TaskCreated;
    return p->state;
}
//////////////////////////////////////////////////////////////////////
// return task unused memory, only returns active tasks memory
//////////////////////////////////////////////////////////////////////
uint32_t task_memory( task_func_t func ) {
    stack_frame_t *p;
    for ( p = os.root_frame; p; p = p->next ) {
        if ( p->ptr == func ) return p->free_memory;
        if ( p->next == os.root_frame ) return 0;
    }
    return 0;
}
//////////////////////////////////////////////////////////////////////
// remove task from the run list
//////////////////////////////////////////////////////////////////////
stack_frame_t *remove_task_from_runlist2( volatile stack_frame_t *frame ) {
    stack_frame_t *p, *prev;
    prev = os.root_frame;
    for ( p = os.root_frame; p; p = p->next ) {
        if ( p == frame ) {
            prev->next = p->next;
            if ( p->state == TaskDestroyable ) {
                os.mem.free( ( uint32_t * )p );
                os.mem.combine_free_blocks( );
                return NULL;
            }
            return p;
        }
        prev = p;
        if ( p->next == os.root_frame ) break;
    }
    return NULL;
}

stack_frame_t *remove_task_from_runlist( task_func_t func ) {
    stack_frame_t *p, *prev;
    prev = os.root_frame;
    for ( p = os.root_frame; p; p = p->next ) {
        if ( p->ptr == func ) {
            prev->next = p->next;
            if ( p->state == TaskDestroyable ) {
                os.mem.free( ( uint32_t * )p );
                os.mem.combine_free_blocks( );
                return NULL;
            }
            return p;
        }
        prev = p;
        if ( p->next == os.root_frame ) break;
    }
    return NULL;
}
//////////////////////////////////////////////////////////////////////
// remove task from the run list
//////////////////////////////////////////////////////////////////////
stack_frame_t *add_task_to_runlist( task_func_t func ) {
    stack_frame_t *p = NULL, *prev = NULL, *ret = NULL;
    mem_block_t *start = os.mem.allocList( );
    mem_block_t *end = start + 31;
    do {
        if ( start->block != 0 ) {
            p = ( stack_frame_t * )start->block;
            if ( p->ptr == func ) {
                if ( p != os.root_frame ) prev->next = p;
                p->next = os.root_frame;
                prev = p;
                ret = p;
            } else if ( p->state == TaskCreated || p->state == TaskDestroyable ) {
                if ( p != os.root_frame ) prev->next = p;
                p->next = os.root_frame;
                prev = p;
            }
        }
    } while ( ++start != end );
    return ret;
}
