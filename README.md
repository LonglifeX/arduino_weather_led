# Arduino Weather LED

Displays live weather data on an **Arduino UNO R4 WiFi** built-in 12×8 LED matrix.
Every 15 minutes the board fetches current conditions from the [Open-Meteo](https://open-meteo.com/) API (no API key required) and scrolls temperature, humidity, and wind speed across the matrix, followed by an animated rain-cloud.

## Hardware

| Component | Notes |
|-----------|-------|
| Arduino UNO R4 WiFi | Built-in LED matrix and WiFi module |

## Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- WiFi network with internet access

## Setup

### 1. WiFi credentials

Copy the example file and fill in your network details:

```
cp include/arduino_secrets.h.example include/arduino_secrets.h
```

Edit `include/arduino_secrets.h`:

```cpp
#define SECRET_SSID "your_wifi_ssid"
#define SECRET_PASS "your_wifi_password"
```

> `arduino_secrets.h` is listed in `.gitignore` and will never be committed.

### 2. GPS coordinates

Copy the example file and set your location:

```
cp include/location_config.h.example include/location_config.h
```

Edit `include/location_config.h`:

```cpp
#define LATITUDE  "51.3397"   // your city
#define LONGITUDE "12.3731"
```

Decimal coordinates can be found on [Google Maps](https://maps.google.com) by right-clicking any point.

> `location_config.h` is also listed in `.gitignore`.

### 3. Build and upload

```bash
pio run --target upload
```

Open the serial monitor at 9600 baud to see connection status and fetched values:

```bash
pio device monitor --baud 9600
```

## Project structure

```
include/
  arduino_secrets.h       ← created by you (gitignored)
  arduino_secrets.h.example
  location_config.h       ← created by you (gitignored)
  location_config.h.example
src/
  main.cpp
platformio.ini
```

## Configuration

Timing constants at the top of `src/main.cpp`:

| Constant | Default | Description |
|----------|---------|-------------|
| `FETCH_INTERVAL_MS` | 15 min | How often weather is re-fetched |
| `CLOUD_DURATION_MS` | 5 000 ms | How long the rain-cloud animation plays |
| `CLOUD_FRAME_MS` | 400 ms | Delay between animation frames |

## Dependencies

Managed automatically by PlatformIO (`platformio.ini`):

- [WiFiS3](https://github.com/arduino-libraries/WiFiS3)
- [ArduinoJson](https://arduinojson.org/) ≥ 7
- [ArduinoGraphics](https://github.com/arduino-libraries/ArduinoGraphics)
- Arduino_LED_Matrix (bundled with the UNO R4 core)
