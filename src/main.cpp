#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "Controls.h"

#define SDA	P36
#define SCL	P37

Adafruit_VL53L0X tof = Adafruit_VL53L0X();
TwoWire bus = TwoWire(0);

void setup()
{
	Serial.begin(115200);

	while (!Serial)
	{
		delay(1);
	}

	bus.begin(SDA, SCL);

	if (!tof.begin(0x29, &bus))
	{
		Serial.println("Initialization of VL53L0X failed!");
		while(true);
	}

	Serial.println("Successfully initialized VL53L0X!");
}

void loop()
{

	VL53L0X_RangingMeasurementData_t measure;
	tof.rangingTest(&measure, false);

	if (measure.RangeStatus != 4)
	{
		Serial.println("Distance (mm): " + (String)measure.RangeMilliMeter);
	}

	else
	{
		Serial.println("Out of range.");
	}

	turnHeadRight();
	delay(NECK_STOP_DELAY);

	operateJaw();

	turnHeadLeft();
	delay(NECK_STOP_DELAY);

	operateJaw();
}