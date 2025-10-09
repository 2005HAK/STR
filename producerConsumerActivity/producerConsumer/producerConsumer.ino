#include <Arduino.h>
// You'll likely need this on vanilla FreeRTOS
//#include semphr.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum {BUF_SIZE = 15};                                 // Size of buffer array
static const int debounceDelay = 100;                 // Button debounce delay in ms

// Globals
static int buf[BUF_SIZE];                             // Shared buffer
static int head = 0;                                  // Writing index to buffer
static int tail = 0;                                  // Reading index to buffer
static int var = 0;                                   // Variable to write to buffer
static SemaphoreHandle_t mtx;                         // Waits for parameter to be read
static SemaphoreHandle_t noItemsAvailable;            // Waits for items to be available
static SemaphoreHandle_t noEmptySpaces;               // Waits for empty spaces in buffer

// Pin definitions producers
const int button1 = 15;
const int button2 = 4;
const int button3 = 17;
const int button4 = 19;

// Pin definitions consumers
const int led1 = 2;
const int led2 = 16;
const int led3 = 5;
const int led4 = 18;

//*****************************************************************************
// Tasks

// Producer: write a given number of times to shared buffer
void producer(void *parameters) {
  int pinButton = *(int*)parameters;

  while(1){
    // Read button state
    int buttonState = digitalRead(pinButton);
    // Verify button press with debounce
    if(buttonState == 1){
      // Save the current time to implement debounce
      int lastDebounceTime = millis();
      // Wait until debounce time has passed
      while((millis() - lastDebounceTime) < debounceDelay){}

      // Wait until has empty spaces in buffer
      xSemaphoreTake(noEmptySpaces, portMAX_DELAY);
      // Wait until mutex is available
      xSemaphoreTake(mtx, portMAX_DELAY);
      
      // Critical section
      // Write to buffer
      buf[head] = var;
      // Update head index
      head = (head + 1) % BUF_SIZE;
      // End critical section

      // Print to Serial
      Serial.print("Produces: ");
      Serial.println(var);
      Serial.flush();
      var = var + 1;

      // Release mutex
      xSemaphoreGive(mtx);
      
      // Release that there is a new item available
      xSemaphoreGive(noItemsAvailable);
    }
  }  
}

// Consumer: continuously read from shared buffer
void consumer(void *parameters) {
  int pinLed = *(int*)parameters;
  int val;

  while (1) {
    // Wait until has items available in buffer
    xSemaphoreTake(noItemsAvailable, portMAX_DELAY);
    // Wait until mutex is available
    xSemaphoreTake(mtx, portMAX_DELAY);
    
    // Critical section
    // Read from buffer
    val = buf[tail];
    // Update tail index
    tail = (tail + 1) % BUF_SIZE;
    // End critical section
    // Print to Serial
    Serial.println("Consumes: ");
    Serial.println(val);
    Serial.flush();

    // Release mutex
    xSemaphoreGive(mtx);

    // Release that there is an empty space available
    xSemaphoreGive(noEmptySpaces);

    // Blink LED to show activity
    digitalWrite(pinLed, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));                                              // I should use vTaskDelay, but I don't remember and now I don't have time to check (It's time to submit!)
    digitalWrite(pinLed, LOW);
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {
  char task_name[12];

  // Configure producers pins with input pull-down
  pinMode(button1, INPUT_PULLDOWN);
  pinMode(button2, INPUT_PULLDOWN);
  pinMode(button3, INPUT_PULLDOWN);
  pinMode(button4, INPUT_PULLDOWN);
  // Configure consumers pins with output
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

  noEmptySpaces = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
  noItemsAvailable = xSemaphoreCreateCounting(BUF_SIZE, 0);

  // Create producer tasks
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button1, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button2, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button3, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(producer, task_name, 1024, (void*)&button4, 1, NULL, app_cpu);
  // Create consumer tasks
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led1, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led2, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led3, 1, NULL, app_cpu);
  xTaskCreatePinnedToCore(consumer, task_name, 1024, (void*)&led4, 1, NULL, app_cpu);
}

void loop() {
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}