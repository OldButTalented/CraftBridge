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

#include "CanBus.h"
#include "Config.h"
#include "RuntimeMetrics.h"
#include "SmartCraftSession.h"
#include "WebInterface.h"

// -----------------------------------------------------------------------------
// Application components
// -----------------------------------------------------------------------------

craftbridge::CanBus can_bus;
craftbridge::SmartCraftSession smartcraft_session(can_bus);
craftbridge::RuntimeMetrics runtime_metrics;
craftbridge::WebInterface web_interface;
uint32_t last_web_snapshot_ms = 0;

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
  delay(1);
}
