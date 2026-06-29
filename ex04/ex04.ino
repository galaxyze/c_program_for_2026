// 作业4：基于触摸传感器的自锁开关
// 默认接线：LED -> GPIO2，触摸引脚 -> GPIO4(T0)

const int LED_PIN = 2;
const int TOUCH_PIN = 4;

const int CALIBRATION_SAMPLES = 40;
const int TOUCH_MARGIN = 18;
const unsigned long DEBOUNCE_MS = 250;

int touchThreshold = 0;
bool ledState = false;
bool lastTouched = false;
unsigned long lastToggleTime = 0;

int readTouchAverage(int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += touchRead(TOUCH_PIN);
    delay(8);
  }
  return total / samples;
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
}

void loop() {
  int touchValue = touchRead(TOUCH_PIN);
  bool touched = touchValue < touchThreshold;
  unsigned long now = millis();

  if (touched && !lastTouched && now - lastToggleTime > DEBOUNCE_MS) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastToggleTime = now;

    Serial.print("LED state: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  lastTouched = touched;
  delay(20);
}
