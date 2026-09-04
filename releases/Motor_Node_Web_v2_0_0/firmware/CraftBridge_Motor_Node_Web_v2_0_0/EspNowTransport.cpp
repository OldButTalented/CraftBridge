#include "EspNowTransport.h"

#include "Config.h"

#include <esp_now.h>
#include <esp_random.h>

#include <cstring>

namespace craftbridge {
namespace {

constexpr uint8_t kRpmValid = 1u << 0;
constexpr uint8_t kCoolantTemperatureValid = 1u << 1;
constexpr uint8_t kRuntimeHoursValid = 1u << 2;
constexpr uint8_t kOilPressureValid = 1u << 3;
constexpr uint8_t kBatteryVoltageValid = 1u << 4;
constexpr uint8_t kFuelFlowValid = 1u << 5;
constexpr uint8_t kFuelUsedValid = 1u << 6;
constexpr uint8_t kTripTimeValid = 1u << 7;

bool isFresh(const SignalState &state, uint32_t now_ms) {
  return state.quality(now_ms) == SignalQuality::Fresh;
}

} // namespace

EspNowTransport *EspNowTransport::instance_ = nullptr;

bool EspNowTransport::begin() {
  if (!config::kEspNowEnabled) {
    return true;
  }

  if (esp_now_init() != ESP_OK) {
    return false;
  }

  esp_now_peer_info_t peer{};
  std::memcpy(
      peer.peer_addr,
      config::kEspNowPeerMac,
      sizeof(config::kEspNowPeerMac));
  peer.channel = config::kWifiChannel;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    esp_now_deinit();
    return false;
  }

  instance_ = this;
  if (esp_now_register_send_cb(&EspNowTransport::onSend) != ESP_OK) {
    instance_ = nullptr;
    esp_now_deinit();
    return false;
  }

  boot_id_ = esp_random();
  initialized_ = true;
  return true;
}

void EspNowTransport::tick(
    const EngineData &engine_data,
    const RuntimeMetrics &runtime_metrics,
    uint32_t now_ms) {
  if (!config::kEspNowEnabled || !initialized_ ||
      now_ms - last_send_ms_ < config::kEspNowSendIntervalMs) {
    return;
  }
  last_send_ms_ = now_ms;

  const bool rpm_fresh = isFresh(engine_data.rpm_state, now_ms);
  const bool coolant_fresh =
      isFresh(engine_data.coolant_temperature_state, now_ms);
  const bool runtime_fresh = isFresh(engine_data.runtime_state, now_ms);
  const bool oil_fresh = isFresh(engine_data.oil_pressure_state, now_ms);
  const bool battery_fresh =
      isFresh(engine_data.battery_voltage_state, now_ms);
  const bool fuel_flow_fresh =
      isFresh(engine_data.fuel_flow_state, now_ms);

  fuel_used_valid_ = fuel_used_valid_ || fuel_flow_fresh;
  trip_time_valid_ = trip_time_valid_ || rpm_fresh;

  uint8_t validity_mask = 0;
  validity_mask |= rpm_fresh ? kRpmValid : 0;
  validity_mask |= coolant_fresh ? kCoolantTemperatureValid : 0;
  validity_mask |= runtime_fresh ? kRuntimeHoursValid : 0;
  validity_mask |= oil_fresh ? kOilPressureValid : 0;
  validity_mask |= battery_fresh ? kBatteryVoltageValid : 0;
  validity_mask |= fuel_flow_fresh ? kFuelFlowValid : 0;
  validity_mask |= fuel_used_valid_ ? kFuelUsedValid : 0;
  validity_mask |= trip_time_valid_ ? kTripTimeValid : 0;

  EspNowPacket packet{};
  packet.protocol_version = 1;
  packet.validity_mask = validity_mask;
  packet.sequence = sequence_++;
  packet.boot_id = boot_id_;
  packet.rpm = engine_data.rpm;
  packet.coolant_temperature_c =
      static_cast<int16_t>(engine_data.coolant_temperature_c);
  packet.runtime_hours = engine_data.runtime_hours;
  packet.oil_pressure_kpa = engine_data.oil_pressure_kpa;
  packet.battery_voltage_v = engine_data.battery_voltage_v;
  packet.fuel_flow_lph = engine_data.fuel_flow_lph;
  packet.fuel_used_l =
      static_cast<float>(runtime_metrics.fuelUsedLiters());
  packet.trip_time_s =
      static_cast<uint32_t>(runtime_metrics.tripTimeMs() / 1000u);

  portENTER_CRITICAL(&mutex_);
  ++send_attempts_;
  portEXIT_CRITICAL(&mutex_);

  if (esp_now_send(
          config::kEspNowPeerMac,
          reinterpret_cast<const uint8_t *>(&packet),
          sizeof(packet)) != ESP_OK) {
    portENTER_CRITICAL(&mutex_);
    ++send_failures_;
    last_status_ = EspNowSendStatus::ImmediateFailed;
    portEXIT_CRITICAL(&mutex_);
  }
}

void EspNowTransport::onSend(
    const esp_now_send_info_t *,
    esp_now_send_status_t status) {
  if (instance_ != nullptr) {
    instance_->recordSendResult(status);
  }
}

void EspNowTransport::recordSendResult(esp_now_send_status_t status) {
  portENTER_CRITICAL(&mutex_);
  if (status == ESP_NOW_SEND_SUCCESS) {
    ++send_successes_;
    last_status_ = EspNowSendStatus::Success;
  } else {
    ++send_failures_;
    last_status_ = EspNowSendStatus::DeliveryFailed;
  }
  portEXIT_CRITICAL(&mutex_);
}

void EspNowTransport::takeDiagnostics(EspNowTxDiagnostics &diagnostics) {
  portENTER_CRITICAL(&mutex_);
  diagnostics.send_attempts = send_attempts_;
  diagnostics.send_successes = send_successes_;
  diagnostics.send_failures = send_failures_;
  diagnostics.last_status = last_status_;
  portEXIT_CRITICAL(&mutex_);
}

} // namespace craftbridge
