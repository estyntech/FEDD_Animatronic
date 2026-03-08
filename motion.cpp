#include "motion.h"
#include "config.h"
#include <ESP32Servo.h>

static Servo headServo;
static Servo jawServo;

void initServos() {
  // FIX: Allocate all 4 timers upfront so ESP32Servo doesn't conflict
  // with the buzzer's LEDC channel or I2C peripherals
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  // Timer 3 is reserved for the buzzer — do NOT allocate it here

  headServo.setPeriodHertz(50);
  jawServo.setPeriodHertz(50);

  // attach(pin, minUs, maxUs) — 500-2400µs covers most hobby servos
  headServo.attach(PIN_SERVO_HEAD, 500, 2400);
  jawServo.attach(PIN_SERVO_JAW,   500, 2400);

  setHeadAngle(HEAD_CENTER_DEG);
  setJawAngle(JAW_CLOSED_DEG);

  Serial.println("[MOTION] Servos initialized.");
}

void setHeadAngle(float degrees) {
  degrees = constrain(degrees, HEAD_MIN_DEG, HEAD_MAX_DEG);
  headServo.write((int)degrees);
}

void setJawAngle(float degrees) {
  degrees = constrain(degrees, JAW_CLOSED_DEG, JAW_OPEN_DEG);
  jawServo.write((int)degrees);
}