#include <DHT11.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT11 dht11(4);


// [BEGIN lopaka generated]

static const unsigned char PROGMEM image_weather_humidity_bits[] = {0x04,0x00,0x04,0x00,0x0c,0x00,0x0e,0x00,0x1e,0x00,0x1f,0x00,0x3f,0x80,0x3f,0x80,0x7e,0xc0,0x7f,0x40,0xff,0x60,0xff,0xe0,0x7f,0xc0,0x7f,0xc0,0x3f,0x80,0x0f,0x00};

static const unsigned char PROGMEM image_weather_temperature_bits[] = {0x1c,0x00,0x22,0x02,0x2b,0x05,0x2a,0x02,0x2b,0x38,0x2a,0x60,0x2b,0x40,0x2a,0x40,0x2a,0x60,0x49,0x38,0x9c,0x80,0xae,0x80,0xbe,0x80,0x9c,0x80,0x41,0x00,0x3e,0x00};


void drawScreen_1(int temperature, int humidity) {

    display.clearDisplay();

    // string 1
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(21, 4);
    display.print("Weather Station");

    // Line
    display.drawLine(0, 16, 127, 16, WHITE);

    // weather_temperature
    display.drawBitmap(9, 29, image_weather_temperature_bits, 16, 16, WHITE);

    // weather_humidity
    display.drawBitmap(78, 28, image_weather_humidity_bits, 11, 16, WHITE);

    // Humidity
    display.setCursor(94, 34);
    display.print(humidity);
    display.print("%");

    // Temperature
    display.setCursor(28, 34);
    display.print(temperature);
    display.print("C");

    display.display();

}

// [END lopaka generated]


void setup() {

    Serial.begin(115200);

    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

        Serial.println("OLED Error");

        while (true) {
            delay(1000);
        }
    }

    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(20, 25);
    display.print("Weather Station");
    display.display();

    Serial.println("Weather Station Started");

    delay(2000);

}


void loop() {

    int temperature = 0;
    int humidity = 0;

    // Read DHT11 sensor
    int result = dht11.readTemperatureHumidity(temperature, humidity);

    if (result == 0) {

        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print(" C | Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");

        drawScreen_1(temperature, humidity);

    } else {

        Serial.print("DHT11 Error: ");
        Serial.println(DHT11::getErrorString(result));

        display.clearDisplay();

        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(20, 25);
        display.print("DHT11 Error");

        display.display();

    }

    delay(2000);

}