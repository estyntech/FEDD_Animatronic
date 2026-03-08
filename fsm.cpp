#include "fsm.h"
#include "config.h"
#include "sensor.h"
#include "motion.h"
#include "leds.h"
#include "buzzer.h"

// FIX: Declare but don't initialize here — setup() creates it before tasks launch
Telemetry telemetry = { STATE_SCANNING, HEAD_CENTER_DEG, 9999, HEAD_CENTER_DEG };
SemaphoreHandle_t telemetryMutex = NULL;

const char* stateName(FSMState s) {
  switch (s) {
    case STATE_SCANNING: return "SCANNING";
    case STATE_TRACKING: return "TRACKING";
    case STATE_LOST:     return "LOST";
    case STATE_DANGER:   return "DANGER";
    default:             return "UNKNOWN";
  }
}

static void updateTelemetry(FSMState state, float angle, int dist, float lastAngle) {
  if (telemetryMutex == NULL) return; // Guard: mutex not yet created
  if (xSemaphoreTake(telemetryMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    telemetry.state          = state;
    telemetry.headAngle      = angle;
    telemetry.distanceMM     = dist;
    telemetry.lastKnownAngle = lastAngle;
    xSemaphoreGive(telemetryMutex);
  }
}

void fsmTask(void* pvParameters) {
  // FIX: Removed mutex creation from here — it's now done in setup()

  FSMState state        = STATE_SCANNING;
  float    headAngle    = HEAD_CENTER_DEG;
  float    lastAngle    = HEAD_CENTER_DEG;
  int      scanDir      = 1;

  float    lostSearchCenter = HEAD_CENTER_DEG;
  int      lostSearchDir    = 1;
  bool     lostFullSweep    = false;

  unsigned long lastStepTime = 0;
  unsigned long lastBeepTime = 0;

  setHeadAngle(headAngle);
  setJawAngle(JAW_CLOSED_DEG);

  while (true) {
    unsigned long now   = millis();
    int  dist           = readDistanceMM();
    bool objectNear     = (dist > 0 && dist < DIST_TRACK_MM);
    bool objectDanger   = (dist > 0 && dist < DIST_DANGER_MM);
    bool objectLost     = (dist <= 0 || dist > DIST_LOST_MM);

    switch (state) {

      case STATE_SCANNING:
        setLEDPulse(COLOR_BLUE, PULSE_SLOW_MS);
        if (now - lastStepTime >= (unsigned long)SCAN_STEP_MS) {
          lastStepTime = now;
          headAngle   += scanDir * SCAN_STEP_DEG;
          if (headAngle >= HEAD_MAX_DEG) { headAngle = HEAD_MAX_DEG; scanDir = -1; }
          if (headAngle <= HEAD_MIN_DEG) { headAngle = HEAD_MIN_DEG; scanDir =  1; }
          setHeadAngle(headAngle);
          Serial.printf("[SCANNING] Angle: %.1f\n", headAngle);
        }
        if (objectNear) {
          lastAngle = headAngle;
          beepOnce(BEEP_FREQ_DETECT, BEEP_DUR_DETECT);
          state = STATE_TRACKING;
          Serial.printf("[FSM] SCANNING -> TRACKING at %.1f deg / %dmm\n", headAngle, dist);
        }
        break;

      case STATE_TRACKING:
        setLEDSolid(COLOR_RED);
        setJawAngle(JAW_CLOSED_DEG);
        if (objectLost) {
          lostSearchCenter = lastAngle;
          lostSearchDir    = 1;
          lostFullSweep    = false;
          state = STATE_LOST;
          Serial.println("[FSM] TRACKING -> LOST");
          break;
        }
        if (objectDanger) {
          state = STATE_DANGER;
          Serial.printf("[FSM] TRACKING -> DANGER at %dmm\n", dist);
          break;
        }
        lastAngle = headAngle;
        Serial.printf("[TRACKING] Angle: %.1f | Dist: %dmm\n", headAngle, dist);
        break;

      case STATE_LOST:
        setLEDPulse(COLOR_RED, PULSE_SLOW_MS);
        if (now - lastStepTime >= (unsigned long)SCAN_FAST_MS) {
          lastStepTime = now;
          headAngle   += lostSearchDir * SCAN_STEP_DEG;
          float searchMin = max((float)HEAD_MIN_DEG, lostSearchCenter - LOST_SEARCH_ARC);
          float searchMax = min((float)HEAD_MAX_DEG, lostSearchCenter + LOST_SEARCH_ARC);
          if (headAngle >= searchMax) { headAngle = searchMax; lostSearchDir = -1; }
          if (headAngle <= searchMin) { headAngle = searchMin; lostSearchDir =  1; lostFullSweep = true; }
          setHeadAngle(headAngle);
          Serial.printf("[LOST] Searching: %.1f (center: %.1f)\n", headAngle, lostSearchCenter);
        }
        if (objectNear) {
          lastAngle = headAngle;
          beepOnce(BEEP_FREQ_DETECT, BEEP_DUR_DETECT);
          state = STATE_TRACKING;
          Serial.printf("[FSM] LOST -> TRACKING at %.1f\n", headAngle);
          break;
        }
        if (lostFullSweep) {
          state   = STATE_SCANNING;
          scanDir = 1;
          Serial.println("[FSM] LOST -> SCANNING");
        }
        break;

      case STATE_DANGER:
        setLEDPulse(COLOR_RED, PULSE_FAST_MS);
        setJawAngle(JAW_OPEN_DEG);
        {
          int clampedDist  = max(1, min(dist, (int)DIST_DANGER_MM));
          int beepInterval = map(clampedDist, 1, DIST_DANGER_MM, 50, 500);
          if (now - lastBeepTime >= (unsigned long)beepInterval) {
            lastBeepTime = now;
            beepOnce(BEEP_FREQ_DANGER, 40);
          }
        }
        Serial.printf("[DANGER] Distance: %dmm\n", dist);
        if (objectLost) {
          setJawAngle(JAW_CLOSED_DEG);
          lostSearchCenter = lastAngle;
          lostSearchDir    = 1;
          lostFullSweep    = false;
          state = STATE_LOST;
          Serial.println("[FSM] DANGER -> LOST");
          break;
        }
        if (!objectDanger && objectNear) {
          setJawAngle(JAW_CLOSED_DEG);
          state = STATE_TRACKING;
          Serial.println("[FSM] DANGER -> TRACKING");
        }
        break;
    }

    updateTelemetry(state, headAngle, dist, lastAngle);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}