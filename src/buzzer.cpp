#include "buzzer.h"
#include "config.h"
#include "driver/ledc.h"

// FIX: Use timer 3 and channel 5 — well away from ESP32Servo's allocations
#define BUZZER_CHANNEL  LEDC_CHANNEL_5
#define BUZZER_TIMER    LEDC_TIMER_3
#define BUZZER_SPEED    LEDC_LOW_SPEED_MODE
#define BUZZER_RES      LEDC_TIMER_10_BIT

static bool          beeping      = false;
static unsigned long beepStart    = 0;
static int           beepDuration = 0;

void initBuzzer() {
  // FIX: Use explicit struct member assignment (compatible across IDF versions)
  ledc_timer_config_t timer;
  memset(&timer, 0, sizeof(timer));
  timer.speed_mode      = BUZZER_SPEED;
  timer.duty_resolution = BUZZER_RES;
  timer.timer_num       = BUZZER_TIMER;
  timer.freq_hz         = 1000;
  timer.clk_cfg         = LEDC_AUTO_CLK;
  ledc_timer_config(&timer);

  ledc_channel_config_t ch;
  memset(&ch, 0, sizeof(ch));
  ch.gpio_num   = PIN_BUZZER;
  ch.speed_mode = BUZZER_SPEED;
  ch.channel    = BUZZER_CHANNEL;
  ch.timer_sel  = BUZZER_TIMER;
  ch.duty       = 0;
  ch.hpoint     = 0;
  ledc_channel_config(&ch);

  Serial.println("[BUZZER] Initialized.");
}

void beepOnce(int frequencyHz, int durationMs) {
  if (beeping) return;
  ledc_set_freq(BUZZER_SPEED, BUZZER_TIMER, frequencyHz);
  ledc_set_duty(BUZZER_SPEED, BUZZER_CHANNEL, 512);
  ledc_update_duty(BUZZER_SPEED, BUZZER_CHANNEL);
  beeping      = true;
  beepStart    = millis();
  beepDuration = durationMs;
}

void updateBuzzer() {
  if (!beeping) return;
  if (millis() - beepStart >= (unsigned long)beepDuration) {
    ledc_set_duty(BUZZER_SPEED, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_SPEED, BUZZER_CHANNEL);
    beeping = false;
  }
}