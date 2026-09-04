#pragma once

#include "CanBus.h"
#include "EngineData.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace craftbridge {

inline constexpr uint32_t kStartupWaitMs = 8000;
inline constexpr uint32_t kS3FreshnessMs = 5000;
inline constexpr uint32_t kResponseGateTimeoutMs = 250;
inline constexpr uint32_t kBaselineLossTimeoutMs = 2000;
inline constexpr uint16_t kRequired170Mask = 0x807F;
inline constexpr uint16_t kRequired1A0Mask = 0x9FFF;

enum class SessionState {
  PassiveStartupWait,
  ExternalSessionActive,
  StandaloneSessionStarting,
  StandaloneSessionActive,
  StartupBlocked,
  StartupFailed,
  SessionLost,
};

enum class SessionSource {
  None,
  External,
  Standalone,
};

struct SessionStatus {
  SessionState state{SessionState::PassiveStartupWait};
  SessionSource source{SessionSource::None};
  uint32_t startup_attempts{};
  uint32_t startup_successes{};
  uint32_t startup_failures{};
  uint32_t startup_tx_count{};
  uint32_t last_required_producer_ms{};
  bool s0_safe{};
  bool s3_complete{};
  bool application_tx_enabled{};
};

uint32_t transform_fa_0401(uint32_t challenge);
uint32_t transform_fa_0206(uint32_t challenge);
uint32_t transform_80_04(uint32_t challenge);

// SmartCraftSession owns all startup authorization and session supervision.
// Application transmission is enabled only while the fixed startup sequence is
// active. Failure and session loss are terminal until a new ignition lifecycle.
class SmartCraftSession {
public:
  explicit SmartCraftSession(StartupFrameSender &frame_sender)
      : frame_sender_(frame_sender) {}

  void onFrame(const CanFrame &frame, uint32_t now_ms);
  void tick(uint32_t now_ms);

  const EngineData &data() const;
  const SessionStatus &status() const;
  size_t startupTxPosition() const;

private:
  void beginLifecycle(uint32_t now_ms);
  void beginStartup(uint32_t now_ms);
  void sendUntilGate(uint32_t now_ms);
  void failStartup();
  void handleDirectedResponse(const CanFrame &frame, uint32_t now_ms);
  void noteS0(const CanFrame &frame);
  void noteS3(uint32_t id, uint8_t page, uint32_t now_ms);
  bool s0Ready() const;
  bool expandedFresh(uint32_t now_ms) const;
  void acceptExternalSession();
  void stopApplicationTx();

  StartupFrameSender &frame_sender_;
  EngineData engine_data_{};
  SessionStatus status_{};
  bool lifecycle_active_{};
  bool baseline_seen_{};
  bool awaiting_producers_{};
  uint32_t lifecycle_start_ms_{};
  uint32_t baseline_last_ms_{};
  uint16_t s0_170_mask_{};
  uint16_t s0_1a0_mask_{};
  uint16_t s3_170_mask_{};
  uint16_t s3_1a0_mask_{};
  std::array<uint32_t, 16> s3_170_ms_{};
  std::array<uint32_t, 16> s3_1a0_ms_{};
  size_t tx_position_{};
  uint32_t gate_deadline_ms_{};
};

} // namespace craftbridge
