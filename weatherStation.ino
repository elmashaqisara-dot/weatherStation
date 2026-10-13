/**
 * Weather Station — ESP32 + DHT11 + SSD1306 (128x64, I2C).
 * DHT11 data: GPIO 4. OLED SDA: 21, SCL: 22, address: 0x3C.
 * Library: DHT11 with readTemperatureHumidity/getErrorString API.
 */
#include <DHT11.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace Config {
constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr unsigned long READ_INTERVAL_MS = 2000;
constexpr unsigned long SERIAL_BAUD = 115200;
}

DHT11 sensor(Config::DHT_PIN);
Adafruit_SSD1306 display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, -1);
bool displayReady = false;
unsigned long lastReadMs = 0;

// Check acknowledgement separately: display.begin alone does not prove
// that an I2C display is physically connected.
bool initDisplay() {
  Wire.beginTransmission(Config::OLED_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("OLED not found. Check address, power, SDA and SCL."));
    return false;
  }
  if (!display.begin(SSD1306_SWITCHCAPVCC, Config::OLED_ADDRESS)) {
    Serial.println(F("OLED initialization failed."));
    return false;
  }
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  return true;
}

void drawHeader() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(19, 3);
  display.print(F("WEATHER STATION"));
  display.drawFastHLine(0, 14, Config::SCREEN_WIDTH, SSD1306_WHITE);
}

void showReading(int temperature, int humidity) {
  if (!displayReady) return;
  drawHeader();
  display.setCursor(3, 20);
  display.print(F("TEMP"));
  display.setCursor(70, 20);
  display.print(F("HUMIDITY"));
  display.drawFastVLine(63, 19, 31, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(3, 34);
  display.print(temperature);
  display.setTextSize(1);
  display.print(F(" C"));
  display.setTextSize(2);
  display.setCursor(70, 34);
  display.print(humidity);
  display.setTextSize(1);
  display.print(F("%"));
  display.setCursor(3, 56);
  display.print(F("LIVE | Refresh: 2s"));
  display.display();
}

void showSensorError() {
  if (!displayReady) return;
  drawHeader();
  display.setCursor(10, 26);
  display.print(F("DHT11 READ ERROR"));
  display.setCursor(7, 41);
  display.print(F("Check GPIO4 / power"));
  display.setCursor(7, 55);
  display.print(F("Retrying every 2s"));
  display.display();
}

void readWeather() {
  int temperature = 0;
  int humidity = 0;
  const int result = sensor.readTemperatureHumidity(temperature, humidity);
  if (result != 0) {
    Serial.print(F("DHT11 error: "));
    Serial.println(DHT11::getErrorString(result));
    showSensorError();
    return;
  }
  // Reject invalid values rather than displaying misleading measurements.
  if (humidity < 0 || humidity > 100) {
    Serial.println(F("Invalid humidity reading."));
    showSensorError();
    return;
  }
  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.print(F(" C | Humidity: "));
  Serial.print(humidity);
  Serial.println(F(" %"));
  showReading(temperature, humidity);
}

void setup() {
  Serial.begin(Config::SERIAL_BAUD);
  Wire.begin(Config::SDA_PIN, Config::SCL_PIN);
  displayReady = initDisplay();
  if (displayReady) {
    drawHeader();
    display.setCursor(12, 32);
    display.print(F("Starting sensor..."));
    display.display();
  }
  Serial.println(F("Weather Station started. First reading in 2 seconds."));
  lastReadMs = millis();
}

void loop() {
  const unsigned long now = millis();
  // Unsigned subtraction also handles millis() rollover.
  if (now - lastReadMs >= Config::READ_INTERVAL_MS) {
    lastReadMs = now;
    readWeather();
  }
}

