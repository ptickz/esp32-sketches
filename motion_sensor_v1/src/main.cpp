#include <Arduino.h>

constexpr uint8_t RED_LED_PIN = 16;   // RX2
constexpr uint8_t GREEN_LED_PIN = 17; // TX2
constexpr uint8_t MOTION_SENSOR_PIN = 27;

unsigned long now;
volatile unsigned long lastTrigger = 0;
volatile bool startTimer = false;

bool printMotion = false;

const unsigned long timeSeconds = 5 * 1000UL;

void ARDUINO_ISR_ATTR motionISR()
{
  lastTrigger = millis();
  startTimer = true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(MOTION_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(MOTION_SENSOR_PIN, motionISR, RISING);

  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);

  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void loop()
{
  now = millis();

  if (startTimer && !printMotion)
  {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("MOTION DETECTED");
    printMotion = true;
  }

  if (startTimer && (now - lastTrigger > timeSeconds))
  {
    Serial.println("MOTION STOPPED");
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    startTimer = false;
    printMotion = false;
  }
}
