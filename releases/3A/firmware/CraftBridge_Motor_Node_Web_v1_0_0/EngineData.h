#pragma once

#include "CanBus.h"

#include <cstdint>

namespace craftbridge {

inline constexpr uint32_t kSignalFreshnessMs = 2000;

enum class SignalQuality {
  Missing,
  Fresh,
  Stale,
  Invalid,
};

enum class DecodeResult {
  Ignored,
  Decoded,
  InvalidValue,
};

// SignalState keeps validity and freshness separate from the typed value.
struct SignalState {
  bool valid{};
  bool seen{};
  bool invalid_value{};
  uint32_t updated_ms{};

  uint32_t ageMs(uint32_t now_ms) const;
  SignalQuality quality(uint32_t now_ms) const;
  void markFresh(uint32_t now_ms);
  void markStale();
  void markInvalid(uint32_t now_ms);
};

struct EngineData {
  uint16_t rpm{};
  uint8_t coolant_temperature_c{};
  float runtime_hours{};
  float oil_pressure_kpa{};
  float battery_voltage_v{};
  float fuel_flow_lph{};

  SignalState rpm_state{};
  SignalState coolant_temperature_state{};
  SignalState runtime_state{};
  SignalState oil_pressure_state{};
  SignalState battery_voltage_state{};
  SignalState fuel_flow_state{};

  DecodeResult decodeFrame(const CanFrame &frame, uint32_t now_ms);
  void expire(uint32_t now_ms);
  void invalidateExpanded();
};

} // namespace craftbridge
