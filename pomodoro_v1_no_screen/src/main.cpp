#include <Arduino.h>

// ---------- Пины ----------
constexpr uint8_t PIN_RGB_R = 16; // RX2, через резистор 220 Ом
constexpr uint8_t PIN_RGB_G = 17; // TX2, через резистор 220 Ом
constexpr uint8_t PIN_RGB_B = 21; // D21, через резистор 220 Ом
constexpr uint8_t PIN_BTN_START = 18;
constexpr uint8_t PIN_BTN_RESET = 19;
constexpr uint8_t PIN_BUZZER = 23;
constexpr uint8_t PIN_POT = 4;

// ---------- Настройки ----------
constexpr bool COMMON_ANODE = false; // true, если у RGB общий плюс
constexpr bool DEBUG = false;        // в отладке «минута» = 1 секунда
constexpr unsigned long MINUTE_MS = DEBUG ? 1000UL : 60000UL;
constexpr unsigned long BREAK_MS = 5 * MINUTE_MS;       // короткий перерыв
constexpr unsigned long LONG_BREAK_MS = 15 * MINUTE_MS; // длинный перерыв
constexpr int POMODOROS_BEFORE_LONG = 4;                // длинный после каждого 4-го
constexpr unsigned long BLINK_MS = 1000;
constexpr int POT_DEADBAND = 100;

enum class State
{
  Idle,
  Work,
  Break,
  Paused
};
enum class Length : uint8_t
{
  M20 = 20,
  M25 = 25,
  M30 = 30
};

State state = State::Idle;
State pausedFrom = State::Idle;
Length chosen = Length::M25;
unsigned long phaseStart = 0;
unsigned long phaseTotal = 0;
unsigned long pausedElapsed = 0;
int lastPot = 0;
int completed = 0;

// ---------- Кнопки с антидребезгом ----------
struct Button
{
  uint8_t pin;
  bool lastRaw;
  bool stable;
  unsigned long changedAt;
};
Button btnStart{PIN_BTN_START, false, false, 0};
Button btnReset{PIN_BTN_RESET, false, false, 0};

// true только в момент нажатия
bool justPressed(Button &b, unsigned long now)
{
  bool raw = digitalRead(b.pin) == LOW; // кнопка на GND
  if (raw != b.lastRaw)
  {
    b.lastRaw = raw;
    b.changedAt = now;
  }
  if (now - b.changedAt >= 30 && raw != b.stable)
  {
    b.stable = raw;
    return raw;
  }
  return false;
}

// ---------- Пищалка без delay() ----------
struct Beeper
{
  uint8_t left;
  uint16_t onMs;
  bool on;
  unsigned long next;
};
Beeper beeper{0, 0, false, 0};

void beepPattern(uint8_t count, uint16_t onMs)
{
  beeper = {count, onMs, false, millis()};
}

void updateBeeper(unsigned long now)
{
  if (beeper.left == 0 && !beeper.on)
    return;
  if (now < beeper.next)
    return;
  if (beeper.on)
  {
    digitalWrite(PIN_BUZZER, LOW);
    beeper.on = false;
    beeper.left--;
    beeper.next = now + 150;
  }
  else
  {
    digitalWrite(PIN_BUZZER, HIGH);
    beeper.on = true;
    beeper.next = now + beeper.onMs;
  }
}

// ---------- Вспомогательное ----------
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
  case State::Paused:
    return "PAUSE";
  }
  return "?";
}

void setState(State next)
{
  Serial.printf("%s -> %s\n", stateName(state), stateName(next));
  state = next;
}

// ---------- Потенциометр ----------
Length lengthFromRaw(int raw)
{
  if (raw < 1300)
    return Length::M20;
  if (raw <= 2600)
    return Length::M25;
  return Length::M30;
}

void readPot()
{
  int raw = analogRead(PIN_POT);
  if (abs(raw - lastPot) <= POT_DEADBAND)
    return; // игнорируем дрожание
  lastPot = raw;
  if (state == State::Idle)
  { // меняем только без таймера
    chosen = lengthFromRaw(raw);
    Serial.printf("Длина: %u мин\n", (unsigned)chosen);
  }
}

