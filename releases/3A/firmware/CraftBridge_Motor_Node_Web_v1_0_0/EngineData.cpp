#include "EngineData.h"

#include <climits>

namespace craftbridge {
namespace {

// SmartCraft multi-byte values in this implementation are big-endian.
uint16_t readUint16BigEndian(const uint8_t *bytes) {
  return static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
}

} // namespace

// -----------------------------------------------------------------------------
// Signal validity and freshness
// -----------------------------------------------------------------------------

uint32_t SignalState::ageMs(uint32_t now_ms) const {
  return seen ? now_ms - updated_ms : UINT32_MAX;
}

SignalQuality SignalState::quality(uint32_t now_ms) const {
  if (invalid_value) {
    return SignalQuality::Invalid;
  }

  if (valid && now_ms - updated_ms <= kSignalFreshnessMs) {
    return SignalQuality::Fresh;
  }

  return seen ? SignalQuality::Stale : SignalQuality::Missing;
}

void SignalState::markFresh(uint32_t now_ms) {
  valid = true;
  seen = true;
  invalid_value = false;
  updated_ms = now_ms;
}

void SignalState::markStale() {
  valid = false;
  invalid_value = false;
}

void SignalState::markInvalid(uint32_t now_ms) {
  valid = false;
  seen = true;
  invalid_value = true;
  updated_ms = now_ms;
}

// -----------------------------------------------------------------------------
// Decoding and scaling
// -----------------------------------------------------------------------------

DecodeResult EngineData::decodeFrame(const CanFrame &frame, uint32_t now_ms) {
  // Remote, extended, and empty frames never contain supported engine signals.
  if (frame.remote || frame.extended || frame.dlc == 0) {
    return DecodeResult::Ignored;
  }

  const uint8_t page = frame.data[0];

  if (frame.id == 0x170 && page == 0x00 && frame.dlc >= 3) {
    rpm = readUint16BigEndian(&frame.data[1]);
    rpm_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  if (frame.id == 0x170 && page == 0x01 && frame.dlc >= 3) {
    fuel_flow_lph =
        readUint16BigEndian(&frame.data[1]) * 0.01f;
    fuel_flow_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  if (frame.id != 0x1A0) {
    return DecodeResult::Ignored;
  }

  if (page == 0x07 && frame.dlc >= 3) {
    coolant_temperature_c = frame.data[2];
    coolant_temperature_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  if (page == 0x02 && frame.dlc >= 5) {
    runtime_hours =
        readUint16BigEndian(&frame.data[3]) / 60.0f;
    runtime_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  if (page == 0x09 && frame.dlc >= 6) {
    battery_voltage_v =
        readUint16BigEndian(&frame.data[4]) * 0.001f;
    battery_voltage_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  if (page == 0x05 && frame.dlc >= 5) {
    oil_pressure_kpa =
        readUint16BigEndian(&frame.data[3]) * 0.01f;
    oil_pressure_state.markFresh(now_ms);
    return DecodeResult::Decoded;
  }

  return DecodeResult::Ignored;
}

// -----------------------------------------------------------------------------
// Expiration and session invalidation
// -----------------------------------------------------------------------------

void EngineData::expire(uint32_t now_ms) {
  if (!rpm_state.invalid_value &&
      rpm_state.quality(now_ms) == SignalQuality::Stale) {
    rpm_state.markStale();
  }

  if (!coolant_temperature_state.invalid_value &&
      coolant_temperature_state.quality(now_ms) == SignalQuality::Stale) {
    coolant_temperature_state.markStale();
  }

  if (!runtime_state.invalid_value &&
      runtime_state.quality(now_ms) == SignalQuality::Stale) {
    runtime_state.markStale();
  }

  if (!oil_pressure_state.invalid_value &&
      oil_pressure_state.quality(now_ms) == SignalQuality::Stale) {
    oil_pressure_state.markStale();
  }

  if (!battery_voltage_state.invalid_value &&
      battery_voltage_state.quality(now_ms) == SignalQuality::Stale) {
    battery_voltage_state.markStale();
  }

  if (!fuel_flow_state.invalid_value &&
      fuel_flow_state.quality(now_ms) == SignalQuality::Stale) {
    fuel_flow_state.markStale();
  }
}

void EngineData::invalidateExpanded() {
  // Baseline RPM remains independent of expanded session loss.
  coolant_temperature_state.markStale();
  runtime_state.markStale();
  oil_pressure_state.markStale();
  battery_voltage_state.markStale();
  fuel_flow_state.markStale();
}

} // namespace craftbridge
