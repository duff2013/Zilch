/*
 *  This example shows how to pause a task. The paused task will
 *  stop wherever in its code. The paused tasks will be taken out
 *  of the context switch so the overall 'os' performance will
 *  increase.
 */
#include <zilch.h>

// zilch os object
Zilch task;
/*******************************************************************/
/*
 *  Stack size is calculated in increments of 32 bits.
 *  So a stack size of 64 equals 256 bytes of space.
 */
#define TASK1_STACK_SIZE 64
#define TASK2_STACK_SIZE 64

void setup() {
    // Add all stack sizes for creating memory pool
    const uint32_t MEM_POOL_SIZE = TASK1_STACK_SIZE + TASK2_STACK_SIZE;
    
    // Allocate memory to the memory pool
    AllocateMemoryPool(MEM_POOL_SIZE);
    
    pinMode(LED_BUILTIN , OUTPUT);
    while (!Serial);
    delay(100);
    Serial.println("Starting tasks now...");
    // create tasks but do not start 'os' yet
    task.create(task1, TASK1_STACK_SIZE, 0);
    task.create(task2, TASK2_STACK_SIZE, 0);
    // start os, all tasks start here in order of 'create' functions
    task.begin();
    // should not get here
}
/*******************************************************************/
//  Not used, if here error with Zilch
void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ERROR");
    delay(25);
}
/*******************************************************************/
// First task
static void task1(void *arg) {
    elapsedMillis task2PauseTimer  = 0;
    elapsedMillis task2ResumeTimer = 0;
    bool Task2PauseTask = false;
    while ( 1 ) {
        
        if (task2PauseTimer >= 5000 && !Task2PauseTask) {
            Serial.println("\nTask 1 is Pausing Task 2");
            // pause task 2 for 5000ms
            task.pause(task2);
            Task2PauseTask = true;
            task2ResumeTimer = 0;
        }
        
        if (task2ResumeTimer >= 5000 && Task2PauseTask) {
            Serial.println("Task 1 is Resuming Task 2\n");
            // resume task2, will start again where it was paused.
            task.resume(task2);
            Task2PauseTask = false;
            task2PauseTimer = 0;
        }
        
        yield();
    }
}
/*******************************************************************/
// 2nd task
static void task2(void *arg) {
    while ( 1 ) {
        Serial.println("Task 2 code section 1");
        delay(500);
        Serial.println("Task 2 code section 2");
        delay(500);
        Serial.println("Task 2 code section 3");
        delay(500);
        Serial.println("Task 2 code section 4");
        delay(500);
    }
}
