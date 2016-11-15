/*
 ||
 || @file 		zilch.cpp
 || @version 	0.5
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
#include "utility/task.h"
/***************************************************
*----------------Editible Options------------------*
****************************************************/
#if defined(KINETISK)
    #define MAX_TASKS 32
#elif defined(KINETISL)
    #define MAX_TASKS 8
#endif
// minimum main stack size.
#define MIN_MAIN_STACK_SIZE 200
// Memory fill pattern used to estimate stack usage.
#define MEMORY_FILL_PATTERN 0XFFFFFFFFUL
// Size of the inter task message buffer
#define MSG_BUFFER_SIZE 20
/****************************************************
*----------------End Editable Options---------------*
*****************************************************/
//TODO: use the PSP stack pointer for tasks returns and MSP for ISR returns.
#define MAIN_RETURN         0xFFFFFFF9  //Tells the handler to return using the MSP
#define THREAD_RETURN       0xFFFFFFFD //Tells the handler to return using the PSP

#ifdef __cplusplus
extern "C" {
#endif
    void init_stack( uint32_t main_stack, uint32_t pattern_override );
#if defined(KINETISK)
    TaskState task_create( task_func_t func, size_t stack_size, void *arg );
#elif defined(KINETISL)
    TaskState task_create( volatile stack_frame_t *frame, size_t stack_size, task_func_t func, void *arg );
#endif
    TaskState  task_state   ( task_func_t func );
    TaskState  main_state   ( loop_func_t func );
    TaskState  task_sync    ( task_func_t func );
    TaskState  task_restart ( task_func_t func );
    TaskState  task_pause   ( task_func_t func );
    TaskState  task_resume  ( task_func_t func );
    uint32_t   task_memory  ( task_func_t func );
    uint32_t   main_memory  ( loop_func_t func );
    //inline uint32_t sys_acquire_lock( volatile unsigned int *lock_var );
    //inline uint32_t sys_release_lock( volatile unsigned int *lock_var );
    //void TaskUseInterrupt(enum IRQ_NUMBER_t interruptName);
#ifdef __cplusplus
}
#endif

// end of bss section
extern unsigned long _ebss;
// end of heap/stack area
extern unsigned long _estack;
// Address of the stack root for last task (else 0)
static volatile uint32_t stackroot = 0;
// Alternate fill pattern, NOT USED!!!
static uint32_t psp_fill_pattern = 0;
// Mask for task swap
static volatile uint32_t task_mask    = 0;
// Restore task_mask when fifo is empty
static volatile uint32_t init_mask     = 0;
// Used for low level ISR that gets called for handler functions.
static volatile boolean update_in_progress  = false;
// Hold task struct for context switch
static volatile stack_frame_t process_tasks[MAX_TASKS];
volatile uint8_t num_task = 0;
// Enum for clarity on what function was called and then passed to LOW Level ISR
// for completion.
typedef enum  {
    PAUSE,
    RESUME,
    RESTART,
    TASK_STATE,
    MAIN_STATE,
    MEMORY
} calling_func_t;
calling_func_t CALLING_FUNCTION;
task_func_t FUNCTION;
loop_func_t LOOP_FUNCTION;
TaskState _STATE;
uint32_t STACK_MEMORY;
task_msg_t msg[MSG_BUFFER_SIZE];
uint32_t msgHead;
uint32_t msgTail;

Zilch::Zilch( uint16_t main_stack_size, const uint32_t pattern ) {
    NVIC_SET_PRIORITY( IRQ_RTC_SECOND, 0xFF ); // 0xFF = lowest priority
    NVIC_ENABLE_IRQ( IRQ_RTC_SECOND );
    init_stack( main_stack_size, pattern );
    for (int i = 0; i <= MSG_BUFFER_SIZE; i++) {
        msg[i].func_to_ptr = nullptr;
        msg[i].func_from_ptr = nullptr;
    }
}

