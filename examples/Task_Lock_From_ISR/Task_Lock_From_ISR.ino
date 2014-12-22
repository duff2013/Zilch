/****************************************
 * This simple example show the functionality
 * of the spin lock on shared resources, 
 * i.e. printing to serial port with
 * interrupts.
 *
 * This example shows the the Interval
 * Timer has a shared resource i.e. Serial
 * printing that will be masked until Lock
 * is free. The Timer should fire every 50ms
 * but is masked and runs every second.
 ****************************************/
#include <zilch.h>
// main task
Zilch task(1000);
// Timer with lock on print resource
IntervalTimer timer;
// var to be used with TASK_LOCK
volatile unsigned int PRINT_LOCK = 0;
/*******************************************************************/
void intervalTimerCallback() {
  /**********************************
  * will be masked if it does not own
  * the lock.
  ***********************************/
  TASK_LOCK(PRINT_LOCK) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("ISR");
  }
  yield();
}
/*******************************************************************/
void setup() {
  delay(2000);
  pinMode(LED_BUILTIN ,OUTPUT);
  // Must declare what ISR's are used in Locking
  TaskUseInterrupt(IRQ_PIT_CH0);
  task.create(task1, 5000, 0);
  // call yield every 50ms
  timer.begin(intervalTimerCallback, 50000);
}

// main thread 
void loop() {
  TASK_LOCK(PRINT_LOCK) {
    Serial.println("loop");
  }
  delay(100);
}
/*******************************************************************/
// First task (coroutine)
static void task1(void *arg) {
  while ( 1 ) {
  /****************************************
   * Spin Lock, will block all other tasks
   * that use this until it is released.
   * Since the delay is in the spin lock
   * all other tasks in this example will 
   * be blocked from running until it 
   * released.
   ***************************************/
    TASK_LOCK(PRINT_LOCK) {
      Serial.println("task1\n");
      delay(1000);
    }
  }
}
