# motion_sensor_v1

PIR motion detector. The sensor triggers a hardware interrupt; the red LED stays on until 5 seconds pass without new motion, then the green LED comes back.

## Wiring

| Part | ESP32 pin |
|------|-----------|
| PIR sensor OUT | GPIO 27 |
| Red LED (motion) | GPIO 16 |
| Green LED (idle) | GPIO 17 |

Serial prints `MOTION DETECTED` / `MOTION STOPPED`.
