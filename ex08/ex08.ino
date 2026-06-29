// 作业5-ex08：物联网安防报警器模拟实验
// 默认接线：LED -> GPIO2，触摸引脚 -> GPIO4(T0)
// 烧录后连接 WiFi 热点 ESP32-Alarm，浏览器打开 http://192.168.4.1

#include <WiFi.h>
#include <WebServer.h>

const char *AP_SSID = "ESP32-Alarm";
const char *AP_PASSWORD = "12345678";

const int LED_PIN = 2;
const int TOUCH_PIN = 4;

const int CALIBRATION_SAMPLES = 40;
const int TOUCH_MARGIN = 18;
const unsigned long ALARM_INTERVAL_MS = 80;

WebServer server(80);

int touchThreshold = 0;
bool armed = false;
bool alarmActive = false;
bool ledState = false;
unsigned long previousBlinkMillis = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 安防报警器</title>
  <style>
    body { margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; font-family: Arial, "Microsoft YaHei", sans-serif; background: #f7f8fb; color: #111827; }
    main { width: min(90vw, 420px); text-align: center; }
    h1 { font-size: 28px; }
    .status { margin: 24px 0; font-size: 24px; font-weight: 700; }
    button { min-width: 130px; margin: 8px; padding: 12px 18px; border: 0; border-radius: 6px; color: white; font-size: 18px; }
    .arm { background: #0f766e; }
    .disarm { background: #b91c1c; }
  </style>
</head>
<body>
  <main>
    <h1>ESP32 安防报警器</h1>
    <div id="status" class="status">读取中...</div>
    <button class="arm" onclick="sendCommand('/arm')">布防 Arm</button>
    <button class="disarm" onclick="sendCommand('/disarm')">撤防 Disarm</button>
  </main>
  <script>
    const statusBox = document.getElementById('status');
    function render(data) {
      if (data.alarm) statusBox.textContent = '报警中';
      else if (data.armed) statusBox.textContent = '已布防';
      else statusBox.textContent = '未布防';
    }
    function refresh() {
      fetch('/status').then(r => r.json()).then(render).catch(() => {});
    }
    function sendCommand(url) {
      fetch(url).then(() => refresh()).catch(() => {});
    }
    refresh();
    setInterval(refresh, 500);
  </script>
</body>
</html>
)rawliteral";

int readTouchAverage(int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += touchRead(TOUCH_PIN);
    delay(8);
  }
  return total / samples;
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleArm() {
  armed = true;
  alarmActive = false;
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "armed");
}

void handleDisarm() {
  armed = false;
  alarmActive = false;
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "disarmed");
}

void handleStatus() {
  String json = "{";
  json += "\"armed\":";
  json += armed ? "true" : "false";
  json += ",\"alarm\":";
  json += alarmActive ? "true" : "false";
  json += ",\"touch\":";
  json += touchRead(TOUCH_PIN);
  json += "}";
  server.send(200, "application/json", json);
}

void updateAlarm() {
  int touchValue = touchRead(TOUCH_PIN);
  if (armed && touchValue < touchThreshold) {
    alarmActive = true;
  }

  if (!alarmActive) {
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousBlinkMillis >= ALARM_INTERVAL_MS) {
    previousBlinkMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  int baseline = readTouchAverage(CALIBRATION_SAMPLES);
  touchThreshold = baseline - TOUCH_MARGIN;
  if (touchThreshold < 5) {
    touchThreshold = 5;
  }

  Serial.print("Touch baseline: ");
  Serial.println(baseline);
  Serial.print("Touch threshold: ");
  Serial.println(touchThreshold);

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();
  updateAlarm();
}
