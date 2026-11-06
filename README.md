# esp32-sketches

Small hardware projects on an ESP32 DevKit (ESP32-WROOM-32), built with [PlatformIO](https://platformio.org/) and the Arduino framework.

| Project | What it does | Parts |
|---------|--------------|-------|
| [pomodoro](pomodoro/) | Pomodoro timer with OLED display, progress bar and pause | SSD1306 OLED, 2 LEDs, 2 buttons, potentiometer, buzzer |
| [pomodoro_v1_no_screen](pomodoro_v1_no_screen/) | First version of the timer: phase shown by RGB LED colour | RGB LED, 2 buttons, potentiometer, buzzer |
| [motion_sensor_v1](motion_sensor_v1/) | PIR motion detector with interrupt and hold timer | PIR sensor, 2 LEDs |
| [thermometer-basic-v.01](thermometer-basic-v.01/) | Temperature and humidity readings over serial | DHT11 |

## Layout

Each folder is a standalone PlatformIO project. Board, upload port and serial monitor settings live in one place, [`common.ini`](common.ini), and every project pulls them in:

```ini
[platformio]
extra_configs = ../common.ini

[env:esp32dev]
extends = esp32_common
```

If the board appears on a different port, change `upload_port` / `monitor_port` in `common.ini` once.

## Build and flash

```sh
pio run -d pomodoro                 # build
pio run -d pomodoro -t upload       # flash
pio device monitor -d pomodoro      # serial monitor, 115200 baud
```

Or open a single project folder in VS Code with the PlatformIO extension.

## License

[MIT](LICENSE)
