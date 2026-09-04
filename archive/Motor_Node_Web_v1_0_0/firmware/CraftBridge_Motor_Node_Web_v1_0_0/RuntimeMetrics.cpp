#include "RuntimeMetrics.h"

namespace craftbridge {

void RuntimeMetrics::update(
    const EngineData &engine_data,
    uint32_t now_ms) {
  if (!initialized_) {
    initialized_ = true;
    last_update_ms_ = now_ms;
    return;
  }

  // Unsigned subtraction remains correct when the 32-bit millisecond counter
  // wraps. The actual elapsed interval is used instead of assuming loop timing.
  const uint32_t elapsed_ms = now_ms - last_update_ms_;
  last_update_ms_ = now_ms;

  if (engine_data.fuel_flow_state.quality(now_ms) ==
      SignalQuality::Fresh) {
    fuel_used_liters_ +=
        static_cast<double>(engine_data.fuel_flow_lph) *
        static_cast<double>(elapsed_ms) / 3600000.0;
  }

  if (engine_data.rpm_state.quality(now_ms) == SignalQuality::Fresh &&
      engine_data.rpm > 0) {
    trip_time_ms_ += elapsed_ms;
  }
}

double RuntimeMetrics::fuelUsedLiters() const {
  return fuel_used_liters_;
}

uint64_t RuntimeMetrics::tripTimeMs() const {
  return trip_time_ms_;
}

} // namespace craftbridge
