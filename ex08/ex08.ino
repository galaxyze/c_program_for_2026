/*
 * ex08：物联网安防报警器模拟实验
 *
 * 硬件连接：
 *   - 报警 LED：GPIO2。
 *   - 触摸输入：GPIO4，也就是 ESP32 的 T0 触摸通道。
 *
 * 网络使用：
 *   - ESP32 创建 WiFi 热点：ESP32-Alarm。
 *   - 热点密码：12345678。
 *   - 手机或电脑连接该热点后，浏览器打开：http://192.168.4.1。
 *
 * 正确实验逻辑：
 *   1. 未布防时，触摸 GPIO4 没有反应，LED 保持熄灭。
 *   2. 点击网页“布防 Arm”后，只进入监视状态，LED 仍然不闪烁。
 *   3. 布防之后，必须出现一次新的触摸动作，才会触发报警。
 *   4. 报警触发后，LED 高频闪烁并锁定，即使手松开也不会停止。
 *   5. 只有点击网页“撤防 Disarm”，LED 才熄灭，报警状态才复位。
 *
 * 关键变量：
 *   - armed：是否已经布防。
 *   - alarmActive：是否已经进入报警锁定状态。
 *   - lastTouched：上一轮是否处于触摸状态，用来识别“刚触摸”的瞬间。
 */

#include <WiFi.h>       // 引入 ESP32 WiFi 库，用来创建无线热点。
#include <WebServer.h>  // 引入 WebServer 库，用来搭建 HTTP 网页服务器。

const char *AP_SSID = "ESP32-Alarm";     // ESP32 创建的 WiFi 热点名称。
const char *AP_PASSWORD = "12345678";    // ESP32 热点密码，长度至少 8 位。

const int LED_PIN = 2;    // 报警 LED 引脚，默认使用 ESP32 板载 LED GPIO2。
const int TOUCH_PIN = 4;  // 触摸引脚，GPIO4 对应 ESP32 的 T0 触摸通道。

const int CALIBRATION_SAMPLES = 40;            // 上电时读取 40 次触摸值，用平均值计算环境基线。
const int TOUCH_MARGIN = 18;                   // 触摸阈值余量；触摸时读数通常变小，所以阈值 = 基线 - 余量。
const unsigned long ALARM_INTERVAL_MS = 80;    // 报警闪烁间隔，每 80ms 翻转一次 LED，形成高频闪烁。

WebServer server(80);  // 创建 Web 服务器对象，监听 HTTP 默认端口 80。

int touchThreshold = 0;             // 自动计算出的触摸阈值，小于该值才认为被触摸。
bool armed = false;                 // 系统是否布防；false 表示未布防，true 表示已布防。
bool alarmActive = false;           // 报警是否已经触发；触发后保持 true，直到网页撤防。
bool ledState = false;              // LED 当前状态；报警闪烁时会不断翻转。
bool lastTouched = false;           // 上一轮触摸状态；用于检测从“未触摸”到“触摸”的边沿。
unsigned long previousBlinkMillis = 0;  // 上一次 LED 闪烁翻转的时间。

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

int readTouchAverage(int samples) {  // 多次读取触摸值并返回平均值，让阈值更稳定。
  long total = 0;                    // 保存所有采样值之和。
  for (int i = 0; i < samples; i++) { // 按指定次数循环采样。
    total += touchRead(TOUCH_PIN);    // 读取 GPIO4 触摸值并累加。
    delay(8);                         // 每次采样后稍等 8ms，让读数更平稳。
  }
  return total / samples;             // 返回平均值，作为未触摸环境基线。
}

bool isTouched() {                    // 把触摸模拟值转换成 true/false 判断。
  int touchValue = touchRead(TOUCH_PIN); // 读取当前触摸值。
  return touchValue < touchThreshold;    // 低于阈值说明被触摸；否则说明未触摸。
}

void handleRoot() {  // 处理浏览器访问首页 / 的请求。
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);  // 返回网页 HTML。
}

void handleArm() {          // 处理网页点击“布防 Arm”后的 /arm 请求。
  armed = true;             // 进入布防状态，只表示开始监视，不表示已经报警。
  alarmActive = false;      // 布防时确保报警锁定状态为 false。
  ledState = false;         // 重置 LED 状态变量。
  lastTouched = isTouched(); // 记录点击布防这一瞬间的触摸状态，防止布防按钮刚点下就误报警。
  digitalWrite(LED_PIN, LOW); // 布防后但未触发报警时，LED 必须保持熄灭。
  server.send(200, "text/plain", "armed"); // 返回布防成功。
}

