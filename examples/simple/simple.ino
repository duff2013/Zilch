/*
 *  Basic usage example.
 */
#include <zilch.h>

Zilch task;
/*******************************************************************/
/*
 *  Stack size is calculated in increments of 32 bits.
 *  So a stack size of 128 equals 512 bytes of space.
 */
#define TASK1_STACK_SIZE 128
#define TASK2_STACK_SIZE 128
#define TASK3_STACK_SIZE 128
#define TASK4_STACK_SIZE 128
#define TASK5_STACK_SIZE 128

void setup() {
    // Add all stack sizes for creating memory pool
    const uint32_t MEM_POOL_SIZE =  TASK1_STACK_SIZE +
                                    TASK2_STACK_SIZE +
                                    TASK3_STACK_SIZE +
                                    TASK4_STACK_SIZE +
                                    TASK5_STACK_SIZE;
    
    // Allocate memory to the memory pool
    AllocateMemoryPool(MEM_POOL_SIZE);
    
    pinMode(LED_BUILTIN , OUTPUT);
    while (!Serial);
    delay(100);
    Serial.println("Starting tasks now...");
    /*
     Configures the task and its memory.
     create(function, Stack Size, Argument);
     */
    task.create(task1, TASK1_STACK_SIZE, 0);
    task.create(task2, TASK2_STACK_SIZE, 0);
    task.create(task3, TASK3_STACK_SIZE, 0);
    task.create(task4, TASK4_STACK_SIZE, 0);
    task.create(task5, TASK5_STACK_SIZE, 0);
    // This starts everything
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
// First task
static void task1(void *arg) {
    while ( 1 ) {
        Serial.println("task1");
        delay(1000);
    }
}
/*******************************************************************/
// 2nd task
static void task2(void *arg) {
    while ( 1 ) {
        Serial.println("task2");
        delay(1000);
    }
}
/*******************************************************************/
// 3rd task
static void task3(void *arg) {
    while ( 1 ) {
        Serial.println("task3");
        delay(1000);
    }
}
/*******************************************************************/
// 4th task
static void task4(void *arg) {
    while ( 1 ) {
        Serial.println("task4");
        delay(1000);
    }
}
/*******************************************************************/
// 5th task
static void task5(void *arg) {
    while ( 1 ) {
        Serial.println("task5");
        delay(1000);
    }
}
