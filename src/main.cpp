#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "Controls.h"
#include <WiFi.h>
#include <WebServer.h>

#define SDA	P23
#define SCL	P24
#define SHUTDOWN P25
// #define INT P22
#define ADDR 0x30
#define DETECT_RANGE 500
#define INTERIOR_RANGE 30
#define BZ P39
#define BEEP_TIME_IDLE 250
#define BEEP_TIME_DANGER 250 / 3
#define DANGER_FREQ 880
#define IDLE_FREQ 1320

// --- WiFi config ---
const char* ssid     = "ncsu";
const char* password = "";
WebServer webServer(80);
volatile int   webAngle    = 90;
volatile int   webDist     = 0;
volatile char  webMode[16] = "SCANNING";

Adafruit_VL53L0X tof = Adafruit_VL53L0X();
TwoWire bus = TwoWire(1);
// bool detected;
// VL53L0X_RangingMeasurementData_t measure;
// int tofReading;

bool checkInRange(int, int, int);
void stopIdle(bool);
void turnHeadRight();
void turnHeadLeft();
void readTof(int);

void idle(void *);
void reading(void *);

TaskHandle_t Idle;
TaskHandle_t Read;

int SensorDistance;

void handleTelemetry() {
  String json = "{\"angle\":";
  json += String(webAngle);
  json += ",\"dist\":";
  json += String(webDist);
  json += ",\"mode\":\"";
  json += String(webMode);
  json += "\"}";
  webServer.send(200, "application/json", json);
}

void handleRoot() {
  // We'll serve the HTML from a string
  extern const char INDEX_HTML[];
  webServer.send(200, "text/html", INDEX_HTML);
}

void webTask(void* parameters) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n", 
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Failed.");
    vTaskDelete(NULL);
    return;
  }
  webServer.on("/", handleRoot);
  webServer.on("/telemetry", handleTelemetry);
  webServer.begin();
  for (;;) {
    webServer.handleClient();
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
}

void setup()
{
	// delay(1000);
	Serial.begin(115200);

	/*
	while (!Serial)
	{
		delay(1);
	}
	*/

	if (!bus.begin(SDA, SCL))
	{
		Serial.println("Failed to initialize i2c bus!");
	}

	Serial.println("Initialized i2c bus!");
	pinMode(SHUTDOWN, OUTPUT);
	digitalWrite(SHUTDOWN, LOW);

	if (!tof.begin(ADDR, true, &bus))
	{
		Serial.println("Initialization of VL53L0X failed!");
		while(true);
	}

	digitalWrite(SHUTDOWN, HIGH);
	Serial.println("Successfully initialized VL53L0X!");

	// digitalWrite(INT, HIGH);
	initControls();

	//web page task
	xTaskCreatePinnedToCore(webTask, "Web", 8192, NULL, 1, NULL, 0);

	xTaskCreatePinnedToCore(
		idle,
		"Idle",
		10000,
		NULL,
		1,
		&Idle,
		0
	);

	xTaskCreatePinnedToCore(
		reading,
		"Read",
		200000,
		NULL,
		1,
		&Read,
		1
	);

	digitalWrite(BLUE_LED_PIN, LOW);
	digitalWrite(RED_LED_PIN, LOW);
}

void loop()
{
		
}

/*
void readTof(int idealRange)
{
	tofReading = tof.rangingTest(&measure, false);
	if(checkInRange(measure.RangeStatus, measure.RangeMilliMeter, idealRange))
	{
		Serial.println("Blocked!");
		stopHead();
	}

	else 
	{
		Serial.println("Not blocked.");
	}
}
*/

bool checkInRange(int status, int distance, int range)
{
	return status != 4 && distance <= range;
}

void turnHeadRight()
{
    for(int i = NECK_MIN_ANGLE; i < NECK_MAX_ANGLE; i++)
    {
		// readTof(idealRange);
        neck.write(i);
        currentNeckAngle = i;
        // tof.rangingTest(&measure, false);
    	vTaskDelay(NECK_TURN_DELAY / portTICK_PERIOD_MS);
    }
}

void turnHeadLeft()
{
    for(int i = NECK_MAX_ANGLE; i > NECK_MIN_ANGLE; i--)
    {
		// readTof(idealRange);
        neck.write(i);
        currentNeckAngle = i;
		// tof.rangingTest(&measure, false);
        vTaskDelay(NECK_TURN_DELAY / portTICK_PERIOD_MS);
    }
}

void idle(void *parameters)
{
	digitalWrite(BLUE_LED_PIN, HIGH);
	digitalWrite(RED_LED_PIN, LOW);

	for(;;)
	{
		centerHead();

		turnHeadLeft();
		vTaskDelay(NECK_STOP_DELAY / portTICK_PERIOD_MS);

		turnHeadRight();
		vTaskDelay(NECK_STOP_DELAY / portTICK_PERIOD_MS);
	}
}

void reading(void *parameters)
{
	for(;;)
	{
		VL53L0X_RangingMeasurementData_t measure;
		tof.rangingTest(&measure, false);
		SensorDistance = measure.RangeMilliMeter;
		Serial.println(SensorDistance);
		
	    // Update web telemetry
    	webAngle = currentNeckAngle;
    	webDist  = SensorDistance;
		
		if(checkInRange(measure.RangeStatus, SensorDistance, DETECT_RANGE) && SensorDistance > INTERIOR_RANGE)
		{
			strcpy((char*)webMode, "DETECTED");
			digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(BLUE_LED_PIN, LOW);
			Serial.println("Blocked!");
			Serial.println(SensorDistance);
			stopHead();
			/*
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			tone(BZ, DANGER_FREQ, BEEP_TIME_DANGER);
			*/
		}
		else 
		{
			strcpy((char*)webMode, "SCANNING");
			digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(BLUE_LED_PIN, HIGH);
			// tone(BZ, IDLE_FREQ, BEEP_TIME_IDLE);
		}
		// vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}
