# pomodoro

Pomodoro timer with a 128×64 OLED. Shows the current phase, time left, number of finished pomodoros and a progress bar.

## Behaviour

- **Potentiometer** (while idle) picks work length: 20 / 25 / 30 min.
- **White button**: start, then pause / resume. In pause the time blinks.
- **Red button**: reset to idle and clear the counter.
- Breaks: 5 min, every 4th break is 15 min.
- LEDs: red = work, green = break, both = pause. Buzzer: 1 beep work, 2 beeps break, long beep pause.

## Wiring

| Part | ESP32 pin |
|------|-----------|
| SSD1306 OLED (I²C, 0x3C) | SDA 21, SCL 22 |
| Red LED | GPIO 16 |
| Green LED | GPIO 17 |
| White button (to GND) | GPIO 18 |
| Red button (to GND) | GPIO 19 |
| Buzzer | GPIO 23 |
| Potentiometer (wiper) | GPIO 34 |

Buttons use the internal pull-up. Set `DEBUG` in `src/main.cpp` to `false` to stop the heartbeat log on serial.
