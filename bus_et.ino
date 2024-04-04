//#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_wpa2.h"
#include <WiFi.h>

#define NUM_LEDS 256
#define DATA_PIN 13

// Vertical layout
//Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 32, DATA_PIN,
//  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
//  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,
//  NEO_GRB            + NEO_KHZ800
//);

// Horizontal layout
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, DATA_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS    + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800
);

DynamicJsonDocument doc(1024);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(255, 255, 0), matrix.Color(0, 0, 255), matrix.Color(255, 0, 255), matrix.Color(0, 255, 255), matrix.Color(255, 255, 255)
};
int pixelPerChar = 5; // Width of Standard Font Characters is 8X6 Pixels
int x = 32; // Width of the Display
int pass = 0; // Counter
int i = 0; // Counter
int clr = 0; // Counter for Indexing Array of Colors

void setup() {  
  Serial.begin(115200);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(20);
  matrix.setTextColor(matrix.Color(80,255,0));

  setup_wpa2();
}

void loop() {
  WiFiClient client;

  HTTPClient http;

  String text = "";

  String bus_id = "";

  Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://ucf.doublemap.com/map/v2/eta?stop=82")) {  // HTTP


    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        deserializeJson(doc, payload);
        JsonObject obj = doc.as<JsonObject>();

        int min = INT_MAX;

        for (JsonObject elem : doc["etas"]["82"]["etas"].as<JsonArray>()) {
            String name2 = elem["avg"];

            if (name2.toInt() < min) {
              min = name2.toInt();
              bus_id = elem["bus_id"].as<String>();
            }
        }

        text = String(min);
      }  
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("[HTTP] Unable to connect");
  }

  // Get last stop
  String last_stop = "";
  String next_stop = "";
  int stops_left = 0;
  if (http.begin(client, "http://ucf.doublemap.com/map/v2/buses")) {  // HTTP


    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        deserializeJson(doc, payload);
        JsonObject obj = doc.as<JsonObject>();

        for (JsonObject elem : doc.as<JsonArray>()) {
            String last_stop_id = elem["lastStop"].as<String>();
            String id = elem["id"].as<String>();

            if (last_stop_id != NULL && id != NULL) {
              if (id == bus_id) {
                Serial.println("Found bus!");
                if (last_stop_id == "81") {
                  last_stop = "CDL";
                  next_stop = "9 at Central";
                  stops_left = 4;
                } else if (last_stop_id == "95") {
                  last_stop = "9 at Central";
                  next_stop = "Village at Science Dr";
                  stops_left = 3;
                } else if (last_stop_id == "29") {
                  last_stop = "Village at Science Dr";
                  next_stop = "Research Pavilion";
                  stops_left = 2;
                } else if (last_stop_id == "30") {
                  last_stop = "Research Pavilion";
                  next_stop = "UCF";
                  stops_left = 1;
                } else if (last_stop_id == "31") {
                  last_stop = "UCF";
                  next_stop = "9 at Central";
                  stops_left = 0;
                }
                break;
              }
            }
        }
      }  
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("[HTTP] Unable to connect");
  }

  if (stops_left == 1) {
    writeText("R5: " + text + " min, " + stops_left + " stop left ");
  } else {
    writeText("R5: " + text + " min, " + stops_left + " stops left ");
  }

  delay(2000);
}

/* @author adavegetable
 *  @link https://forums.adafruit.com/viewtopic.php?t=115535
 */
void writeText(String msg) {
  int arrSize = sizeof(colors) / sizeof(*colors); // Array of Text Colors;
  int msgSize = (msg.length() * pixelPerChar) + (2 * pixelPerChar); // CACULATE message length;
  int scrollingMax = (msgSize) + matrix.width(); // ADJUST Displacement for message length;

  x = matrix.width(); // RESET Cursor Position and Start Text String at New Position on the Far Right;
  clr = 0; // RESET Color/Repeat Index;

  while (clr <= arrSize) {
    /* Change Color with Each Pass of Complete Message */
    matrix.setTextColor(colors[clr]);

    matrix.fillScreen(0); // BLANK the Entire Screen;
    matrix.setCursor(x, 0); // Set Starting Point for Text String;
    matrix.print(msg); // Set the Message String;

    /* SCROLL TEXT FROM RIGHT TO LEFT BY MOVING THE CURSOR POSITION */
    if (--x < -scrollingMax ) {
      /*  ADJUST FOR MESSAGE LENGTH  */
      // Decrement x by One AND Compare New Value of x to -scrollingMax;
    // This Animates (moves) the text by one pixel to the Left;

      x = matrix.width(); // After Scrolling by scrollingMax pixels, RESET Cursor Position and Start String at New Position on the Far Right;
      ++clr; // INCREMENT COLOR/REPEAT LOOP COUNTER AFTER MESSAGE COMPLETED;
    }
    matrix.show(); // DISPLAY the Text/Image
    delay(80); // SPEED OF SCROLLING or FRAME RATE;
  }
  clr = 0; // Reset Color/Loop Counter;

/* LATHER - RINSE - REPEAT - Why coders have such nice hair */
}
