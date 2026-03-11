// =============================================================================
// claude generated
// config.h — Central Configuration
// Animatronic Gargoyle | NCSU IEEE Dev Board | ESP32-S3-WROOM-1
// =============================================================================
// Edit this file to match your physical wiring. All GPIO numbers reference
// the ESP32-S3 GPIO numbering, cross-checked against the NCSU IEEE Dev Board
// V1.0 pinout table (J3/J4 headers).
//
// SAFE GPIOs on ESP32-S3-WROOM-1 for general use:
//   GPIO 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18
// AVOID: GPIO 0 (Boot), GPIO 19/20 (USB), GPIO 26-32 (Flash/PSRAM)
// RESTRICTED (*): GPIO 1, 2, 42, 41 — check datasheet before use
// =============================================================================

#pragma once

// --- WiFi -------------------------------------------------------------------
// NCSU open network — no password needed for registered devices
#define WIFI_SSID        "ncsu"
#define WIFI_PASSWORD    ""

// --- Servo Motors -----------------------------------------------------------
#define PIN_SERVO_HEAD    4    // Head rotation servo PWM
#define PIN_SERVO_JAW     5    // Jaw servo PWM

// --- VL53L0X I2C ------------------------------------------------------------
// GPIO 8/9 are the native I2C0 pins on ESP32-S3
#define PIN_I2C_SDA       8
#define PIN_I2C_SCL       9

// --- RGB Eye LEDs -----------------------------------------------------------
// Two RGB LEDs wired in parallel (common cathode assumed).
// Drive HIGH to illuminate each channel.
#define PIN_LED_RED      10
#define PIN_LED_GREEN    11
#define PIN_LED_BLUE     12

// --- Piezo Passive Buzzer ---------------------------------------------------
#define PIN_BUZZER        6

// --- FSM Distance Thresholds (millimeters) ----------------------------------
#define DIST_TRACK_MM   1500   // Closer than this → start tracking
#define DIST_DANGER_MM   400   // Closer than this → DANGER state
#define DIST_LOST_MM    1800   // Farther than this → object considered lost
                               // Hysteresis: LOST > TRACK prevents rapid toggling

// --- Head Servo Motion ------------------------------------------------------
#define HEAD_MIN_DEG       0   // Full left
#define HEAD_MAX_DEG     180   // Full right
#define HEAD_CENTER_DEG   90

#define SCAN_STEP_DEG      1   // Degrees per step (lower = smoother sweep)
#define SCAN_STEP_MS      15   // ms between steps at normal scan speed
#define SCAN_FAST_MS       7   // ms between steps during lost-tracking search
#define LOST_SEARCH_ARC   55   // Degrees either side of last known angle to search
#define TRACK_SMOOTH       3   // Max degrees per update when tracking (smoothing)

// --- Jaw Servo --------------------------------------------------------------
#define JAW_CLOSED_DEG     0
#define JAW_OPEN_DEG      45   // Tune this after printing and assembly

// --- LED Pulse Timing (ms) --------------------------------------------------
#define PULSE_SLOW_MS    1000  // Slow pulse: SCANNING (blue) and LOST (red)
#define PULSE_FAST_MS     150  // Fast pulse: DANGER (red)

// --- Buzzer -----------------------------------------------------------------
#define BEEP_FREQ_DETECT  1000  // Hz — detection beep
#define BEEP_DUR_DETECT    100  // ms — detection beep duration
#define BEEP_FREQ_DANGER  2000  // Hz — danger beep tone

// --- Web Server -------------------------------------------------------------
#define WEB_PORT            80
#define TELEMETRY_MS       100  // Web page polling interval (ms)

// --- FreeRTOS ---------------------------------------------------------------
#define PRI_SENSOR          3
#define PRI_FSM             2
#define PRI_WEB             1
#define STACK_SENSOR      4096
#define STACK_FSM         8192
#define STACK_WEB        16384
