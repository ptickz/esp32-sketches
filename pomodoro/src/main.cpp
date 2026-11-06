#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

constexpr uint8_t RED_LED_PIN = 16;       // RX2
constexpr uint8_t GREEN_LED_PIN = 17;     // TX2
constexpr uint8_t POTENTIOMETER_PIN = 34; // D34
constexpr uint8_t BUZZER_PIN = 23;        // D23
constexpr uint8_t WHITE_BUTTON_PIN = 18;  // D18
constexpr uint8_t RED_BUTTON_PIN = 19;    // D19

constexpr bool DEBUG = true;
constexpr unsigned long MINUTE_MS = 60UL * 1000;
// constexpr unsigned long MINUTE_MS = 1000;

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr unsigned long SHORT_BREAK_MS = 5UL * MINUTE_MS;
constexpr unsigned long LONG_BREAK_MS = 15UL * MINUTE_MS;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 40;
constexpr uint8_t POMODOROS_BEFORE_LONG_BREAK = 4;

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr unsigned long RENDER_INTERVAL_MS = 200;

enum class State
{
  Idle,
  Work,
  Break,
  Pause
};

State state = State::Idle;
State pausedFrom = State::Work;

unsigned long phaseEnd = 0;
unsigned long phaseDuration = 0;
unsigned long remaining = 0;
unsigned long workMs = 25UL * MINUTE_MS;
uint8_t completedPomodoros = 0;

unsigned long lastPrint = 0;
unsigned long lastRender = 0;

struct Button
{
  uint8_t pin;
  bool stableState;
  bool lastReading;
  unsigned long lastChangeMs;
};

Button whiteButton = {WHITE_BUTTON_PIN, HIGH, HIGH, 0};
Button redButton = {RED_BUTTON_PIN, HIGH, HIGH, 0};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool displayOk = false;

void updateDurationFromPot();
unsigned long nextBreakMs();
void startPhase(unsigned long now, unsigned long duration);
const char *stateName(State s);
void setState(State next);
bool wasPressed(Button &b, unsigned long now);
void setLeds(bool red, bool green);
void beep(unsigned long ms);
void beeps(uint8_t count);
void render(unsigned long now);
void debugPrint(unsigned long now);

void setup()
{
  Serial.begin(SERIAL_BAUD);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(WHITE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  displayOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!displayOk)
  {
    Serial.println("Display not found");
  }

  Serial.println("BOOT OK");
}

void loop()
{
  unsigned long now = millis();

  if (DEBUG)
  {
    debugPrint(now);
  }

  bool start = wasPressed(whiteButton, now);
  bool reset = wasPressed(redButton, now);

  if (reset && state != State::Idle)
  {
    completedPomodoros = 0;
    setState(State::Idle);
    setLeds(false, false);
  }

  switch (state)
  {
  case State::Idle:
    updateDurationFromPot();
    if (start)
    {
      startPhase(now, workMs);
      setState(State::Work);
      setLeds(true, false);
      beeps(1);
    }
    break;

  case State::Work:
  case State::Break:
    if (start)
    {
      pausedFrom = state;
      remaining = phaseEnd - now;
      setState(State::Pause);
      setLeds(true, true);
      beep(500);
    }
    else if ((long)(now - phaseEnd) >= 0)
    {
      if (state == State::Work)
      {
        completedPomodoros++;
        startPhase(now, nextBreakMs());
        setState(State::Break);
        setLeds(false, true);
        beeps(2);
      }
      else
      {
        startPhase(now, workMs);
        setState(State::Work);
        setLeds(true, false);
        beeps(1);
      }
    }
    break;

  case State::Pause:
    if (start)
    {
      phaseEnd = now + remaining;
      setState(pausedFrom);
      if (state == State::Work)
      {
        setLeds(true, false);
        beeps(1);
      }
      else
      {
        setLeds(false, true);
        beeps(2);
      }
    }
    break;
  }

  if ((state == State::Work || state == State::Break) && now - lastPrint >= 1000)
  {
    lastPrint = now;
    unsigned long left = (phaseEnd - now) / 1000;
    Serial.printf("%s %02lu:%02lu (#%u)\n", stateName(state), left / 60, left % 60, completedPomodoros);
  }

  if (displayOk && now - lastRender >= RENDER_INTERVAL_MS)
  {
    lastRender = now;
    render(now);
  }
}