TaskState Zilch::create( task_func_t task, size_t stack_size, void *arg ) {
    // Round stack size to a word multiple
    int s_size = ( stack_size + sizeof ( uint32_t ) - 1 ) / sizeof ( uint32_t ) * sizeof ( uint32_t );
    
    Serial.print("Num: ");
    Serial.print(num_task);
    Serial.print(" | S Size: ");
    Serial.print(s_size);
    Serial.print(" | Stack Size: ");
    Serial.println(stack_size);
    
    if ( num_task+1 >= MAX_TASKS ) return TaskInvalid;
    ++num_task;
#if defined(KINETISK)
    TaskState p = task_create( task, s_size, arg );
#elif defined(KINETISL)
    volatile stack_frame_t *frame = &process_tasks[num_task];
    TaskState p = task_create( frame, s_size, task, arg );
#endif
    return p;
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

uint32_t Zilch::memory( task_func_t task ) {
    return task_memory( task );
}

uint32_t Zilch::memory( loop_func_t task ) {
    uint32_t tmp = 0;// = main_size(task);
    return tmp;
}

bool Zilch::transmit( void *p, task_func_t task_to, task_func_t task_from, int count ) {
    for (int i = 0; i < MSG_BUFFER_SIZE; i++) {
        if (msg[i].func_to_ptr == nullptr && msg[i].func_from_ptr == nullptr) {
            msg[i].func_to_ptr = task_to;
            msg[i].func_from_ptr = task_from;
            byte *tmp = ( byte * )p;
            memcpy( msg[i].type_b, tmp, count );
            return false;
        }
    }
    return true;
}

bool Zilch::receive( task_func_t task_to, task_func_t task_from, void *p ) {
    for (int i = 0; i < MSG_BUFFER_SIZE; i++) {
        if (msg[i].func_to_ptr == task_to && msg[i].func_from_ptr == task_from) {
            byte *tmp = ( byte * )p;
            memcpy( tmp, msg[i].type_b, 4 );
            msg[i].func_to_ptr = nullptr;
            msg[i].func_from_ptr = nullptr;
            return false;
        }
    }
    return true;
}
/******************************************************************************************
  ________          ________          ________          ________          ________
_|        |________|        |________|        |________|        |________|        |________
*******************************************************************************************/
//////////////////////////////////////////////////////////////////////
// Initialize main stack
//////////////////////////////////////////////////////////////////////
void init_stack( uint32_t main_stack, uint32_t pattern_override ) {
    num_task = 0;
    psp_fill_pattern = pattern_override;
    
    int i = 0;
    for (i = 0; i < MAX_TASKS; i++) {
        volatile stack_frame_t *u = &process_tasks[i];
        u->address = 0;
        u->state = TaskInvalid;
    }
    int stack_size = main_stack;
    stack_size = ( stack_size + sizeof (uint32_t) ) / sizeof (uint32_t) * sizeof (uint32_t);
    asm volatile( "MRS %[result], MSP\n" : [result] "=r" (stackroot) );
    volatile stack_frame_t *p = &process_tasks[num_task];
    
    if ( stack_size >= MIN_MAIN_STACK_SIZE ) {
        p->initial_sp = stackroot;
        p->stack_size = stack_size;
        stackroot -= stack_size;
    }
    else {
        p->stack_size = MIN_MAIN_STACK_SIZE;
        p->initial_sp = stackroot;
        stackroot -= MIN_MAIN_STACK_SIZE;
    }
    p->address = 1;
    p->state = TaskCreated;
    task_mask = ( 1 << 0 );
}
//////////////////////////////////////////////////////////////////////
// This routine is the task's launch pad routine
//////////////////////////////////////////////////////////////////////
static void task_start( ) {
    stack_frame_t* p;
#if defined(KINETISK)
    asm volatile( "mov %[result], r12\n" : [result] "=r" ( p ) );		 // r12 points to task initially
#elif defined(KINETISL)
    asm volatile("mov %[result], r7\n" : [result] "=r" ( p ) );     // r7 points to task initially
#endif
    asm volatile( "mov r0, %[value]\n" : : [value] "r" ( p->arg ) );	 // Supply void *arg to task call
    asm volatile( "mov r1, %[value]\n" : : [value] "r" ( p->func_ptr ) );// r1 now holds the function ptr to call
#if defined(KINETISK)
    asm volatile( "push {r2-r12}" ); // push to stack
    asm volatile( "blx  r1\n" );     // func(arg) call
    asm volatile( "pop  {r2-r12}" ); // pop from stack if task is returned
#elif defined(KINETISL)
    asm volatile("push {r4-r7}\n");  // push r4-r7 to stack
    //asm volatile("push {r3}\n");     // push r3 to stack
    //asm volatile("mov r4,r8\n");     // mov r8 to r4
    //asm volatile("mov r5,r9\n");     // mov r9 to r5
    //asm volatile("push {r4,r5}\n");	 // Push r8,r9
    
    asm volatile("blx  r1\n");       // func(arg) call
    
    //asm volatile("pop {r4,r5}\n");   // pop r,8,r9
    //asm volatile("mov r8,r4\n");     // r4 to r8
    //asm volatile("mov r9,r5\n");     // r5 to r9
    //asm volatile("pop {r3}\n");      // pop r3 from stack
    asm volatile("pop  {r4-r7}\n");  // pop r4-r7 from stack
#endif
    p->state = TaskReturned;		 // update state when task has returned
    
    for (;;) yield( );               // returned task now loop here till restarted
}
//////////////////////////////////////////////////////////////////////
// Set up a task to execute (but don't launch it)
//////////////////////////////////////////////////////////////////////
#if defined(KINETISL)
TaskState task_create( volatile stack_frame_t *frame, size_t stack_size, task_func_t func, void *arg ) {
    asm volatile("push {r0,r1,r2,r3}\n");
    asm volatile("stmia r0!,{r4,r5,r6,r7}\n");       // Save lower regs
    asm volatile("mov r1,r8\n");
    asm volatile("mov r2,r9\n");
    asm volatile("mov r3,sl\n");
    asm volatile("stmia r0!,{r1,r2,r3}\n");		// Save r8,r9 & sl
    asm volatile("mov r1,fp\n");
    asm volatile("mov r3,lr\n");
    asm volatile("stmia r0!,{r1,r2,r3}\n");		// Save fp,(placeholder for sp) & lr
    asm volatile("pop {r0,r1,r2,r3}\n");		// Restore regs
    frame->stack_size = stack_size;		// In r1
    frame->func_ptr 	= func;			// In r2
    frame->arg 	= arg;			// In r3
    frame->r7	= (uint32_t)frame;	// Overwrite r12 with fiber ptr
    frame->sp = stackroot;			// Save as Fiber's sp
    frame->lr = (void *) task_start;	// Fiber startup code
    frame->state = TaskCreated;		// Set state of this fiber
    frame->initial_sp = stackroot;		// Save sp for restart()
    stackroot    -= stack_size;             // This is the new root of the stack
    int address   = 1 << num_task;          // get task address
    frame->address   |= address;                // set task address
    task_mask     = task_mask | ( 1 << num_task ); // task swap mask
    init_mask     = task_mask;              // num of tasks
    return TaskCreated;
}
#elif defined(KINETISK)
TaskState task_create( task_func_t func, size_t stack_size, void *arg ) {
    volatile stack_frame_t *p = &process_tasks[num_task]; // Task struct
    asm volatile( "STMEA %0,{r1-r11}\n" : "+r" ( p ) :: "memory" );// Save r1-r11 to task struct
    p->stack_size = stack_size;     // Save task size
    p->func_ptr   = func;           // Save task function
    p->arg        = arg;            // Save task arg
    p->r12        = ( uint32_t )p;  // r12 points to struct
    p->sp         = stackroot;              // Save as tasks's sp
    p->state      = TaskCreated;		    // Set state of this task
    p->initial_sp = stackroot;		        // Save sp for restart()
    p->lr         = ( void * ) task_start;	// Task startup code
    stackroot    -= stack_size;             // This is the new root of the stack
    int address   = 1 << num_task;          // get task address
    p->address   |= address;                // set task address
    task_mask     = task_mask | ( 1 << num_task ); // task swap mask
    init_mask     = task_mask;              // num of tasks
    return TaskCreated;
}
#endif
//////////////////////////////////////////////////////////////////////
// Swap one task for another. This should be optimized as much as possible
//////////////////////////////////////////////////////////////////////
void task_swap( volatile stack_frame_t *prevframe, volatile stack_frame_t *nextframe ) {
#if defined(KINETISK)
    asm volatile (
                  "MRS %[result], MSP\n"
                  : [result] "=r" ( prevframe->sp )
                  );
    asm volatile (
                  "ADD r0, #4"             "\n\t"   // &prevframe->r2
                  "STMEA r0!,{r2-r12,lr}"  "\n\t"   // Save r2-r12 + lr
                  "LDMIA r1,{r1-r12,lr}"   "\n\t"   // Restore r1(sp) and r2-r12, lr
                  "MSR MSP, r1"            "\n\t"   // Set new sp
                  "BX lr"                  "\n"
                  );
#elif defined(KINETISL)
    asm volatile("stmia r0!,{r4-r7}\n");	// Save r4,r5,r6,r7
    asm volatile("mov r2,r8\n");            // mov r8 to r2
    asm volatile("mov r3,r9\n");            // mov r9 to r3
    asm volatile("mov r4,sl\n");            // mov sl to r4
    asm volatile("stmia r0!,{r2-r4}\n");	// Save r8,r9,sl
    asm volatile("mov r2,fp\n");            // mov fp to r2
    asm volatile("mov r3,sp\n");            // mov sp to r3
    asm volatile("mov r4,lr\n");            // mov lr to r4
    asm volatile("stmia r0!,{r2-r4}\n");	// Save fp,sp,lr
    
    asm volatile("add r1,#16");             // r0 = &nextfibe->r8
    asm volatile("ldmia r1!,{r2-r4}\n");	// Load values for r8,r9,sl
    asm volatile("mov r8,r2\n");            // mov r2 to r8
    asm volatile("mov r9,r3\n");            // mov r3 to r9
    asm volatile("mov sl,r4\n");            // mov r4 to sl
    asm volatile("ldmia r1!,{r2-r4}\n");	// Load values for fp,sp,lr
    asm volatile("mov fp,r2\n");            // mov fp to r2
    asm volatile("mov sp,r3\n");            // mov sp to r3
    asm volatile("mov lr,r4\n");            // mov lr to r4
    asm volatile("sub r1,#40\n");           // r0 = nextfibe
    asm volatile("ldmia r1!,{r4-r7}\n");	// Restore r4-r7
#endif
}
//////////////////////////////////////////////////////////////////////
// all invocations of yield in teensyduino api go through this now.
//////////////////////////////////////////////////////////////////////
void yield( void ) {
    //digitalWriteFast(16, HIGH);
    // There is only the main context running
    if ( num_task <= 0 ) return;
#ifdef USE_INTERRUPTS
    __disable_irq( );
#endif
    uint32_t mask = task_mask;
    uint32_t tail = __builtin_ctz( mask );
    mask &= ( mask - 1 );
    uint32_t head = __builtin_ctz( mask );
    if(head > num_task) head = 0;
    // previous task to be stored
    volatile stack_frame_t *last = &process_tasks[tail];
    // next task to be loaded
    volatile stack_frame_t *next = &process_tasks[head];
    enum TaskState  lst = last->state;
    enum TaskState  nxt = next->state;
    // set previous state to created or returned
    enum TaskState l = ( lst != TaskReturned ) ? TaskCreated : TaskReturned;
    last->state = ( lst == TaskPause ) ? TaskPause : l;
    // set current state to executing or returned
    enum TaskState n = ( nxt != TaskReturned ) ? TaskExecuting : TaskReturned;
    next->state = ( nxt == TaskPause ) ? TaskPause : n;
    // update next task
    const uint32_t init = init_mask;
    task_mask = mask == 0 ? init : mask;
    // make the swap
    task_swap( last, next );
#ifdef USE_INTERRUPTS
    __enable_irq( );
#endif
    //digitalWriteFast(16, LOW);
}
//////////////////////////////////////////////////////////////////////
// pass task state, pass loop state
// ** calls low priority software isr to update **
//////////////////////////////////////////////////////////////////////
TaskState task_state( task_func_t func ) {
    FUNCTION = func;
    CALLING_FUNCTION = TASK_STATE;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return _STATE;
}
TaskState main_state( loop_func_t func ) {
    LOOP_FUNCTION = func;
    CALLING_FUNCTION = MAIN_STATE;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return _STATE;
}
//////////////////////////////////////////////////////////////////////
// routine to block until selected task return's.
//////////////////////////////////////////////////////////////////////
TaskState task_sync( task_func_t func ) {
    enum TaskState return_state;
    do {
        yield( );
        return_state = task_state( func );
    }
    while ( ( return_state == TaskCreated ) || ( return_state == TaskPause )  ) ; // Keep waiting
    return return_state;
}
//////////////////////////////////////////////////////////////////////
// start up returned task
// ** calls low priority software isr to update **
//////////////////////////////////////////////////////////////////////
TaskState task_restart( task_func_t func ) {
    FUNCTION = func;
    CALLING_FUNCTION = RESTART;
    update_in_progress = true;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return _STATE;
}
//////////////////////////////////////////////////////////////////////
// pause running task
// ** calls low priority software isr to update **
//////////////////////////////////////////////////////////////////////
TaskState task_pause( task_func_t func ) {
    FUNCTION = func;
    CALLING_FUNCTION = PAUSE;
    update_in_progress = true;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return _STATE;
}
//////////////////////////////////////////////////////////////////////
// start paused task
// ** calls low priority software isr to update **
//////////////////////////////////////////////////////////////////////
TaskState task_resume( task_func_t func ) {
    FUNCTION = func;
    CALLING_FUNCTION = RESUME;
    update_in_progress = true;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return _STATE;
}
//////////////////////////////////////////////////////////////////////
// TODO: return task unused size, not working yet
//////////////////////////////////////////////////////////////////////
uint32_t task_memory( task_func_t func ) {
    FUNCTION = func;
    CALLING_FUNCTION = MEMORY;
    update_in_progress = true;
    NVIC_SET_PENDING( IRQ_RTC_SECOND );
    while( update_in_progress ) ;
    return STACK_MEMORY;
}

uint32_t main_memory( loop_func_t func ) {
    volatile stack_frame_t *p = &process_tasks[0];
    return p->initial_sp;
}
//////////////////////////////////////////////////////////////////////
// low priority isr to update or change the state of each task
//////////////////////////////////////////////////////////////////////
void rtc_seconds_isr(void) {
    task_func_t f_ptr;
    TaskState state;
    uint32_t num;
    int i = 0;
    volatile stack_frame_t *p;
    do {
        p = &process_tasks[i];
        f_ptr = p->func_ptr;
        if ( i > num_task ) {
            _STATE = TaskInvalid;
            update_in_progress = false;
            return;
        }
        i++;
    } while ( FUNCTION != f_ptr  );
    
    /* call function handlers */
    switch ( CALLING_FUNCTION ) {
        case PAUSE:
            p = &process_tasks[i-1];
            state = p->state;
            if ( state == TaskPause ) {
                _STATE = TaskPause;
                update_in_progress = false;
                break;
            }
            p->state = TaskPause;
            num = init_mask;
            num &= ~( p->address );
            init_mask = num;
            _STATE = TaskPause;
            update_in_progress = false;
            break;
        case RESUME:
            p = &process_tasks[i-1];
            state = p->state;
            _STATE = state;
            if ( state != TaskPause ) {
                update_in_progress = false;
                break;
            }
            p->state = TaskCreated;
            num = init_mask;
            num |= ( p->address );
            init_mask = num;
            _STATE = TaskCreated;
            update_in_progress = false;
            break;
        case RESTART:
            p = &process_tasks[i-1];
            state = p->state;
            if ( state != TaskReturned ) {
                _STATE = TaskInvalid;
                update_in_progress = false;
                break;
            }
            p->state = TaskCreated;
#if defined(KINETISK)
            p->r12 = ( uint32_t )p;
#elif defined(KINETISL)
            p->r7 = ( uint32_t )p;
#endif
            p->func_ptr = FUNCTION;
            p->lr = ( void * )task_start;
            p->sp = p->initial_sp;
            _STATE = TaskCreated;
            update_in_progress = false;
            break;
        case TASK_STATE:
            p = &process_tasks[i-1];
            _STATE = p->state;
            update_in_progress = false;
            break;
        case MAIN_STATE:
            p = &process_tasks[0];
            _STATE = p->state;
            update_in_progress = false;
            break;
        case MEMORY:
        {
            p = &process_tasks[i-1];
            volatile uint32_t *start = (volatile uint32_t *)p->initial_sp;
            volatile const uint32_t *stop = start - (p->stack_size/4);
            int count = 0;
            while ( start >= stop ) {
                //__disable_irq();
                if(*start == 0xFFFFFFFF) count++;
                start--;
                //__enable_irq();
            }
            STACK_MEMORY = (count-1)*4;
            update_in_progress = false;
        }
            break;
            
        default:
            update_in_progress = false;
            break;
    }
}