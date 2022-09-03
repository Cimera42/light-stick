# Light-stick

Custom version of a light painting stick. Loads image files from an SD card and displays using a string of 144 WS2812b RGB LEDs.

Currently built to run on the Firebeetle-ESP32 & OLED screen I/O shield.

| Table of Contents |
| ----------------- |
| Getting started   |
| Wiring            |
| Todo              |

### Getting started

1. Download repository
2. Install [PlatformIO](https://platformio.org/)
3. Build and upload to microcontroller

### Wiring

| ESP32 | SD card          | USB  | WS2812 |
| ----- | ---------------- | ---- | ------ |
| GND   | GND              | GND  | GND    |
| VCC   |                  | VBUS | +VCC   |
| 3v3   | 3v               |      |        |
| D2    | CS               |      |        |
| D4    |                  |      | Data   |
| MISO  | ~~DO~~/~~DI~~ ?? |      |        |
| MOSI  | ~~DI~~/~~DO~~ ?? |      |        |
| SCK   | CLK              |      |        |

### Todo

List of things to be done at some point.

| Todo                           | Priority |
| ------------------------------ | -------- |
| Brightness control             | H        |
| Speed control                  | H        |
| Image file rotation correction | M        |
| Directory support              | L        |
| Status screen                  | L        |
