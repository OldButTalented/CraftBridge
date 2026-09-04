// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <WiFi.h>
#include <esp_err.h>
#include <esp_mac.h>
#include <esp_wifi.h>

#include <cstring>

namespace {

constexpr uint32_t kSerialBaud = 115200;

bool isZeroMac(const uint8_t mac[6]) {
  constexpr uint8_t kZeroMac[6]{};
  return std::memcmp(mac, kZeroMac, sizeof(kZeroMac)) == 0;
}

void printMac(const char *label, const uint8_t mac[6]) {
  Serial.printf(
      "%-14s: %02X:%02X:%02X:%02X:%02X:%02X\n",
      label,
      mac[0],
      mac[1],
      mac[2],
      mac[3],
      mac[4],
      mac[5]);
}

void printReadError(const char *label, esp_err_t error) {
  Serial.printf(
      "%-14s: ERROR (%s)\n",
      label,
      error == ESP_OK ? "zero MAC returned" : esp_err_to_name(error));
}

} // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(250);

  uint8_t base_mac[6]{};
  uint8_t sta_mac[6]{};
  uint8_t softap_mac[6]{};

  const esp_err_t base_result = esp_read_mac(base_mac, ESP_MAC_BASE);

  const bool sta_started = WiFi.STA.begin(false);
  const bool softap_started = WiFi.AP.begin();

  const esp_err_t sta_result =
      sta_started ? esp_wifi_get_mac(WIFI_IF_STA, sta_mac)
                  : ESP_ERR_WIFI_NOT_INIT;
  const esp_err_t softap_result =
      softap_started ? esp_wifi_get_mac(WIFI_IF_AP, softap_mac)
                     : ESP_ERR_WIFI_NOT_INIT;

  const bool base_valid = base_result == ESP_OK && !isZeroMac(base_mac);
  const bool sta_valid = sta_result == ESP_OK && !isZeroMac(sta_mac);
  const bool softap_valid =
      softap_result == ESP_OK && !isZeroMac(softap_mac);

  Serial.println();
  Serial.println("=== CraftBridge Node Identity ===");
  Serial.println();

  if (base_valid) {
    printMac("Chip/Base MAC", base_mac);
  } else {
    printReadError("Chip/Base MAC", base_result);
  }

  if (sta_valid) {
    printMac("STA MAC", sta_mac);
  } else {
    printReadError("STA MAC", sta_result);
  }

  if (softap_valid) {
    printMac("SoftAP MAC", softap_mac);
  } else {
    printReadError("SoftAP MAC", softap_result);
  }

  Serial.println();
  Serial.println("Pairing use:");
  Serial.println();
  Serial.println("If this board will be used as MOTOR NODE:");
  Serial.println("  Copy the SoftAP MAC to Instrument Node Config.h");
  Serial.println("  as the expected Motor Node ESP-NOW source MAC.");
  Serial.println();
  Serial.println("If this board will be used as INSTRUMENT NODE:");
  Serial.println("  Copy the STA MAC to Motor Node Config.h");
  Serial.println("  as the Instrument Node ESP-NOW peer destination MAC.");
  Serial.println();

  if (base_valid && sta_valid && softap_valid) {
    Serial.println("Result: OK - identity values are ready to use.");
  } else {
    Serial.println("Result: ERROR - do not use any MAC marked ERROR.");
  }

  Serial.println("=================================");
}

void loop() {
  delay(1000);
}