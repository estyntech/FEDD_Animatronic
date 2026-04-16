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
const char* ssid     = "benjamin";
const char* password = "Yoda0928";
WebServer webServer(80);
volatile int   webAngle    = 90;
volatile int   webDist     = 0;
volatile char  webMode[16] = "SCANNING";

portMUX_TYPE modeMux = portMUX_INITIALIZER_UNLOCKED;

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

const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>PROJECT PARALLAX</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: #080c08;
    color: #c8d8c0;
    font-family: 'Courier New', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 28px 16px 40px;
    gap: 24px;
  }

  header {
    text-align: center;
  }

  .title {
    font-size: 1.5rem;
    letter-spacing: 6px;
    text-transform: uppercase;
    color: #4ecb71;
    font-weight: bold;
  }

  .subtitle {
    font-size: 0.7rem;
    letter-spacing: 3px;
    color: #31de31;
    margin-top: 4px;
    text-transform: uppercase;
  }

  .stats-row {
    display: flex;
    gap: 12px;
    flex-wrap: wrap;
    justify-content: center;
    width: 100%;
    max-width: 540px;
  }

  .stat-card {
    flex: 1;
    min-width: 120px;
    background: #0d150d;
    border: 1px solid #1a2e1a;
    border-radius: 8px;
    padding: 12px 16px;
    text-align: center;
  }

  .stat-label {
    font-size: 0.6rem;
    letter-spacing: 3px;
    text-transform: uppercase;
    color: #31de31;
    margin-bottom: 6px;
  }

  .stat-value {
    font-size: 1.4rem;
    font-weight: bold;
    color: #4ecb71;
    transition: color 0.3s;
  }

  .stat-value.danger { color: #e05050; }
  .stat-value.warning { color: #e0a030; }

  .mode-badge {
    display: inline-block;
    font-size: 0.75rem;
    letter-spacing: 2px;
    padding: 4px 14px;
    border-radius: 4px;
    font-weight: bold;
    transition: background 0.3s, color 0.3s, border-color 0.3s;
  }

  .mode-SCANNING { background: #081822; color: #40aaee; border: 1px solid #40aaee; }
  .mode-DETECTED { background: #1a0808; color: #e05050; border: 1px solid #e05050; animation: dangerPulse 0.3s infinite alternate; }

  @keyframes dangerPulse { from { opacity: 1; } to { opacity: 0.4; } }

  .radar-wrapper {
    position: relative;
    width: 100%;
    max-width: 500px;
  }

  canvas {
    display: block;
    width: 100%;
    height: auto;
  }

  .status-bar {
    width: 100%;
    max-width: 500px;
    background: #0d150d;
    border: 1px solid #1a2e1a;
    border-radius: 8px;
    padding: 10px 16px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.7rem;
    color: #31de31;
    letter-spacing: 1px;
  }

  #conn-status { color: #4ecb71; }
  #conn-status.offline { color: #e05050; }

  .log-box {
    width: 100%;
    max-width: 500px;
    background: #0a120a;
    border: 1px solid #1a2e1a;
    border-radius: 8px;
    padding: 10px 14px;
    font-size: 0.7rem;
    color: #31de31;
    height: 90px;
    overflow-y: auto;
    line-height: 1.6;
  }

  .log-entry { color: #31de31; }
  .log-entry.alert { color: #c04040; }

  .range-bar-wrap {
    width: 100%;
    max-width: 500px;
    background: #0d150d;
    border: 1px solid #1a2e1a;
    border-radius: 8px;
    padding: 12px 16px;
  }

  .range-label {
    font-size: 0.6rem;
    letter-spacing: 2px;
    text-transform: uppercase;
    color: #31de31;
    margin-bottom: 8px;
  }

  .range-track {
    width: 100%;
    height: 6px;
    background: #1a2e1a;
    border-radius: 3px;
    position: relative;
    overflow: hidden;
  }

  .range-fill {
    height: 100%;
    border-radius: 3px;
    background: #4ecb71;
    transition: width 0.2s, background 0.3s;
    width: 0%;
  }

  .range-ticks {
    display: flex;
    justify-content: space-between;
    margin-top: 4px;
    font-size: 0.6rem;
    color: #31de31;
  }
</style>
</head>
<body>

<header>
  <div class="title">PROJECT PARALLAX</div>
  <div class="subtitle">Object Detection System</div>
</header>

<div class="stats-row">
  <div class="stat-card">
    <div class="stat-label">Mode</div>
    <div class="stat-value" id="stat-mode">
      <span class="mode-badge mode-SCANNING" id="mode-badge">SCANNING</span>
    </div>
  </div>
  <div class="stat-card">
    <div class="stat-label">Head Angle</div>
    <div class="stat-value" id="stat-angle">90&deg;</div>
  </div>
  <div class="stat-card">
    <div class="stat-label">Distance</div>
    <div class="stat-value" id="stat-dist">---</div>
  </div>
</div>

<div class="radar-wrapper">
  <canvas id="radar" width="500" height="270"></canvas>
</div>

<div class="range-bar-wrap">
  <div class="range-label">Proximity &mdash; 0mm to 500mm detection range</div>
  <div class="range-track">
    <div class="range-fill" id="range-fill"></div>
  </div>
  <div class="range-ticks">
    <span>0mm</span>
    <span>125mm</span>
    <span>250mm</span>
    <span>375mm</span>
    <span>500mm</span>
  </div>
</div>

<div class="status-bar">
  <span>ESP32 &mdash; NCSU WIFI</span>
  <span id="conn-status">&#9632; CONNECTING...</span>
  <span id="update-time">--:--:--</span>
</div>

<div class="log-box" id="log-box">
  <div class="log-entry">System initializing...</div>
</div>

<script>
const POLL_MS     = 150;
const MAX_DIST    = 500;
const canvas      = document.getElementById('radar');
const ctx         = canvas.getContext('2d');
const CW          = canvas.width;
const CH          = canvas.height;
const CX          = CW / 2;
const CY          = CH;
const R           = CH - 10;

let currentAngle  = 90;
let currentDist   = -1;
let currentMode   = 'SCANNING';
let sweepTrail    = [];
let animFrame;

function toRad(deg) { return (180 - deg) * Math.PI / 180; }

function drawRadar(angle, dist, mode) {
  ctx.clearRect(0, 0, CW, CH);

  ctx.fillStyle = '#080c08';
  ctx.fillRect(0, 0, CW, CH);

  const isDetected = mode === 'DETECTED';
  const baseGreen  = isDetected ? '#3a1010' : '#0a180a';
  const ringColor  = isDetected ? '#5a2020' : '#1a3a1a';
  const lineColor  = isDetected ? '#e05050' : '#4ecb71';
  const dimLine    = isDetected ? '#602020' : '#1a4a1a';

  ctx.fillStyle = baseGreen;
  ctx.beginPath();
  ctx.arc(CX, CY, R, Math.PI, 0);
  ctx.fill();

  for (let i = 1; i <= 4; i++) {
    ctx.beginPath();
    ctx.arc(CX, CY, R * i / 4, Math.PI, 0);
    ctx.strokeStyle = ringColor;
    ctx.lineWidth = i === 4 ? 1 : 0.5;
    ctx.stroke();
  }

  for (let a = 0; a <= 180; a += 30) {
    const rad = a * Math.PI / 180;
    ctx.beginPath();
    ctx.moveTo(CX, CY);
    ctx.lineTo(CX - Math.cos(rad) * R, CY - Math.sin(rad) * R);
    ctx.strokeStyle = ringColor;
    ctx.lineWidth = 0.5;
    ctx.stroke();
  }

  ctx.fillStyle = '#31de31';
  ctx.font = '10px Courier New';
  ctx.textAlign = 'center';
  const distLabels = [125, 250, 375, 500];
  distLabels.forEach((d, i) => {
    const yr = CY - R * (i + 1) / 4 - 3;
    ctx.fillText(d + 'mm', CX, yr);
  });

  ctx.fillStyle = '#31de31';
  ctx.textAlign = 'left';
  ctx.fillText('180', 4, CH - 4);
  ctx.textAlign = 'right';
  ctx.fillText('0', CW - 4, CH - 4);

  sweepTrail.push({ angle, time: Date.now() });
  sweepTrail = sweepTrail.filter(t => Date.now() - t.time < 800);

  sweepTrail.forEach((t, i) => {
    const alpha = (i / sweepTrail.length) * 0.25;
    const rad   = toRad(t.angle);
    ctx.beginPath();
    ctx.moveTo(CX, CY);
    ctx.lineTo(CX + Math.cos(rad) * R, CY - Math.sin(rad) * R);
    ctx.strokeStyle = isDetected
      ? `rgba(224,80,80,${alpha})`
      : `rgba(78,203,113,${alpha})`;
    ctx.lineWidth = 6;
    ctx.stroke();
  });

  const sweepRad = toRad(angle);
  ctx.beginPath();
  ctx.moveTo(CX, CY);
  ctx.lineTo(CX + Math.cos(sweepRad) * R, CY - Math.sin(sweepRad) * R);
  ctx.strokeStyle = lineColor;
  ctx.lineWidth = 1.5;
  ctx.stroke();

  if (dist > 0 && dist <= MAX_DIST) {
    const normDist = dist / MAX_DIST;
    const dotR     = R * normDist;
    const dotX     = CX + Math.cos(sweepRad) * dotR;
    const dotY     = CY - Math.sin(sweepRad) * dotR;

    ctx.beginPath();
    ctx.arc(dotX, dotY, 10, 0, 2 * Math.PI);
    ctx.fillStyle = isDetected ? 'rgba(224,80,80,0.15)' : 'rgba(78,203,113,0.15)';
    ctx.fill();

    ctx.beginPath();
    ctx.arc(dotX, dotY, 5, 0, 2 * Math.PI);
    ctx.fillStyle = lineColor;
    ctx.fill();

    ctx.beginPath();
    ctx.arc(dotX, dotY, 5, 0, 2 * Math.PI);
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 0.5;
    ctx.stroke();

    ctx.fillStyle = lineColor;
    ctx.font = 'bold 11px Courier New';
    ctx.textAlign = 'center';
    const labelX = dotX + (dotX > CX ? 28 : -28);
    const labelY = dotY - 10;
    ctx.fillText(dist + 'mm', labelX, labelY);
  }

  ctx.beginPath();
  ctx.arc(CX, CY, 6, 0, 2 * Math.PI);
  ctx.fillStyle = lineColor;
  ctx.fill();

  ctx.beginPath();
  ctx.arc(CX, CY, 3, 0, 2 * Math.PI);
  ctx.fillStyle = '#080c08';
  ctx.fill();
}

function updateUI(angle, dist, mode) {
  currentAngle = angle;
  currentDist  = dist;
  currentMode  = mode;

  const angleEl = document.getElementById('stat-angle');
  const distEl  = document.getElementById('stat-dist');
  const badge   = document.getElementById('mode-badge');
  const fill    = document.getElementById('range-fill');

  angleEl.textContent = angle + '\u00b0';
  angleEl.className   = 'stat-value';

  if (dist > 0) {
    distEl.textContent = dist + 'mm';
    distEl.className   = 'stat-value' + (dist < 150 ? ' danger' : dist < 300 ? ' warning' : '');

    const pct = Math.min(100, Math.round((1 - dist / MAX_DIST) * 100));
    fill.style.width      = pct + '%';
    fill.style.background = dist < 150 ? '#e05050' : dist < 300 ? '#e0a030' : '#4ecb71';
  } else {
    distEl.textContent    = '---';
    distEl.className      = 'stat-value';
    fill.style.width      = '0%';
    fill.style.background = '#4ecb71';
  }

  badge.textContent = mode;
  badge.className   = 'mode-badge mode-' + mode;

  drawRadar(angle, dist, mode);
}

function addLog(msg, isAlert) {
  const box  = document.getElementById('log-box');
  const line = document.createElement('div');
  line.className   = 'log-entry' + (isAlert ? ' alert' : '');
  line.textContent = '[' + new Date().toLocaleTimeString() + '] ' + msg;
  box.appendChild(line);
  box.scrollTop = box.scrollHeight;
  while (box.children.length > 30) box.removeChild(box.firstChild);
}

function setConnected(ok) {
  const el = document.getElementById('conn-status');
  el.textContent = ok ? '\u25a0 CONNECTED' : '\u25a0 OFFLINE';
  el.className   = ok ? '' : 'offline';
}

function updateTime() {
  document.getElementById('update-time').textContent = new Date().toLocaleTimeString();
}

let lastMode = '';

async function poll() {
  try {
    const res  = await fetch('/telemetry');
    const data = await res.json();
    const angle = parseInt(data.angle) || 90;
    const dist  = parseInt(data.dist)  || -1;
    const mode  = data.mode || 'SCANNING';

    updateUI(angle, dist, mode);
    setConnected(true);
    updateTime();

    if (mode !== lastMode) {
      addLog('Mode changed: ' + mode, mode === 'DETECTED');
      lastMode = mode;
    }
  } catch (e) {
    setConnected(false);
    addLog('Connection lost — retrying...', true);
  }
}

drawRadar(90, -1, 'SCANNING');
addLog('Connecting to ESP32...');

setInterval(poll, POLL_MS);

</script>
</body>
</html>
)rawhtml";

void handleTelemetry() {
  // Read webMode inside the spinlock so we never get a half-written string
  char modeCopy[16];
  portENTER_CRITICAL(&modeMux);
  strncpy(modeCopy, (const char*)webMode, sizeof(modeCopy));
  portEXIT_CRITICAL(&modeMux);

  String json = "{\"angle\":";
  json += String((int)webAngle);
  json += ",\"dist\":";
  json += String((int)webDist);
  json += ",\"mode\":\"";
  json += String(modeCopy);
  json += "\"}";

  // Allow any origin — important if you ever open the page from a file:// URL
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", json);
}

void handleRoot() {
  // INDEX_HTML is now defined right above
  webServer.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

// -----------------------------------------------------------------------
// WiFi + web server task (Core 0)
// -----------------------------------------------------------------------
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
    Serial.println("\n[WIFI] Failed to connect.");
    vTaskDelete(NULL);
    return;
  }
  webServer.on("/",          handleRoot);
  webServer.on("/telemetry", handleTelemetry);
  webServer.begin();
  for (;;) {
    webServer.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
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
  digitalWrite(SHUTDOWN, HIGH);  // Release from reset FIRST
  delay(10);                     // Give VL53L0X time to boot (~1.2ms needed, 10ms is safe)

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
	xTaskCreatePinnedToCore(webTask, "Web", 16384, NULL, 1, NULL, 0);

	xTaskCreatePinnedToCore(
		idle,
		"Idle",
		10000,
		NULL,
		2,
		&Idle,
		1
	);

	xTaskCreatePinnedToCore(
		reading,
		"Read",
		65536,
		NULL,
		3,
		&Read,
		1
	);

	digitalWrite(BLUE_LED_PIN, LOW);
	digitalWrite(RED_LED_PIN, LOW);
}

void loop()
{
		// intentionally empty bc everything runs in FreeRTOS tasks
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
			portENTER_CRITICAL(&modeMux);
			strcpy((char*)webMode, "DETECTED");
			portEXIT_CRITICAL(&modeMux);
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
			portENTER_CRITICAL(&modeMux);
      		strcpy((char*)webMode, "SCANNING");
      		portEXIT_CRITICAL(&modeMux);
			digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(BLUE_LED_PIN, HIGH);
			// tone(BZ, IDLE_FREQ, BEEP_TIME_IDLE);
		}
		vTaskDelay(25 / portTICK_PERIOD_MS);
	}
}
