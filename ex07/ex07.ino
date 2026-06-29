// 作业5-ex07：Web网页端无极调光器
// 默认接线：LED -> GPIO2
// 烧录后连接 WiFi 热点 ESP32-Dimmer，浏览器打开 http://192.168.4.1

#include <WiFi.h>
#include <WebServer.h>

const char *AP_SSID = "ESP32-Dimmer";
const char *AP_PASSWORD = "12345678";

const int LED_PIN = 2;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

WebServer server(80);
int brightness = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 无极调光器</title>
  <style>
    body { margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; font-family: Arial, "Microsoft YaHei", sans-serif; background: #f5f7fb; color: #1f2937; }
    main { width: min(90vw, 420px); text-align: center; }
    h1 { font-size: 28px; margin-bottom: 24px; }
    input { width: 100%; }
    .value { margin-top: 18px; font-size: 44px; font-weight: 700; }
  </style>
</head>
<body>
  <main>
    <h1>ESP32 无极调光器</h1>
    <input id="slider" type="range" min="0" max="255" value="0">
    <div id="value" class="value">0</div>
  </main>
  <script>
    const slider = document.getElementById('slider');
    const value = document.getElementById('value');
    slider.addEventListener('input', () => {
      value.textContent = slider.value;
      fetch('/set?value=' + slider.value).catch(() => {});
    });
  </script>
</body>
</html>
)rawliteral";

void setupPwm() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
#else
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
#endif
}

void writePwm(int value) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(LED_PIN, value);
#else
  ledcWrite(PWM_CHANNEL, value);
#endif
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleSetBrightness() {
  if (server.hasArg("value")) {
    brightness = constrain(server.arg("value").toInt(), 0, 255);
    writePwm(brightness);
  }
  server.send(200, "text/plain", String(brightness));
}

void setup() {
  Serial.begin(115200);
  setupPwm();
  writePwm(0);

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/set", handleSetBrightness);
  server.begin();
}

void loop() {
  server.handleClient();
}