// ---------- RGB ----------
void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
  if (COMMON_ANODE)
  {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(PIN_RGB_R, r);
  analogWrite(PIN_RGB_G, g);
  analogWrite(PIN_RGB_B, b);
}

float progress(unsigned long now)
{
  if (phaseTotal == 0)
    return 0;
  unsigned long e = now - phaseStart;
  if (e >= phaseTotal)
    return 1.0f;
  return (float)e / phaseTotal;
}

void updateRGB(unsigned long now)
{
  switch (state)
  {
  case State::Idle:
    setRGB(0, 0, 255); // синий
    break;
  case State::Work:
    setRGB(255, 0, 0); // красный
    break;
  case State::Break:
    setRGB(0, 255, 0); // зелёный
    break;
  case State::Paused:
    if ((now / BLINK_MS) % 2 == 0)
      setRGB(255, 255, 0); // жёлтый 1 с
    else
      setRGB(0, 0, 0); // выкл 1 с
    break;
  }
}

// ---------- Переходы фаз ----------
void startWork(unsigned long now)
{
  phaseStart = now;
  phaseTotal = (unsigned long)chosen * MINUTE_MS;
  setState(State::Work);
  beepPattern(1, 100);
}

void startBreak(unsigned long now)
{
  phaseStart = now;
  bool isLong = (completed % POMODOROS_BEFORE_LONG) == 0;
  phaseTotal = isLong ? LONG_BREAK_MS : BREAK_MS;
  setState(State::Break);
  Serial.printf("%s перерыв\n", isLong ? "Длинный" : "Короткий");
  beepPattern(2, 100);
}

void onStartPressed(unsigned long now)
{
  switch (state)
  {
  case State::Idle:
    startWork(now);
    break;
  case State::Work:
  case State::Break:
    pausedFrom = state;
    pausedElapsed = now - phaseStart;
    setState(State::Paused);
    beepPattern(1, 600); // длинный писк
    break;
  case State::Paused:
    phaseStart = now - pausedElapsed; // продолжаем с той же отметки
    setState(pausedFrom);
    beepPattern(1, 100);
    break;
  }
}

void onResetPressed()
{
  if (state == State::Idle)
  {
    chosen = Length::M25;
    Serial.println("Длина сброшена: 25 мин");
    return;
  }
  setState(State::Idle);
}

void checkPhaseEnd(unsigned long now)
{
  if (state != State::Work && state != State::Break)
    return;
  if (now - phaseStart < phaseTotal)
    return;
  if (state == State::Work)
  {
    completed++;
    startBreak(now);
  }
  else
  {
    startWork(now);
  }
}

void printStatus(unsigned long now)
{
  static unsigned long lastPrint = 0;
  if (!DEBUG || now - lastPrint < 1000)
    return;
  lastPrint = now;
  if (state == State::Idle)
  {
    Serial.printf("IDLE, длина %u мин, помидоров: %d\n", (unsigned)chosen, completed);
    return;
  }
  unsigned long elapsed = (state == State::Paused) ? pausedElapsed : (now - phaseStart);
  unsigned long left = phaseTotal > elapsed ? (phaseTotal - elapsed) / 1000 : 0;
  Serial.printf("%s, осталось %lu с, помидоров: %d\n", stateName(state), left, completed);
}

// ---------- Главный цикл ----------
void setup()
{
  Serial.begin(115200);
  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BTN_START, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);

  lastPot = analogRead(PIN_POT);
  chosen = lengthFromRaw(lastPot);
  Serial.println("Помидор-таймер готов");
}

void loop()
{
  unsigned long now = millis();

  readPot();
  if (justPressed(btnStart, now))
    onStartPressed(now);
  if (justPressed(btnReset, now))
    onResetPressed();
  checkPhaseEnd(now);
  updateBeeper(now);
  updateRGB(now);
  printStatus(now);
}