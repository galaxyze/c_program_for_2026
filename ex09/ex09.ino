// 作业5-ex09：实时传感器Web仪表盘
// 默认接线：触摸引脚 -> GPIO4(T0)
// 烧录后连接 WiFi 热点 ESP32-Touch-Dashboard，浏览器打开 http://192.168.4.1

#include <WiFi.h>
#include <WebServer.h>

const char *AP_SSID = "ESP32-Touch-Dashboard";
const char *AP_PASSWORD = "12345678";

const int TOUCH_PIN = 4;

WebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 触摸传感器仪表盘</title>
  <style>
    body { margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; font-family: Arial, "Microsoft YaHei", sans-serif; background: #f4f6f8; color: #111827; }
    main { width: min(90vw, 460px); text-align: center; }
    h1 { font-size: 26px; margin-bottom: 18px; }
    .number { font-size: 76px; font-weight: 800; line-height: 1.1; }
    .label { margin-top: 12px; font-size: 18px; color: #4b5563; }
  </style>
</head>
<body>
  <main>
    <h1>ESP32 实时触摸传感器</h1>
    <div id="value" class="number">--</div>
    <div class="label">手靠近触摸引脚时，数值会变小</div>
  </main>
  <script>
    const valueBox = document.getElementById('value');
    function refresh() {
      fetch('/value')
        .then(r => r.text())
        .then(v => valueBox.textContent = v)
        .catch(() => {});
    }
    refresh();
    setInterval(refresh, 200);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleValue() {
  server.send(200, "text/plain", String(touchRead(TOUCH_PIN)));
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/value", handleValue);
  server.begin();
}

void loop() {
  server.handleClient();
}
