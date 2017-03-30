/*
 *  Shows how to get free memory for each stack.
 *  Use the Serial Plotter to visualize the memory
 *  usage.
 */
#include <zilch.h>

Zilch task;

/*
 *  Stack size is calculated in increments of 32 bits.
 *  So a stack size of 128 equals 512 bytes of space.
 */
#define WORKER_STACK_SIZE 128
#define TASK1_STACK_SIZE  128
#define TASK2_STACK_SIZE  128
/*******************************************************************/
void setup() {
    // Add all stack sizes for creating memory pool
    const uint32_t MEM_POOL_SIZE =  TASK1_STACK_SIZE +
                                    TASK2_STACK_SIZE +
                                    WORKER_STACK_SIZE;
    
    // Allocate memory to the memory pool
    AllocateMemoryPool(MEM_POOL_SIZE);
    pinMode(LED_BUILTIN , OUTPUT);
    while (!Serial);
    delay(100);
    Serial.println("Starting tasks now...");
    
    // create stacks
    task.create(worker, WORKER_STACK_SIZE, 0);
    task.create(task1, TASK1_STACK_SIZE, 0);
    task.create(task2, TASK2_STACK_SIZE, 0);
    task.begin();
    // should not get here
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ERROR");
    delay(25);
    
}
/*******************************************************************/
// main task
void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ERROR");
    delay(25);
}
/*******************************************************************/
// First task
static void worker(void *arg) {
    while ( 1 ) {
        // worker task free memory
        int worker_mem_free = task.freeMemory(worker);
        // task1 free memory
        int task1_mem_free = task.freeMemory(task1);
        // task2 free memory
        int task2_mem_free = task.freeMemory(task2);
        
        /*
         Use Serial Plotter to visualize each tasks
         realtime memory usage. Task1 and Task2 each
         create an array in their respective stacks
         that increrase or decrease in size.
         */
        Serial.print(worker_mem_free);
        Serial.print(",");
        Serial.print(task1_mem_free);
        Serial.print(",");
        Serial.println(task2_mem_free);
        delay(20);
    }
}
/*******************************************************************/
// First task
static void task1(void *arg) {
    int array_size = 1;
    while ( 1 ) {
        // array size increases each loop
        uint32_t array[array_size];
        memset(array, 'A', array_size * 4);
        if (array_size++ >= 32) array_size = 1;
        delay(20);
    }
}
/*******************************************************************/
// 2nd task
static void task2(void *arg) {
    int array_size = 32;
    while ( 1 ) {
        // array size decreases each loop.
        uint32_t array[array_size];
        memset(array, 'A', array_size * 4);
        if (array_size-- <= 1) array_size = 32;
        delay(20);
    }
}
