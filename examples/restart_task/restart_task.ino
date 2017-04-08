/*
 *  This example shows how to handle returned tasks
 */
#include <zilch.h>

// Zilch object
Zilch task;
/*******************************************************************/
/*
 *  Stack size is calculated in increments of 32 bits.
 *  So a stack size of 64 equals 256 bytes of space.
 */
#define WORKER_STACK_SIZE   128
#define TASK1_STACK_SIZE    128
#define TASK2_STACK_SIZE    128
#define TASK3_STACK_SIZE    128
#define TASK4_STACK_SIZE    128

void setup() {
    // Add all stack sizes for creating memory pool
    const uint32_t MEM_POOL_SIZE =  WORKER_STACK_SIZE +
                                    TASK1_STACK_SIZE  +
                                    TASK2_STACK_SIZE  +
                                    TASK3_STACK_SIZE  +
                                    TASK4_STACK_SIZE;
    
    // Allocate memory to the memory pool
    AllocateMemoryPool(MEM_POOL_SIZE);
    
    pinMode(LED_BUILTIN , OUTPUT);
    while (!Serial);
    delay(100);
    Serial.println("Starting tasks now...");
    task.create(worker, WORKER_STACK_SIZE, 0);
    task.create(task1, TASK1_STACK_SIZE, 0);
    task.create(task2, TASK2_STACK_SIZE, 0);
    task.create(task3, TASK3_STACK_SIZE, 0);
    task.create(task4, TASK4_STACK_SIZE, 0);
    task.begin();
    // should not get here
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ERROR");
    delay(25);
    
}
/*******************************************************************/
//  Not used, if here error with Zilch
void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ERROR");
    delay(25);
}
/*******************************************************************/
// kernal task
static void worker(void *arg) {
    // hold state of each tasks
    TaskState task1_state;
    TaskState task2_state;
    TaskState task3_state;
    TaskState task4_state;
    while ( 1 ) {
        
        // get state of tasks
        task1_state = task.state(task1);
        task2_state = task.state(task2);
        task3_state = task.state(task3);
        task4_state = task.state(task4);
        
        // when all tasks have returned, restart them all
        if (task1_state == TaskReturned && task2_state == TaskReturned && task3_state == TaskReturned && task4_state == TaskReturned ) {
            delay(500);
            Serial.print("Worker task is restarting all returned tasks in: ");
            delay(1000);
            Serial.print("3 - ");
            delay(1000);
            Serial.print("2 - ");
            delay(1000);
            Serial.println("1");
            delay(1000);
            //task.restartAll();// restart all tasks
            task.restart(task1);// restart task1
            task.restart(task2);// restart task2
            task.restart(task3);// restart task3
            task.restart(task4);// restart task4
        }
        yield();
    }
}
/*******************************************************************/
// 1st task
static void task1(void *arg) {
    int i = 5;
    while ( i-- >= 0 ) {
        Serial.println("Task 1");
        delay(500);
    }
    Serial.println("\nTask 1 is now Returning\n");
}
/*******************************************************************/
// 2nd task
static void task2(void *arg) {
    int i = 10;
    while ( i-- >= 0 ) {
        Serial.println("Task 2");
        delay(500);
    }
    Serial.println("\nTask 2 is now Returning\n");
}
/*******************************************************************/
// 3rd task
static void task3(void *arg) {
    int i = 15;
    while ( i-- >= 0 ) {
        Serial.println("Task 3");
        delay(500);
    }
    Serial.println("\nTask 3 is now Returning\n");
}
/*******************************************************************/
// 4th task
static void task4(void *arg) {
    int i = 20;
    while ( i-- >= 0 ) {
        Serial.println("Task 4\n");
        delay(500);
    }
    Serial.println("\nTask 4 is now Returning\n");
}
