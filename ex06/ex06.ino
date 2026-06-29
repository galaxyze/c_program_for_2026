// 作业4-ex06：警车双闪灯效（双通道PWM）
// 默认接线：LED_A -> GPIO2，LED_B -> GPIO16

const int LED_A_PIN = 2;
const int LED_B_PIN = 16;

const int LED_A_CHANNEL = 0;
const int LED_B_CHANNEL = 1;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

int brightness = 0;
int direction = 1;
unsigned long previousMillis = 0;
const unsigned long intervalMs = 8;

void setupPwmPin(int pin, int channel) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, PWM_FREQ, PWM_RESOLUTION);
#else
  ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(pin, channel);
#endif
}

void writePwmPin(int pin, int channel, int value) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, value);
#else
  ledcWrite(channel, value);
#endif
}

void setup() {
  setupPwmPin(LED_A_PIN, LED_A_CHANNEL);
  setupPwmPin(LED_B_PIN, LED_B_CHANNEL);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < intervalMs) {
    return;
  }
  previousMillis = currentMillis;

  brightness += direction;
  if (brightness >= 255) {
    brightness = 255;
    direction = -1;
  } else if (brightness <= 0) {
    brightness = 0;
    direction = 1;
  }

  writePwmPin(LED_A_PIN, LED_A_CHANNEL, brightness);
  writePwmPin(LED_B_PIN, LED_B_CHANNEL, 255 - brightness);
}