unsigned long nextBreakMs()
{
  return (completedPomodoros % POMODOROS_BEFORE_LONG_BREAK == 0) ? LONG_BREAK_MS : SHORT_BREAK_MS;
}

void startPhase(unsigned long now, unsigned long duration)
{
  phaseDuration = duration;
  phaseEnd = now + duration;
}

void updateDurationFromPot()
{
  uint16_t val = analogRead(POTENTIOMETER_PIN);
  if (val < 1365)
  {
    workMs = 20UL * MINUTE_MS;
  }
  else if (val < 2730)
  {
    workMs = 25UL * MINUTE_MS;
  }
  else
  {
    workMs = 30UL * MINUTE_MS;
  }
}

bool wasPressed(Button &b, unsigned long now)
{
  bool reading = digitalRead(b.pin);

  if (reading != b.lastReading)
  {
    b.lastChangeMs = now;
    b.lastReading = reading;
  }

  if (now - b.lastChangeMs >= BUTTON_DEBOUNCE_MS && reading != b.stableState)
  {
    b.stableState = reading;
    return b.stableState == LOW;
  }
  return false;
}

const char *stateName(State s)
{
  switch (s)
  {
  case State::Idle:
    return "IDLE";
  case State::Work:
    return "WORK";
  case State::Break:
    return "BREAK";
  case State::Pause:
    return "PAUSE";
  }
  return "UNKNOWN";
}

void setState(State next)
{
  Serial.printf("%s -> %s\n", stateName(state), stateName(next));
  state = next;
}

void debugPrint(unsigned long now)
{
  static unsigned long lastBeat = 0;
  if (now - lastBeat >= 2000)
  {
    lastBeat = now;
    Serial.printf("alive, state=%s, white=%d, red=%d, pot=%u, work=%lu, display=%s\n",
                  stateName(state),
                  digitalRead(WHITE_BUTTON_PIN),
                  digitalRead(RED_BUTTON_PIN),
                  analogRead(POTENTIOMETER_PIN),
                  workMs / MINUTE_MS,
                  displayOk ? "ok" : "none");
  }
}

void setLeds(bool red, bool green)
{
  digitalWrite(RED_LED_PIN, red);
  digitalWrite(GREEN_LED_PIN, green);
}

void beep(unsigned long ms)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

void beeps(uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    beep(100);
    if (i + 1 < count)
      delay(100);
  }
}

void render(unsigned long now)
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 4);
  display.print(stateName(state));
  display.setCursor(32, 4);
  display.print("RatJobTimer");
  display.setCursor(104, 4);
  display.printf("#%u", completedPomodoros);

  unsigned long leftMs;
  switch (state)
  {
  case State::Idle:
    leftMs = workMs;
    break;
  case State::Pause:
    leftMs = remaining;
    break;
  default:
    leftMs = phaseEnd - now;
    break;
  }
  unsigned long left = (leftMs + 999) / 1000;

  bool blinkOff = (state == State::Pause) && ((now / 500) % 2);
  if (!blinkOff)
  {
    display.setTextSize(3);
    display.setCursor(19, 22);
    display.printf("%02lu:%02lu", left / 60, left % 60);
  }

  if (state != State::Idle && phaseDuration > 0)
  {
    unsigned long elapsed = phaseDuration - leftMs;
    int16_t w = (elapsed * 124UL) / phaseDuration;
    display.drawRect(0, 54, 128, 10, SSD1306_WHITE);
    display.fillRect(2, 56, w, 6, SSD1306_WHITE);
  }

  display.display();
}