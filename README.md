# Weather Station
**ESP32 · DHT11 · SSD1306 OLED**

A compact environmental monitor that displays temperature (°C) and relative humidity (%) on a 128 × 64 OLED, with readings also available through the Serial Monitor.

## Features
- Large temperature and humidity readings.
- A measurement every two seconds using a non-blocking timer.
- Sensor errors replace the live readings; the next scheduled reading retries automatically.
- Checks for an OLED acknowledgement before initializing it.
- Continues measuring through Serial if the OLED is unavailable at startup.
- Pin assignments and timing grouped in one configuration section.

## Hardware
- ESP32 development board with GPIO 4, 21 and 22 available
- DHT11 sensor or sensor module
- SSD1306 128 × 64 I2C OLED
- USB data cable, jumper wires and breadboard

## Wiring
Disconnect power before wiring. Confirm the pin labels on your own modules.

| Component pin | ESP32 connection |
| --- | --- |
| DHT11 DATA / S | GPIO 4 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |
| DHT11 GND | GND |
| OLED GND | GND |

Power each module according to its specifications and keep signals at ESP32-compatible 3.3 V levels. Use a DHT11 module compatible with 3.3 V operation; a bare sensor needs a DATA pull-up according to its datasheet. Do not pull ESP32 GPIO up to 5 V. The sketch expects OLED address `0x3C`; change `Config::OLED_ADDRESS` if your display uses a different address.

## Arduino IDE setup
1. Install the ESP32 board package and choose the board matching your device.
2. Install **Adafruit GFX Library** and **Adafruit SSD1306** with their dependencies.
3. Install the **DHT11** library that provides `DHT11.h`, `readTemperatureHumidity(int&, int&)` and `DHT11::getErrorString()`. This is the API used in the original project; the Adafruit `DHT.h` library is not interchangeable.
4. Keep `weatherStation.ino` in a folder named `weatherStation`. When downloading GitHub ZIP, rename the extracted folder if necessary.
5. Open the sketch, select the USB port, click Verify, then Upload.
6. Open Serial Monitor at **115200 baud**.

The first measurement occurs after two seconds. This project reads the local sensor; it does not fetch online weather or connect to Wi-Fi.

## Troubleshooting
| Symptom | Check |
| --- | --- |
| DHT11 read error | Power, common ground, GPIO 4, pin order and DATA pull-up |
| OLED not found | SDA/SCL, display power and I2C address |
| Missing header / compiler error | Installed libraries and their API; choose an ESP32 board |
| No Serial output | USB data cable, port and 115200 baud |

If the OLED was absent at startup, reconnect it and reset the board to initialize it.

## Validation
Source reviewed for timing, error handling and display bounds. Compilation with the real board package and libraries, and physical hardware testing, are still required. No hardware test is claimed.

---

## نبذة بالعربية
محطة بسيطة لقياس الحرارة والرطوبة باستخدام ESP32 وحساس DHT11 وشاشة OLED. تتحدث القراءات كل ثانيتين، وتظهر رسالة واضحة عند تعذر قراءة الحساس. إذا لم تُكتشف الشاشة عند بدء التشغيل، تستمر القراءات عبر Serial Monitor.

**التوصيلات:** الحساس GPIO 4، والشاشة SDA على GPIO 21 وSCL على GPIO 22. سرعة Serial Monitor هي 115200. لا يحتاج المشروع إلى إنترنت.

