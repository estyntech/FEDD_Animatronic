#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>

Servo neck;
Servo jaw;

void setup() 
{
  Serial.begin(9600);
  neck.write(90);
  jaw.write(90);
}

void loop() 
{

}