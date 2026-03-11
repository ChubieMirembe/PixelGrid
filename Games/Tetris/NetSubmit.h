#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
extern "C" {
  #include "esp_wifi.h"
  #include "esp_eap_client.h"
}

#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include "NetConfig.h"


static inline String joinUrl(const char* base, const char* path) {
  String b(base);
  if (b.endsWith("/")) b.remove(b.length() - 1);
  String p(path);
  if (!p.startsWith("/")) p = "/" + p;
  return b + p;
}

static inline String b64url_from_bytes(const uint8_t* data, size_t len) {
  // Standard base64 to buffer then translate to base64url and strip '='
  size_t outLen = 0;
  // base64 length upper bound: 4*ceil(n/3) + 1
  size_t cap = 4 * ((len + 2) / 3) + 1;
  char* out = (char*)malloc(cap);
  if (!out) return "";

  int rc = mbedtls_base64_encode((unsigned char*)out, cap, &outLen, data, len);
  if (rc != 0) { free(out); return ""; }
  out[outLen] = 0;

  String s(out);
  free(out);

  s.replace("+", "-");
  s.replace("/", "_");
  while (s.endsWith("=")) s.remove(s.length() - 1);
  return s;
}
static inline bool netEnsureTime(uint32_t timeoutMs = 15000) {
  // If time already looks sane, don't re-sync.
  time_t now = time(nullptr);
  if (now > 1700000000) return true; // ~2023-11

  // Configure NTP. These pool hosts usually work on campus networks.
  // If eduroam blocks NTP, this may hang/fail and you’ll need your uni’s NTP host.
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

  const uint32_t start = millis();
  while (time(nullptr) < 1700000000) {
    if (millis() - start > timeoutMs) {
      Serial.println("[NET] NTP sync timeout (time still not set).");
      return false;
    }
    delay(250);
    Serial.print("t");
  }
  Serial.println();
  Serial.printf("[NET] Time OK: %ld\n", (long)time(nullptr));
  return true;
}
static inline String canonicalDeviceMessage(const char* game_code, int score, int ts, const char* nonce) {
  // MUST match server.py canonical_device_message()
  String msg;
  msg.reserve(128);
  msg += "game_code="; msg += game_code;
  msg += "&score=";    msg += String(score);
  msg += "&ts=";       msg += String(ts);
  msg += "&nonce=";    msg += nonce;
  return msg;
}

static inline String computeDeviceSig(const char* secret, const char* game_code, int score, int ts, const char* nonce) {
  String msg = canonicalDeviceMessage(game_code, score, ts, nonce);

  uint8_t mac[32];
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) return "";

  int rc = mbedtls_md_hmac(info,
                          (const unsigned char*)secret, strlen(secret),
                          (const unsigned char*)msg.c_str(), msg.length(),
                          mac);
  if (rc != 0) return "";

  return b64url_from_bytes(mac, sizeof(mac));
}

static inline void eapConfigure() {
  // Configure EAP creds. You said you already have a working combination;
  // keep it to set_identity/set_username/set_password only.
  esp_eap_client_set_identity((uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_eap_client_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
  esp_eap_client_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));

  // PEAP is the common eduroam type; your header supports set_eap_methods.
  esp_eap_client_set_eap_methods(ESP_EAP_TYPE_PEAP);

  // If your eduroam requires CA pinning, we can add esp_eap_client_set_ca_cert later.
  esp_eap_client_clear_ca_cert();
  esp_eap_client_clear_certificate_and_key();

  // Enable enterprise auth in WiFi stack
  esp_wifi_sta_enterprise_enable();
}

static inline bool netEnsureWifi(uint32_t timeoutMs = 45000) {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.disconnect(true, true);
  delay(250);
  WiFi.mode(WIFI_STA);

  eapConfigure();

  Serial.println("[NET] Connecting to eduroam...");
  WiFi.begin(EDUROAM_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) {
      Serial.printf("[NET] WiFi connect timeout, status=%d\n", (int)WiFi.status());
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("[NET] IP: "); Serial.println(WiFi.localIP());

  if (!netEnsureTime()) return false;

  return true;
}

static inline bool httpJsonPost(const String& url, const String& jsonBody, int& httpCodeOut, String& responseOut) {
  WiFiClientSecure client;
  client.setInsecure(); // ngrok/testing; for production you should validate certs

  HTTPClient http;
  if (!http.begin(client, url)) return false;

  http.addHeader("Content-Type", "application/json");
  httpCodeOut = http.POST((uint8_t*)jsonBody.c_str(), jsonBody.length());
  responseOut = http.getString();
  http.end();
  return true;
}

static inline bool extractJsonStringField(const String& json, const char* field, String& out) {
  // Minimal, non-robust field extractor: looks for "field":"value"
  // Good enough for your server responses.
  String key = String("\"") + field + "\":";
  int i = json.indexOf(key);
  if (i < 0) return false;
  i = json.indexOf('"', i + key.length());
  if (i < 0) return false;
  int j = json.indexOf('"', i + 1);
  if (j < 0) return false;
  out = json.substring(i + 1, j);
  return true;
}

static inline bool submitScoreToServer(uint32_t score, String& outCode) {
  outCode = "";

  if (!netEnsureWifi()) return false;

  // IMPORTANT: server enforces abs(now - ts) <= 120
  const int ts = (int)(time(nullptr));
  if (ts < 1700000000) {
    Serial.println("[NET] Time not set; cannot sign request. (Need NTP sync)");
    return false;
  }

  char nonceBuf[24];
  uint32_t r1 = (uint32_t)esp_random();
  uint32_t r2 = (uint32_t)esp_random();
  snprintf(nonceBuf, sizeof(nonceBuf), "%08lx%08lx", (unsigned long)r1, (unsigned long)r2);

  String sig = computeDeviceSig(GAME_SECRET, GAME_CODE, (int)score, ts, nonceBuf);
  if (sig.length() == 0) {
    Serial.println("[NET] Failed to compute HMAC signature.");
    return false;
  }

  String codesUrl = joinUrl(API_BASE, "/api/codes");

  String body1;
  body1.reserve(256);
  body1 += "{";
  body1 += "\"score\":"; body1 += String((int)score); body1 += ",";
  body1 += "\"game_code\":\""; body1 += GAME_CODE; body1 += "\",";
  body1 += "\"ts\":"; body1 += String(ts); body1 += ",";
  body1 += "\"nonce\":\""; body1 += nonceBuf; body1 += "\",";
  body1 += "\"sig\":\""; body1 += sig; body1 += "\"";
  body1 += "}";

  int http1 = 0;
  String resp1;
  if (!httpJsonPost(codesUrl, body1, http1, resp1)) {
    Serial.println("[NET] POST /api/codes failed to start.");
    return false;
  }

  Serial.printf("[NET] /api/codes -> %d\n", http1);
  if (http1 < 200 || http1 >= 300) {
    Serial.println(resp1);
    return false;
  }

  String oneTimeCode;
  if (!extractJsonStringField(resp1, "code", oneTimeCode)) {
    Serial.println("[NET] Could not parse code from /api/codes response.");
    Serial.println(resp1);
    return false;
  }

  // This is what you want to show to the user on the LCD
  outCode = oneTimeCode;
  return true;
}