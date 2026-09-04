#pragma once

#include <array>
#include <cstdint>

namespace craftbridge {

class SmartCraftSession;

// Portable CAN frame used by the hardware adapter, decoder, and session logic.
struct CanFrame {
  uint32_t id{};
  bool extended{};
  bool remote{};
  uint8_t dlc{};
  std::array<uint8_t, 8> data{};
};

// Counters are diagnostic only and never authorize transmission.
struct CanDiagnostics {
  uint32_t rx_total{};
  uint32_t rx_standard{};
  uint32_t rx_extended{};
  uint32_t rx_rtr{};
  uint32_t rx_malformed{};
  uint32_t rx_missed{};
  uint32_t rx_overrun{};
  uint32_t bus_errors{};
  uint32_t tx_successful{};
  uint32_t tx_failed{};
};

// This narrow interface keeps the session logic host-testable. Only the
// session state machine can request a verified startup frame.
class StartupFrameSender {
public:
  virtual ~StartupFrameSender() = default;

private:
  virtual bool sendVerifiedStartupFrame(const CanFrame &frame) = 0;
  friend class SmartCraftSession;
};

class CanBus final : public StartupFrameSender {
public:
  bool begin();
  bool receive(CanFrame &frame);
  void updateDiagnostics();

  bool ready() const;
  const CanDiagnostics &diagnostics() const;

private:
  bool sendVerifiedStartupFrame(const CanFrame &frame) override;

  bool ready_{};
  CanDiagnostics diagnostics_{};
};

} // namespace craftbridge
