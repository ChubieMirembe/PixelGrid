#include <WiFi.h>
#include <HTTPClient.h>

extern "C" {
  #include "esp_wifi.h"
  #include "esp_wpa2.h"
}

static const char* SSID = "eduroam";

// INNER credentials (real login)
static const char* EAP_USERNAME = "username";     // Modify
static const char* EAP_PASSWORD = "password";     // Modify

// OUTER identity (recommended for eduroam)
static const char* EAP_IDENTITY = "anonymous@hull.ac.uk";

void connectEduroam() {

  Serial.println("Connecting to eduroam...");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  delay(200);

  esp_wifi_sta_wpa2_ent_set_identity(
      (uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_username(
      (uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password(
      (uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));

  esp_wifi_sta_wpa2_ent_enable();
  WiFi.begin(SSID);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 80) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect.");
    Serial.print("WiFi status: ");
    Serial.println((int)WiFi.status());
  }
}

void testInternet() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected. Skipping HTTP test.");
    return;
  }

  Serial.println("Testing internet access...");

  HTTPClient http;
  http.begin("http://httpbin.org/get");   // simple public test endpoint

  int httpCode = http.GET();

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Internet test SUCCESS");
  } else {
    Serial.println("Internet test FAILED");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  connectEduroam();
  testInternet();
}

void loop() {
  // Keep connection alive
}
