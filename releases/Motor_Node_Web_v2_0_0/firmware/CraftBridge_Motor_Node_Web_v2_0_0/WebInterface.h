#pragma once

#include "CanBus.h"
#include "EngineData.h"
#include "RuntimeMetrics.h"
#include "SmartCraftSession.h"

#ifdef ARDUINO
#include <WebServer.h>
#endif

#include <cstddef>
#include <cstdint>

namespace craftbridge {

// WebInterface stores a typed 250 ms snapshot. Text is formatted only when an
// HTTP request arrives, so the main loop does not allocate or copy strings.
class WebInterface {
public:
  WebInterface();

  bool begin();
  void update(
      const EngineData &engine_data,
      const SessionStatus &session_status,
      const CanDiagnostics &can_diagnostics,
      const RuntimeMetrics &runtime_metrics,
      uint32_t now_ms);
  void loop();

  const char *renderDashboardHtml();
  const char *renderDiagnosticsHtml();
  const char *renderStatusJson();

private:
  static constexpr size_t kResponseCapacity = 8192;

  void clearResponse();
  void appendText(const char *text);
  void appendFormat(const char *format, ...);
  void appendJsonSignal(
      const char *name,
      const char *value,
      const char *unit,
      const SignalState &state,
      bool prepend_comma);

#ifdef ARDUINO
  WebServer server_;
#endif
  EngineData engine_data_{};
  SessionStatus session_status_{};
  CanDiagnostics can_diagnostics_{};
  double fuel_used_liters_{};
  uint64_t trip_time_ms_{};
  uint32_t captured_at_ms_{};
  uint32_t last_can_rx_total_{};
  uint32_t last_can_error_total_{};
  uint32_t last_can_activity_ms_{};
  uint32_t last_can_error_ms_{};
  bool can_activity_seen_{};
  bool can_error_seen_{};
  bool snapshot_ready_{};
  char response_[kResponseCapacity]{};
  size_t response_length_{};
};

} // namespace craftbridge
