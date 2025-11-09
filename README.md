# Environment Monitoring Station using ESP32 (DHT11 + MQ-135 + LDR)

ESP32-based monitor that reads **temperature**, **humidity**, **air quality** (raw MQ-135), and **light level** (LDR). It hosts a **web dashboard** (auto-refresh) and a **JSON API**. An **LED** switches on when it’s dark; a **buzzer** warns on poor air quality. Thresholds are easy to calibrate in code.

## 🧩 Hardware
- ESP32 DevKit (ADC pins available)
- DHT11 (temp/humidity) on **GPIO 4**
- MQ-135 analog out on **GPIO 34** (ADC1)
- LDR voltage divider on **GPIO 35** (ADC1)
- LED on **GPIO 26** (with resistor)
- Buzzer on **GPIO 27**
- 5V USB power

> **ESP32 ADC note:** Pins **34/35** are input-only (correct for sensors).

## 📚 Libraries
- `DHT sensor library` (Adafruit)
- `WebServer` (built-in with ESP32 core)
- `WiFi` (ESP32 core)

Install via **Boards Manager** (ESP32 by Espressif) and **Library Manager**.

## 🔌 Wiring
- **DHT11:** VCC 3.3V/5V, GND, DATA → GPIO 4 (with ~10k pull-up if needed)
- **MQ-135:** VCC 5V (module), GND, **AO → GPIO 34**
- **LDR:** voltage divider → **GPIO 35**
- **LED:** anode → **GPIO 26** through 220Ω, cathode → GND
- **Buzzer:** + → **GPIO 27**, − → GND

## ⚙️ Configure
Edit in `Environment_Monitor_ESP32.ino`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

int LDR_THRESHOLD = 1500;
int MQ_THRESHOLD  = 1800;
