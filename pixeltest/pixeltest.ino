#include <WiFi.h>
#include <HTTPClient.h>

extern "C" {
  #include "esp_wifi.h"
  #include "esp_wpa2.h"
}

// =======================
// Eduroam config
// =======================
static const char* SSID = "eduroam";

// Inner credentials (your real login)
static const char* EAP_USERNAME = "c.mirembe-2024@hull.ac.uk";   // change
static const char* EAP_PASSWORD = "Kingblade998";              // change

// Outer identity (often anonymous)
static const char* EAP_IDENTITY = "anonymous@hull.ac.uk";

// =======================
// API config
// =======================
static const char* API_URL =
  "http://10.31.228.19/api/leaderboard?limit=5&game=snake";

// Call interval
static const unsigned long API_INTERVAL_MS = 10000;

// =======================
// Helpers
// =======================
static bool connectEduroam(unsigned long timeoutMs = 40000) {
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

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    return true;
  }

  Serial.println("Failed to connect.");
  Serial.print("WiFi status: ");
  Serial.println((int)WiFi.status());
  return false;
}

static void callApiOnce() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping API call.");
    return;
  }

  HTTPClient http;
  http.setTimeout(5000);

  Serial.print("GET ");
  Serial.println(API_URL);

  http.begin(API_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.print("HTTP ");
    Serial.println(httpCode);

    String payload = http.getString();
    Serial.println("Response:");
    Serial.println(payload);
  } else {
    Serial.print("HTTP error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

// =======================
// Arduino entrypoints
// =======================
static unsigned long lastApiMs = 0;
static unsigned long lastReconnectAttemptMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  // Connect at boot
  connectEduroam();

  // Optional quick public test (remove if you want)
  {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(5000);
      http.begin("http://httpbin.org/get");
      int code = http.GET();
      Serial.print("Internet test code: ");
      Serial.println(code);
      http.end();
    }
  }

  // Make an immediate API call once after boot (optional)
  callApiOnce();
  lastApiMs = millis();
}

void loop() {
  const unsigned long now = millis();

  // If WiFi dropped, try to reconnect every 10 seconds
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastReconnectAttemptMs > 10000) {
      lastReconnectAttemptMs = now;
      connectEduroam();
    }
    delay(50);
    return;
  }

  // Call API periodically
  if (now - lastApiMs >= API_INTERVAL_MS) {
    lastApiMs = now;
    callApiOnce();
  }

  delay(10);
}
