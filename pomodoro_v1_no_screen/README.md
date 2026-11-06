# pomodoro_v1_no_screen

First version of the pomodoro timer, no display: the phase is shown by an RGB LED colour. Fully non-blocking: the buzzer runs on `millis()` instead of `delay()`.

## Behaviour

- **Potentiometer** (while idle) picks work length: 20 / 25 / 30 min.
- **Start button**: start, then pause / resume.
- **Reset button**: back to idle; in idle it resets length to 25 min.
- Breaks: 5 min, every 4th break is 15 min.
- RGB: blue = idle, red = work, green = break, blinking yellow = pause.

## Wiring

| Part | ESP32 pin |
|------|-----------|
| RGB LED R / G / B (220 Ω each) | GPIO 16 / 17 / 21 |
| Start button (to GND) | GPIO 18 |
| Reset button (to GND) | GPIO 19 |
| Buzzer | GPIO 23 |
| Potentiometer (wiper) | GPIO 4 |

Common-cathode LED by default; set `COMMON_ANODE = true` for common-anode. `DEBUG = true` makes a "minute" one second long and prints status every second.
