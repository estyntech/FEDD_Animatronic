#include "leds.h"
#include "config.h"

static LEDColor      currentColor  = COLOR_OFF;
static bool          pulseMode     = false;
static int           pulsePeriod   = 1000;
static bool          ledState      = false;
static unsigned long lastToggle    = 0;

// Track last requested mode to avoid resetting timer every tick
static LEDColor      lastRequestedColor  = COLOR_OFF;
static bool          lastRequestedPulse  = false;
static int           lastRequestedPeriod = 0;

void initLEDs() {
  pinMode(PIN_LED_RED,   OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE,  OUTPUT);
  setLEDSolid(COLOR_OFF);
  Serial.println("[LEDS] Initialized.");
}

static void applyColor(LEDColor color, bool on) {
  digitalWrite(PIN_LED_RED,   LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE,  LOW);
  if (!on) return;
  switch (color) {
    case COLOR_RED:   digitalWrite(PIN_LED_RED,   HIGH); break;
    case COLOR_GREEN: digitalWrite(PIN_LED_GREEN, HIGH); break;
    case COLOR_BLUE:  digitalWrite(PIN_LED_BLUE,  HIGH); break;
    case COLOR_WHITE:
      digitalWrite(PIN_LED_RED,   HIGH);
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_BLUE,  HIGH);
      break;
    default: break;
  }
}

void setLEDSolid(LEDColor color) {
  pulseMode    = false;
  currentColor = color;
  lastRequestedColor  = color;
  lastRequestedPulse  = false;
  applyColor(color, true);
}

void setLEDPulse(LEDColor color, int periodMs) {
  // FIX: Only update state if something actually changed.
  // Previously this reset lastToggle every FSM tick, breaking the pulse timing.
  if (lastRequestedPulse && lastRequestedColor == color && lastRequestedPeriod == periodMs) {
    return; // Nothing changed — let updateLEDs() handle the timing
  }
  currentColor        = color;
  pulseMode           = true;
  pulsePeriod         = periodMs;
  lastRequestedColor  = color;
  lastRequestedPulse  = true;
  lastRequestedPeriod = periodMs;
  lastToggle          = millis(); // Reset timer only on actual mode change
}

void updateLEDs() {
  if (!pulseMode) return;
  unsigned long now = millis();
  if (now - lastToggle >= (unsigned long)(pulsePeriod / 2)) {
    lastToggle = now;
    ledState   = !ledState;
    applyColor(currentColor, ledState);
  }
}