#pragma once

#include "EngineData.h"
#include "RuntimeMetrics.h"

#include <cstdint>

#include <esp_now.h>
#include <freertos/FreeRTOS.h>

namespace craftbridge {

#pragma pack(push, 1)
struct EspNowPacket {
  uint8_t protocol_version;
  uint8_t validity_mask;
  uint16_t reserved;
  uint32_t sequence;
  uint32_t boot_id;
  uint16_t rpm;
  int16_t coolant_temperature_c;
  float runtime_hours;
  float oil_pressure_kpa;
  float battery_voltage_v;
  float fuel_flow_lph;
  float fuel_used_l;
  uint32_t trip_time_s;
};
#pragma pack(pop)

static_assert(sizeof(EspNowPacket) == 40, "Unexpected ESP-NOW packet size");

enum class EspNowSendStatus : int8_t {
  None = -1,
  Success = 0,
  DeliveryFailed = 1,
  ImmediateFailed = 2,
};

struct EspNowTxDiagnostics {
  uint32_t send_attempts{};
  uint32_t send_successes{};
  uint32_t send_failures{};
  EspNowSendStatus last_status{EspNowSendStatus::None};
};

class EspNowTransport {
public:
  bool begin();
  void tick(
      const EngineData &engine_data,
      const RuntimeMetrics &runtime_metrics,
      uint32_t now_ms);
  void takeDiagnostics(EspNowTxDiagnostics &diagnostics);

private:
  static void onSend(
      const esp_now_send_info_t *info,
      esp_now_send_status_t status);
  void recordSendResult(esp_now_send_status_t status);

  static EspNowTransport *instance_;

  portMUX_TYPE mutex_ = portMUX_INITIALIZER_UNLOCKED;
  bool initialized_{};
  bool fuel_used_valid_{};
  bool trip_time_valid_{};
  uint32_t last_send_ms_{};
  uint32_t sequence_{};
  uint32_t boot_id_{};
  uint32_t send_attempts_{};
  uint32_t send_successes_{};
  uint32_t send_failures_{};
  EspNowSendStatus last_status_{EspNowSendStatus::None};
};

} // namespace craftbridge
