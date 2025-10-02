#include <Arduino.h>
/**
 * FreeRTOS Counting Semaphore Challenge
 * 
 * Challenge: use a mutex and counting semaphores to protect the shared buffer 
 * so that each number (0 throguh 4) is printed exactly 3 times to the Serial 
 * monitor (in any order). Do not use queues to do this!
 * 
 * Hint: you will need 2 counting semaphores in addition to the mutex, one for 
 * remembering number of filled slots in the buffer and another for 
 * remembering the number of empty slots in the buffer.
 * 
 * Date: January 24, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS
//#include semphr.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum {BUF_SIZE = 15};                  // Size of buffer array
static const int debounceDelay = 100;

// Globals
static int buf[BUF_SIZE];             // Shared buffer
static int head = 0;                  // Writing index to buffer
static int tail = 0;                  // Reading index to buffer
static SemaphoreHandle_t mtx;     // Waits for parameter to be read
static SemaphoreHandle_t noItemsAvailable;     
static SemaphoreHandle_t noEmptyStaces;     

const int button1 = 15;
const int button2 = 4;
const int button3 = 17; // 17
const int button4 = 19;
const int led1 = 2;
const int led2 = 16; // 16
const int led3 = 5;
const int led4 = 18;
//*****************************************************************************
// Tasks

// Producer: write a given number of times to shared buffer
void producer(void *parameters) {
  int pinButton = *(int*)parameters;
  int var = 0;
  while(1){
    int buttonState = digitalRead(pinButton);
    if(buttonState == 1){
      int lastDebounceTime = millis();
      while((millis() - lastDebounceTime) < debounceDelay){}

      xSemaphoreTake(noEmptyStaces, portMAX_DELAY);
      xSemaphoreTake(mtx, portMAX_DELAY);
      
      buf[head] = var;
      head = (head + 1) % BUF_SIZE;

      xSemaphoreGive(mtx);
      
      Serial.print("Produces: ");
      Serial.println(var);
      Serial.flush();
      var = var + 1;

      xSemaphoreGive(noItemsAvailable);
    }
  }  
}

// Consumer: continuously read from shared buffer
void consumer(void *parameters) {
  int pinLed = *(int*)parameters;
  int val;
  while (1) {
    xSemaphoreTake(noItemsAvailable, portMAX_DELAY);

    xSemaphoreTake(mtx, portMAX_DELAY);
    
    val = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    Serial.println("Consume: ");
    Serial.println(val);
    Serial.flush();

    xSemaphoreGive(mtx);

    xSemaphoreGive(noEmptyStaces);

    digitalWrite(pinLed, HIGH);
    delay(1000);
    digitalWrite(pinLed, LOW);
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  char task_name[12];

  pinMode(button1, INPUT_PULLDOWN);
  pinMode(button2, INPUT_PULLDOWN);
  pinMode(button3, INPUT_PULLDOWN);
  pinMode(button4, INPUT_PULLDOWN);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Semaphore Alternate Solution---");

  // Create mutexes and semaphores before starting tasks
  mtx = xSemaphoreCreateMutex();

  noEmptyStaces = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
  noItemsAvailable = xSemaphoreCreateCounting(BUF_SIZE, 0);

  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button1, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button2, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button3, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button4, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led1, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led2, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led3, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led4, 1, NULL, app_cpu);
}

void loop() {
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}