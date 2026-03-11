#pragma once
#include <Arduino.h>

enum LEDColor {
  COLOR_OFF,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_WHITE
};

void initLEDs();

// Set LEDs to a solid color
void setLEDSolid(LEDColor color);

// Set LEDs to pulse at the given period (ms)
// Call repeatedly from FSM loop — non-blocking
void setLEDPulse(LEDColor color, int periodMs);

// Must be called frequently (every loop tick) to execute non-blocking pulse
void updateLEDs();
