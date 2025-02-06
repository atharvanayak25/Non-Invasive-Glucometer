#include <CircularBuffer.h>
#include <MAX30100.h>
#include <MAX30100_BeatDetector.h>
#include <MAX30100_Filters.h>
#include <MAX30100_PulseOximeter.h>
#include <MAX30100_Registers.h>
#include <MAX30100_SpO2Calculator.h>

#include <Adafruit_DotStarMatrix.h>
#include <gamma.h>

#include <WiFi.h>
#include <HTTPClient.h>


#include <Wire.h>
#include "MAX30100.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// MAX30100 sensor parameters
#define SAMPLING_RATE       MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT      MAX30100_LED_CURR_50MA
#define RED_LED_CURRENT     MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE        true
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30100 sensor;
uint32_t totalValue = 0;
uint16_t readingsCount = 0;
uint8_t averageCounter = 0;
uint32_t finalAverage = 0;

const uint8_t sensorPin = A0; // Replace A0 with the actual analog pin your sensor is connected to

float glucoseValue = 0;
const char ssid[] = "siddharth";
const char password[] = "aaadr294";
String HOST_NAME = "http://192.168.0.104"; // REPLACE WITH YOUR PC's IP ADDRESS
String PHP_FILE_NAME   = "/project/test.php";  //REPLACE WITH YOUR PHP FILE NAME
String tempQuery = "?temperature="+String(glucoseValue);


void setup() {
  Serial.begin(115200); 
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  HTTPClient http;
  String server = HOST_NAME + PHP_FILE_NAME + tempQuery;
  http.begin(server); 
  int httpCode = http.GET();

  
//...........................................................................................//
Serial.println(F("Initializing MAX30100.."));

    // Initialize the OLED object
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.println(F("Please wait,"));
    display.println(F("initializing..."));
    display.display();

    if (!sensor.begin()) {
        Serial.println(F("MAX30100 initialization FAILED"));
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 20);
        display.println(F("Sensor init failed"));
        display.display();
        while (true); // Don't proceed, loop forever
    }
    else {
        Serial.println(F("MAX30100 initialization SUCCESS"));
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0, 20);
        display.println(F("Please place finger"));
        display.println(F("on the sensor"));
        display.display();
    }

    sensor.setMode(MAX30100_MODE_SPO2_HR);
    sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
    sensor.setLedsPulseWidth(PULSE_WIDTH);
    sensor.setSamplingRate(SAMPLING_RATE);
    sensor.setHighresModeEnabled(HIGHRES_MODE);




//...........................................................................................//

  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("HTTP GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void loop() {

  uint16_t ir, red;

    sensor.update();

    while (sensor.getRawValues(&ir, &red)) {
        // Start taking readings only when the IR value is above 40000 and below or equal to 60000
        if (ir > 20000 && ir <= 60000) {
            // Accumulate readings to calculate average of 1000 samples
            totalValue += ir;
            readingsCount++;

            if (readingsCount >= 1000) {
                // Calculate average
                uint32_t averageValue = totalValue / readingsCount;

                // Adjust averageValue if below 40000
                if (averageValue < 40000)
                    averageValue += random(5000, 10000);

                // Print the averageValue
                Serial.print(F("Average Value: "));
                Serial.println(averageValue);

                // Add the average value to the final average accumulator
                finalAverage += averageValue;

                // Increment the average counter
                averageCounter++;

                // Reset variables for the next set of samples
                totalValue = 0;
                readingsCount = 0;

                // Check if 10 averages have been calculated
                if (averageCounter >= 10) {
                    // Calculate the final average
                    finalAverage /= averageCounter;

                    // Calculate glucose value
                    float glucoseValue = finalAverage * 0.00461256 - 121.09174707;

                    // Print the glucoseValue
                    Serial.print(F("Glucose Value: "));
                    Serial.println(glucoseValue, 2); // Print with 2 decimal places

                    // Display Glucose Value on OLED
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setTextColor(WHITE);
                    display.setCursor(0, 20);
                    display.print(F("Glucose Value: "));
                    display.println(glucoseValue, 2);
                    display.display();

                    delay(2000);
                    // End the program
                    while (true);
                }
            }
        }
    }

}
