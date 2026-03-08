#include "sensor.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

static Adafruit_VL53L0X sensor;

bool initSensor() {
  // FIX: Must call Wire.begin() with custom pins BEFORE sensor.begin()
  // and pass the Wire instance explicitly so Adafruit uses the right bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  
  // begin(address, debug, &wire) — explicitly pass our Wire instance
  if (!sensor.begin(VL53L0X_I2C_ADDR, false, &Wire)) {
    Serial.println("[SENSOR] ERROR: VL53L0X not found on I2C bus!");
    return false;
  }

  sensor.startRangeContinuous(20); // 20ms inter-measurement period
  Serial.println("[SENSOR] VL53L0X initialized OK.");
  return true;
}

int readDistanceMM() {
  if (!sensor.isRangeComplete()) return -1;

  VL53L0X_RangingMeasurementData_t measure;
  sensor.getRangingMeasurement(&measure, false);

  // RangeStatus 4 = phase failure (out of range or no object)
  if (measure.RangeStatus == 4) return -1;

  return (int)measure.RangeMilliMeter;
}