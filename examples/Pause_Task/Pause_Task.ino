/*******************************
 * This shows how to pause and
 * resume a task from another
 * task.
 ******************************/
#include <zilch.h>

Zilch task(1000);// main task
/*******************************************************************/
void setup() {
  delay(2000);
  pinMode(LED_BUILTIN , OUTPUT);
  task.create(task1, 200, 0);
}

// main thread
void loop() {
  Serial.println("Pause task1");
  task.pause( task1 );
  delay(500);
  Serial.println("Resume task1");
  delay(500);
  task.resume( task1 );
  delay(500);
}
/*******************************************************************/
// First task (coroutine)
static void task1(void *arg) {
  while ( 1 ) {
    Serial.println("Running task1");
    delay(10);
  }
}

