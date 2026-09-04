/*
 * CraftBridge Motor Node Web
 *
 * This firmware reads SmartCraft CAN traffic, establishes or reuses the
 * required engine-data session, decodes six verified engine signals, and
 * exposes normalized values through a local Wi-Fi web interface.
 *
 * Data flow:
 *   SmartCraft CAN -> CanBus -> SmartCraftSession -> EngineData -> WebInterface
 *
 * Safety:
 *   Application transmission is zero during the initial observation period.
 *   Only the fixed, response-gated startup sequence can transmit. A startup
 *   failure or session loss stops application transmission and never triggers
 *   an automatic retry.
 *
 * The implementation targets the tested SmartCraft engine implementation.
 * Other engines can differ and require independent protocol and bench
 * validation before use.
 *
 * File overview:
 *   Config             Hardware pins, identity, and fixed application settings.
 *   CanBus             TWAI transport, CAN frames, and bus diagnostics.
 *   SmartCraftSession  Startup authorization and session supervision.
 *   EngineData         Signal storage, decoding, scaling, and freshness.
 *   RuntimeMetrics     Boot-scoped fuel-used and running-time integration.
 *   WebInterface       Wi-Fi access point, HTML dashboard, and JSON endpoint.
 */

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <esp_wifi.h>

#include <cstdio>
#include <cstring>

#include "CanBus.h"
#include "Config.h"
#include "EspNowTransport.h"
#include "RuntimeMetrics.h"
#include "SmartCraftSession.h"
#include "WebInterface.h"

// -----------------------------------------------------------------------------
// Application components
// -----------------------------------------------------------------------------

craftbridge::CanBus can_bus;
craftbridge::EspNowTransport esp_now_transport;
craftbridge::SmartCraftSession smartcraft_session(can_bus);
craftbridge::RuntimeMetrics runtime_metrics;
craftbridge::WebInterface web_interface;
uint32_t last_web_snapshot_ms = 0;
uint32_t last_esp_now_diagnostics_ms = 0;
bool first_esp_now_result_logged = false;

namespace {

constexpr uint32_t kEspNowDiagnosticsIntervalMs = 10000;

const char *sendStatusText(craftbridge::EspNowSendStatus status) {
  switch (status) {
    case craftbridge::EspNowSendStatus::Success:
      return "success";
    case craftbridge::EspNowSendStatus::DeliveryFailed:
      return "delivery_fail";
    case craftbridge::EspNowSendStatus::ImmediateFailed:
      return "immediate_fail";
    case craftbridge::EspNowSendStatus::None:
      return "none";
  }
  return "unknown";
}

bool writeSerialNonBlocking(const char *line) {
  const size_t length = std::strlen(line);
  if (Serial.availableForWrite() < static_cast<int>(length)) {
    return false;
  }
  return Serial.write(
      reinterpret_cast<const uint8_t *>(line),
      length) == length;
}

void logEspNowDiagnostics(uint32_t now_ms) {
  craftbridge::EspNowTxDiagnostics diagnostics{};
  esp_now_transport.takeDiagnostics(diagnostics);
  char line[128]{};

  if (!first_esp_now_result_logged &&
      diagnostics.last_status != craftbridge::EspNowSendStatus::None) {
    std::snprintf(
        line,
        sizeof(line),
        "ESP-NOW TX first: %s attempts=%lu success=%lu fail=%lu\n",
        sendStatusText(diagnostics.last_status),
        static_cast<unsigned long>(diagnostics.send_attempts),
        static_cast<unsigned long>(diagnostics.send_successes),
        static_cast<unsigned long>(diagnostics.send_failures));
    first_esp_now_result_logged = writeSerialNonBlocking(line);
  }

  if (now_ms - last_esp_now_diagnostics_ms <
      kEspNowDiagnosticsIntervalMs) {
    return;
  }

  std::snprintf(
      line,
      sizeof(line),
      "ESP-NOW TX: attempts=%lu success=%lu fail=%lu last=%s\n",
      static_cast<unsigned long>(diagnostics.send_attempts),
      static_cast<unsigned long>(diagnostics.send_successes),
      static_cast<unsigned long>(diagnostics.send_failures),
      sendStatusText(diagnostics.last_status));
  if (writeSerialNonBlocking(line)) {
    last_esp_now_diagnostics_ms = now_ms;
  }
}

void printConfiguredEspNow() {
  const uint8_t *peer = craftbridge::config::kEspNowPeerMac;
  Serial.printf(
      "ESP-NOW peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
      peer[0], peer[1], peer[2], peer[3], peer[4], peer[5]);

  uint8_t primary = 0;
  wifi_second_chan_t secondary = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &secondary) == ESP_OK) {
    Serial.printf("ESP-NOW runtime channel: %u\n", primary);
  } else {
    Serial.println("ESP-NOW runtime channel: unavailable");
  }
}

} // namespace
// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(craftbridge::config::kSerialMonitorBaud);
  delay(200);

  Serial.printf(
      "%s\nSAFE IDLE: SmartCraft application TX = 0\n",
      craftbridge::config::kFirmwareIdentity);

  if (web_interface.begin()) {
    Serial.printf(
        "Web AP ready: SSID %s, %s\n",
        craftbridge::config::kAccessPointName,
        craftbridge::config::kAccessPointAddress);
  } else {
    Serial.println("Web AP initialization failed");
  }

  if (esp_now_transport.begin()) {
    Serial.println("ESP-NOW output ready");
  } else {
    Serial.println("ESP-NOW initialization failed; Motor Node continues");
  }
  printConfiguredEspNow();

  Serial.printf(
      "Starting TWAI: 250 kbit/s TX GPIO%d RX GPIO%d\n",
      craftbridge::config::kCanTxGpio,
      craftbridge::config::kCanRxGpio);

  // Start CAN only after blocking initialization so RX can be serviced
  // immediately when setup returns. Do not block after a successful start.
  if (!can_bus.begin()) {
    Serial.println("TWAI initialization failed");
  }
}

// -----------------------------------------------------------------------------
// CAN receive processing
// -----------------------------------------------------------------------------

void processCanFrames(uint32_t now_ms) {
  craftbridge::CanFrame frame{};
  while (can_bus.receive(frame)) {
    smartcraft_session.onFrame(frame, now_ms);
  }

  can_bus.updateDiagnostics();
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------

void loop() {
  const uint32_t now_ms = millis();

  if (can_bus.ready()) {
    processCanFrames(now_ms);
  }

  smartcraft_session.tick(now_ms);
  runtime_metrics.update(smartcraft_session.data(), now_ms);
  esp_now_transport.tick(
      smartcraft_session.data(),
      runtime_metrics,
      now_ms);

  if (now_ms - last_web_snapshot_ms >=
      craftbridge::config::kWebSnapshotIntervalMs) {
    web_interface.update(
        smartcraft_session.data(),
        smartcraft_session.status(),
        can_bus.diagnostics(),
        runtime_metrics,
        now_ms);
    last_web_snapshot_ms = now_ms;
  }

  web_interface.loop();
  logEspNowDiagnostics(now_ms);
  delay(1);
}