void handleDisarm() {       // 处理网页点击“撤防 Disarm”后的 /disarm 请求。
  armed = false;            // 退出布防状态。
  alarmActive = false;      // 清除报警锁定状态。
  ledState = false;         // 重置 LED 状态变量。
  lastTouched = false;      // 清除上一轮触摸记录，下一次布防重新判断。
  digitalWrite(LED_PIN, LOW); // 撤防后立即熄灭 LED。
  server.send(200, "text/plain", "disarmed"); // 返回撤防成功。
}

void handleStatus() {       // 处理网页轮询 /status 的请求。
  String json = "{";       // JSON 对象开始。
  json += "\"armed\":"; // 添加 armed 字段名。
  json += armed ? "true" : "false"; // 添加 armed 当前值。
  json += ",\"alarm\":"; // 添加 alarm 字段名。
  json += alarmActive ? "true" : "false"; // 添加 alarm 当前值。
  json += ",\"touch\":"; // 添加 touch 字段名。
  json += touchRead(TOUCH_PIN); // 返回当前触摸原始读数，便于观察调试。
  json += "}";            // JSON 对象结束。
  server.send(200, "application/json", json); // 把状态返回给网页。
}

void updateAlarm() {  // 按“未布防 -> 已布防等待新触摸 -> 报警锁定”更新报警状态。
  if (!armed && !alarmActive) { // 未布防且未报警：系统待机。
    digitalWrite(LED_PIN, LOW); // 未布防时触摸无反应，LED 保持熄灭。
    return;                     // 不检测触摸触发报警，直接返回。
  }

  if (armed && !alarmActive) {  // 已布防但还没报警：等待新的触摸动作。
    bool touched = isTouched(); // 读取当前是否触摸。

    if (touched && !lastTouched) { // 只有从“未触摸”变成“触摸”的瞬间才触发报警。
      alarmActive = true;          // 锁定报警状态，手松开也不会自动解除。
      previousBlinkMillis = millis(); // 记录报警开始时间。
      ledState = true;             // 报警刚触发时先让 LED 亮起。
      digitalWrite(LED_PIN, HIGH); // 立即点亮 LED，给出报警反馈。
    }

    lastTouched = touched;         // 保存本轮触摸状态，供下一轮判断边沿。
    return;                        // 已布防但未报警时，不执行闪烁逻辑。
  }

  if (alarmActive) {               // 报警已经触发并锁定。
    unsigned long currentMillis = millis(); // 获取当前系统时间。
    if (currentMillis - previousBlinkMillis >= ALARM_INTERVAL_MS) { // 到达闪烁间隔。
      previousBlinkMillis = currentMillis; // 更新时间戳。
      ledState = !ledState;        // 翻转 LED 状态，形成高频闪烁。
      digitalWrite(LED_PIN, ledState ? HIGH : LOW); // 输出新的 LED 状态。
    }
  }
}

void setup() {                     // ESP32 上电或复位后执行一次。
  Serial.begin(115200);            // 初始化串口，便于查看调试信息。
  pinMode(LED_PIN, OUTPUT);        // 把 LED 引脚设置为输出模式。
  digitalWrite(LED_PIN, LOW);      // 启动时 LED 熄灭。

  int baseline = readTouchAverage(CALIBRATION_SAMPLES); // 读取未触摸基线。
  touchThreshold = baseline - TOUCH_MARGIN;             // 根据基线计算触摸阈值。
  if (touchThreshold < 5) {        // 防止阈值过低。
    touchThreshold = 5;            // 设置最小阈值。
  }

  Serial.print("Touch baseline: "); // 打印触摸基线提示。
  Serial.println(baseline);          // 打印触摸基线值。
  Serial.print("Touch threshold: "); // 打印触摸阈值提示。
  Serial.println(touchThreshold);    // 打印触摸阈值。

  WiFi.softAP(AP_SSID, AP_PASSWORD); // 创建 ESP32 热点。
  Serial.print("AP IP address: ");   // 打印 IP 提示。
  Serial.println(WiFi.softAPIP());    // 通常输出 192.168.4.1。

  server.on("/", handleRoot);        // 注册首页路由。
  server.on("/arm", handleArm);      // 注册布防路由。
  server.on("/disarm", handleDisarm); // 注册撤防路由。
  server.on("/status", handleStatus); // 注册状态查询路由。
  server.begin();                     // 启动 Web 服务器。
}

void loop() {             // 主循环不断执行。
  server.handleClient();  // 处理网页请求。
  updateAlarm();          // 更新安防状态和报警 LED。
}
