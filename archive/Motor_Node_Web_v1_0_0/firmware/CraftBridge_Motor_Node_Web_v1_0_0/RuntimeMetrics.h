#pragma once

#include "EngineData.h"

#include <cstdint>

namespace craftbridge {

// RuntimeMetrics contains only boot-scoped values derived from fresh engine
// data. It deliberately has no persistence or reset interface.
class RuntimeMetrics {
public:
  void update(const EngineData &engine_data, uint32_t now_ms);

  double fuelUsedLiters() const;
  uint64_t tripTimeMs() const;

private:
  bool initialized_{};
  uint32_t last_update_ms_{};
  double fuel_used_liters_{};
  uint64_t trip_time_ms_{};
};

} // namespace craftbridge
