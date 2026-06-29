// 作业5：多档位触摸调速呼吸灯
// 默认接线：LED -> GPIO2，触摸引脚 -> GPIO4(T0)

const int LED_PIN = 2;
const int TOUCH_PIN = 4;

const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

const int CALIBRATION_SAMPLES = 40;
const int TOUCH_MARGIN = 18;
const unsigned long DEBOUNCE_MS = 250;

int touchThreshold = 0;
bool lastTouched = false;
unsigned long lastTouchTime = 0;

int speedLevel = 1;
int brightness = 0;
int direction = 1;
unsigned long lastBreathTime = 0;

int readTouchAverage(int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += touchRead(TOUCH_PIN);
    delay(8);
  }
  return total / samples;
}

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

unsigned long breathIntervalMs() {
  if (speedLevel == 1) {
    return 18;  // 慢速呼吸
  }
  if (speedLevel == 2) {
    return 9;   // 中速呼吸
  }
  return 4;     // 快速呼吸
}

void handleTouch() {
  int touchValue = touchRead(TOUCH_PIN);
  bool touched = touchValue < touchThreshold;
  unsigned long now = millis();

  if (touched && !lastTouched && now - lastTouchTime > DEBOUNCE_MS) {
    speedLevel++;
    if (speedLevel > 3) {
      speedLevel = 1;
    }
    lastTouchTime = now;

    Serial.print("Speed level: ");
    Serial.println(speedLevel);
  }

  lastTouched = touched;
}

void updateBreathingLed() {
  unsigned long now = millis();
  if (now - lastBreathTime < breathIntervalMs()) {
    return;
  }
  lastBreathTime = now;

  brightness += direction;
  if (brightness >= 255) {
    brightness = 255;
    direction = -1;
  } else if (brightness <= 0) {
    brightness = 0;
    direction = 1;
  }

  writePwm(brightness);
}

void setup() {
  Serial.begin(115200);
  setupPwm();
  writePwm(0);

  int baseline = readTouchAverage(CALIBRATION_SAMPLES);
  touchThreshold = baseline - TOUCH_MARGIN;
  if (touchThreshold < 5) {
    touchThreshold = 5;
  }

  Serial.print("Touch baseline: ");
  Serial.println(baseline);
  Serial.print("Touch threshold: ");
  Serial.println(touchThreshold);
  Serial.println("Speed level: 1");
}

void loop() {
  handleTouch();
  updateBreathingLed();
}
