// =============================================================================
// claude generated
// animatronic.ino — Main Entry Point
// Animatronic Gargoyle | NCSU IEEE Dev Board | ESP32-S3-WROOM-1
//
// Required Libraries (install via Arduino Library Manager):
//   - "VL53L0X" by Pololu
//   - "ESP32Servo" by Kevin Harrington
//   - "WebServer" (included with esp32 Arduino core)
//
// Board: "ESP32S3 Dev Module"
// Flash Mode: QIO / 80MHz
// PSRAM: Disabled (unless using WROOM-2)
// USB CDC On Boot: Enabled (for Serial output via USB)
// =============================================================================

#include "config.h"
#include "sensor.h"
#include "motion.h"
#include "leds.h"
#include "buzzer.h"
#include "fsm.h"
#include "webserver.h"

void sensorTask(void* pvParameters) {
  if (!initSensor()) {
    Serial.println("[SENSOR] FATAL: VL53L0X not found. Halting sensor task.");
    vTaskDelete(NULL);
    return;
  }
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void outputTask(void* pvParameters) {
  while (true) {
    updateLEDs();
    updateBuzzer();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n============================================");
  Serial.println("  Animatronic Gargoyle | NCSU IEEE ESP32-S3");
  Serial.println("============================================");

  initLEDs();
  initBuzzer();
  initServos();

  // FIX: Create mutex here, before any task can access it
  telemetryMutex = xSemaphoreCreateMutex();

  // Startup blink
  setLEDSolid(COLOR_WHITE);
  delay(300);
  setLEDSolid(COLOR_OFF);

  xTaskCreatePinnedToCore(sensorTask, "Sensor", STACK_SENSOR, NULL, PRI_SENSOR, NULL, 0);
  xTaskCreatePinnedToCore(webTask,    "Web",    STACK_WEB,    NULL, PRI_WEB,    NULL, 0);
  xTaskCreatePinnedToCore(fsmTask,    "FSM",    STACK_FSM,    NULL, PRI_FSM,    NULL, 1);
  xTaskCreatePinnedToCore(outputTask, "Output", 2048,         NULL, 2,          NULL, 1);

  Serial.println("[SETUP] All tasks launched.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
