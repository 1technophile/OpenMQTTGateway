#pragma once
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

extern QueueHandle_t BLEJsonQueue;
extern TaskHandle_t BLEJsonTaskHandle;

// Make sure PublishDeviceData is declared somewhere visible!
extern void PublishDeviceData(JsonObject& BLEdata);

// Call this once in your setup/init to start the task
inline void startBLEJsonTask() {
  if (!BLEJsonQueue) {
    BLEJsonQueue = xQueueCreate(4, sizeof(DynamicJsonDocument*));
  }
  if (!BLEJsonTaskHandle) {
    xTaskCreate(
        [](void*) {
          for (;;) { // Infinite loop!
            DynamicJsonDocument* doc;
            if (xQueueReceive(BLEJsonQueue, &doc, portMAX_DELAY) == pdTRUE) {
              JsonObject obj = doc->as<JsonObject>();
              PublishDeviceData(obj);
              delete doc;
            }
          }
          // Never return from here!
          // vTaskDelete(NULL); // Only if you want to end the task (rare)
        },
        "BLEJsonTask",
        8192, // or whatever stack size you need
        nullptr,
        1,
        &BLEJsonTaskHandle);
  }
}
