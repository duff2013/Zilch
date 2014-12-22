/****************************************
 * This simple example show the functionality
 * of the spin lock on shared resources, 
 * i.e. printing to serial port.
 *
 * This example shows the loop (main) 
 * function will block all the other faster 
 * running tasks from running at 100ms 
 * instead it they will all run at 1 second.
 * Comment out the TASK_LOCK and you will
 * see that other tasks will run as normal
 * i.e. 100ms.
 ****************************************/
#include <zilch.h>
// main task
Zilch task(1000);
// var to be used with TASK_LOCK
volatile unsigned int PRINT_LOCK = 0;

void setup() {
  delay(2000);
  pinMode(LED_BUILTIN ,OUTPUT);
  task.create(task1, 1000, 0);
  task.create(task2, 1000, 0);
  task.create(task3, 1000, 0);
  task.create(task4, 1000, 0);
  task.create(task5, 1000, 0);
}

// main thread 
void loop() {
  /****************************************
   * Spin Lock, will block all other tasks
   * that use this until it is released.
   * Since the delay is in the spin lock
   * all other tasks in this example will 
   * be blocked from running until it 
   * released.
   ***************************************/
  TASK_LOCK(PRINT_LOCK) {
    Serial.println("loop");
    delay(1000);
  }
}
/*******************************************************************/

// First task (coroutine)
static void task1(void *arg) {
  while ( 1 ) {
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task1");
    }
    delay(100);
  }
}
/*******************************************************************/
// 2nd task (coroutine)
static void task2(void *arg) {
  while ( 1 ) {
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task2");
    }
    delay(100);
  }
}
/*******************************************************************/
// 3rd task (coroutine)
static void task3(void *arg) {
  while ( 1 ) {
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task3");
    }
    delay(100);
  }
}
/*******************************************************************/
// 4th task (coroutine)
static void task4(void *arg) {
  while ( 1 ) {
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task4");
    }
    delay(100);
  }
}
/*******************************************************************/
// 5th task (coroutine)
static void task5(void *arg) {
  while ( 1 ) {
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task5");
    }
    delay(100);
  }
}
