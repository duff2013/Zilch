#include <zilch.h>

Zilch task(1000);// main task
IntervalTimer timer;
/*******************************************************************/
void intervalTimerCallback() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  yield();// yield call from ISR, must uncomment USE_INTERRUPT in task.c
}
/*******************************************************************/
void setup() {
  delay(2000);
  pinMode(LED_BUILTIN ,OUTPUT);
  task.create(task1, 1000, 0);
  task.create(task2, 1000, 0);
  task.create(task3, 1000, 0);
  task.create(task4, 1000, 0);
  task.create(task5, 1000, 0);
  timer.begin(intervalTimerCallback, 50000);// call yield every 50ms
}

// main thread 
void loop() {
  Serial.println("loop");
  delay(1000);
}
/*******************************************************************/

// First task (coroutine)
static void task1(void *arg) {
  while ( 1 ) {
    Serial.println("task1");
    delay(1000);
  }
}
/*******************************************************************/
// 2nd task (coroutine)
static void task2(void *arg) {
  while ( 1 ) {
    Serial.println("task2");
    delay(1000);
  }
}
/*******************************************************************/
// 3rd task (coroutine)
static void task3(void *arg) {
  while ( 1 ) {
    Serial.println("task3");
    delay(1000);
  }
}
/*******************************************************************/
// 4th task (coroutine)
static void task4(void *arg) {
  while ( 1 ) {
    Serial.println("task4");
    delay(1000);
  }
}
/*******************************************************************/
// 5th task (coroutine)
static void task5(void *arg) {
  while ( 1 ) {
    Serial.println("task5");
    delay(1000);
  }
}

