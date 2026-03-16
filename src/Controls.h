#include <Arduino.h>
#include <Servo.h>

#define NECK_PIN P34
#define JAW_PIN P35

#define NECK_MIN_ANGLE 0
#define NECK_MAX_ANGLE 180

#define JAW_MIN_ANGLE 0
#define JAW_MAX_ANGLE 90

#define NECK_TURN_DELAY 25
#define NECK_STOP_DELAY 100

#define JAW_TURN_DELAY 25
#define JAW_STOP_DELAY 25

Servo neck;
Servo jaw;

void initControls()
{
    neck.attach(NECK_PIN);
    jaw.attach(JAW_PIN);

    neck.write(NECK_MAX_ANGLE / 2);
    jaw.write(JAW_MIN_ANGLE);
}

void turnHeadRight()
{
    for(int i = NECK_MIN_ANGLE; i < NECK_MAX_ANGLE; i++)
    {
        neck.write(i);
        delay(NECK_TURN_DELAY);
    }
}

void turnHeadLeft()
{
    for(int i = NECK_MAX_ANGLE; i > NECK_MIN_ANGLE; i--)
    {
        neck.write(i);
        delay(NECK_TURN_DELAY);
    }
}

void operateJaw()
{
  for(int i = JAW_MIN_ANGLE; i < JAW_MAX_ANGLE; i++)
  {
    jaw.write(i);
    delay(JAW_TURN_DELAY);
  }

  delay(JAW_STOP_DELAY);
  
  for(int i = JAW_MAX_ANGLE; i > JAW_MIN_ANGLE; i--)
  {
    jaw.write(i);
    delay(JAW_TURN_DELAY);
  }
}