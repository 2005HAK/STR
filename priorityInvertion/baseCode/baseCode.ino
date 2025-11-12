/**
 * ESP32 Critical Section Demo
 * * Demonstrate how priority inversion can be avoided through the use of
 * critical sections.
 * * Date: February 12, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 */

#include <Arduino.h> // Necessário para Serial.begin e funções Arduino

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
TickType_t cs_wait = 5;   // Time spent in critical section (ms)
TickType_t med_wait = 5000; // Time medium task spends working (ms)

// Globals
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//*****************************************************************************
// Tasks

// Task L (low priority)
void doTaskL(void *parameters) {

  TickType_t timestamp;

  // Do forever
  for(;;){
    // Take lock
    Serial.println("Task L trying to take lock...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    // CORREÇÃO: Usar portENTER_CRITICAL para spinlock MUX
    portENTER_CRITICAL(&spinlock); 

    // Say how long we spend waiting for a lock
    Serial.print("Task L got lock. Spent ");
    Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
    Serial.println(" ms waiting for lock. Doing some work...");

    // Hog the processor for a while doing nothing
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while ( (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait);

    // Release lock
    Serial.println("Task L releasing lock.");
    // CORREÇÃO: Usar portEXIT_CRITICAL
    portEXIT_CRITICAL(&spinlock);

    // Go to sleep
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task M (medium priority)
void doTaskM(void *parameters) {
  TickType_t timestamp;

  // Do forever
  for(;;){

    // Hog the processor for a while doing nothing
    Serial.println("Task M doing some work...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while ( (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < med_wait);

    // Go to sleep
    Serial.println("Task M done!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task H (high priority)
void doTaskH(void *parameters) {
  TickType_t timestamp;

  // Do forever
  for(;;){

    // Take lock
    Serial.println("Task H trying to take lock...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    // CORREÇÃO: Usar portENTER_CRITICAL
    portENTER_CRITICAL(&spinlock); 

    // Say how long we spend waiting for a lock
    Serial.print("Task H got lock. Spent ");
    Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
    Serial.println(" ms waiting for lock. Doing some work...");

    // Hog the processor for a while doing nothing
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while ( (xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait);

    // Release lock
    Serial.println("Task H releasing lock.");
    // CORREÇÃO: Usar portEXIT_CRITICAL
    portEXIT_CRITICAL(&spinlock);
    
    // Go to sleep
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)
void setup() {
  // Configurar Serial (Adicionado <Arduino.h>)
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Critical Section Demo---");
  
  // A ordem de início das tarefas é crítica para forçar a Inversão de Prioridade:
  // 1. Task L (Task L pega o spinlock)
  // 2. Task M (Task M preempita L)
  // 3. Task H (Task H preempita M e tenta pegar o spinlock)

  // Start Task L (low priority = 1)
  xTaskCreatePinnedToCore(doTaskL, "Task L", 4096, NULL, 1, NULL, app_cpu); // Stack aumentada
  
  // Introduce a delay to force Task L to run and take the lock
  vTaskDelay(100 / portTICK_PERIOD_MS); // Atraso maior para L ter tempo de pegar o lock

  // Start Task M (medium priority = 2)
  xTaskCreatePinnedToCore(doTaskM, "Task M", 4096, NULL, 2, NULL, app_cpu);

  // Start Task H (high priority = 3)
  xTaskCreatePinnedToCore(doTaskH, "Task H", 4096, NULL, 3, NULL, app_cpu);

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}