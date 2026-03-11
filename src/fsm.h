// claude generated
#pragma once
#include <Arduino.h>

// FSM States
enum FSMState {
  STATE_SCANNING,
  STATE_TRACKING,
  STATE_LOST,
  STATE_DANGER
};

// Shared telemetry — written by FSM task, read by web server task
// Use the mutex when accessing from web task
struct Telemetry {
  volatile FSMState state;
  volatile float    headAngle;      // Current head angle in degrees
  volatile int      distanceMM;     // Latest sensor reading in mm
  volatile float    lastKnownAngle; // Last angle where object was seen
};

extern Telemetry telemetry;
extern SemaphoreHandle_t telemetryMutex;

// FSM entry point — runs as a FreeRTOS task
void fsmTask(void* pvParameters);

// Human-readable state name for serial/web output
const char* stateName(FSMState s);
