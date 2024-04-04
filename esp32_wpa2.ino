#include "esp_wpa2.h"
#include <WiFi.h>
#include "credentials.h"

// Credentials:
// #define EAP_ID
// #define EAP_USERNAME
// #define EAP_PASSWORD
// #define ssid

void setup_wpa2() {
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);

  Serial.println("Connecting to ");
  Serial.println(ssid);
  
  WiFi.disconnect(true);      
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wifi_sta_wpa2_ent_enable();
  
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
