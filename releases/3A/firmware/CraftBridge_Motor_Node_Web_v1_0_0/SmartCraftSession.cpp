#include "SmartCraftSession.h"

#include <algorithm>

namespace craftbridge {
namespace {

constexpr uint32_t kClientId = 0x00000B73;
constexpr uint32_t kEcuId = 0x0000730B;

uint32_t readUint32BigEndian(const uint8_t *bytes) {
  return (static_cast<uint32_t>(bytes[0]) << 24) |
         (static_cast<uint32_t>(bytes[1]) << 16) |
         (static_cast<uint32_t>(bytes[2]) << 8) |
         bytes[3];
}

uint32_t transformChallenge(
    uint32_t challenge,
    uint32_t multiplier,
    uint32_t xor_value) {
  const uint64_t product =
      static_cast<uint64_t>(challenge) * multiplier;
  return static_cast<uint32_t>(product) ^ xor_value;
}

enum class Dynamic {
  None,
  Fa0401,
  Fa0206,
  Sel8004,
};

struct Step {
  uint32_t id;
  uint8_t dlc;
  std::array<uint8_t, 8> payload;
  bool gate;
  uint8_t response_dlc;
  std::array<uint8_t, 8> response;
  bool challenge;
  Dynamic dynamic;
};

// Fixed startup for the tested SmartCraft implementation: 30 transmissions,
// 27 response gates, and three responses calculated from live challenges.
// Every gate must match before the sequence can continue.
#define P(...) std::array<uint8_t, 8>{__VA_ARGS__}
constexpr std::array<Step, 30> kSteps{{
    {kClientId, 1, P(0x55), true, 1, P(0xAA), false, Dynamic::None},
    {kClientId, 2, P(0xC0, 0x00), true, 1, P(0x1B), false, Dynamic::None},
    {kClientId, 2, P(0xC0, 0x01), true, 1, P(0x03), false, Dynamic::None},
    {kClientId, 2, P(0xC0, 0x06), true, 1, P(0x0C), false, Dynamic::None},
    {kClientId, 2, P(0xC0, 0x05), true, 1, P(0x0A), false, Dynamic::None},
    {kClientId, 3, P(0xFA, 0x04, 0x01), true, 4, P(), true, Dynamic::None},
    {kClientId, 5, P(), true, 2, P(0x04, 0x01), false, Dynamic::Fa0401},
    {kClientId, 5, P(0x06, 0x00, 0x0C, 0x00, 0x00), true, 3, P(0x0C, 0x00, 0x00), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x4D, 0x59, 0x32, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x30, 0x36, 0x70, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x41, 0x41, 0x41, 0x49), false, Dynamic::None},
    {kClientId, 2, P(0x00, 0x01), true, 1, P(0x00), false, Dynamic::None},
    {kClientId, 5, P(0x06, 0x00, 0x0D, 0x00, 0x00), true, 3, P(0x0D, 0x00, 0x00), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x4D, 0x59, 0x32, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x30, 0x36, 0x70, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x41, 0x41, 0x41, 0x49), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x5F, 0x30, 0x39, 0x5F), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x33, 0x63, 0x79, 0x6C), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x34, 0x30, 0x5F, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x31, 0x5F, 0x30, 0x30), false, Dynamic::None},
    {kClientId, 2, P(0x03, 0x01), true, 4, P(0x30, 0x00, 0x00, 0x00), false, Dynamic::None},
    {kClientId, 1, P(0x55), false, 0, P(), false, Dynamic::None},
    {kClientId, 1, P(0x55), true, 1, P(0xAA), false, Dynamic::None},
    {kClientId, 1, P(0x55), true, 1, P(0xAA), false, Dynamic::None},
    {0x1608B073, 8, P(0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF), false, 0, P(), false, Dynamic::None},
    {0x1608B173, 8, P(0x00, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF), false, 0, P(), false, Dynamic::None},
    {kClientId, 3, P(0xFA, 0x02, 0x06), true, 4, P(), true, Dynamic::None},
    {kClientId, 5, P(), true, 2, P(0x02, 0x06), false, Dynamic::Fa0206},
    {kClientId, 2, P(0x80, 0x04), true, 4, P(), true, Dynamic::None},
    {kClientId, 5, P(), true, 1, P(0x04), false, Dynamic::Sel8004},
}};
#undef P

uint32_t elapsed(uint32_t now_ms, uint32_t then_ms) {
  return now_ms - then_ms;
}

uint16_t pageBit(uint8_t page) {
  const uint8_t bit = page == 0xFF ? 15 : page;
  return static_cast<uint16_t>(1u << bit);
}

bool isS0Page(uint32_t id, uint8_t page) {
  const bool family_170 =
      id == 0x170 &&
      (page == 0x00 ||
       page == 0x03 ||
       page == 0x06 ||
       page == 0xFF);
  const bool family_1a0 =
      id == 0x1A0 &&
      (page == 0x01 || page == 0xFF);
  return family_170 || family_1a0;
}

} // namespace

uint32_t transform_fa_0401(uint32_t challenge) {
  return transformChallenge(challenge, 0xD379A9C8, 0x1B4610CA);
}

uint32_t transform_fa_0206(uint32_t challenge) {
  return transformChallenge(challenge, 0xCF88B813, 0x4353E4D3);
}

uint32_t transform_80_04(uint32_t challenge) {
  return transformChallenge(challenge, 0xAB20FA1B, 0x208FB01A);
}

const EngineData &SmartCraftSession::data() const {
  return engine_data_;
}

const SessionStatus &SmartCraftSession::status() const {
  return status_;
}

size_t SmartCraftSession::startupTxPosition() const {
  return tx_position_;
}

void SmartCraftSession::stopApplicationTx() {
  status_.application_tx_enabled = false;
}

void SmartCraftSession::beginLifecycle(uint32_t now_ms) {
  lifecycle_active_ = true;
  lifecycle_start_ms_ = now_ms;
  s0_170_mask_ = 0;
  s0_1a0_mask_ = 0;
  s3_170_mask_ = 0;
  s3_1a0_mask_ = 0;
  s3_170_ms_.fill(0);
  s3_1a0_ms_.fill(0);
  status_.state = SessionState::PassiveStartupWait;
  status_.source = SessionSource::None;
  status_.s0_safe = true;
  status_.s3_complete = false;
  stopApplicationTx();
  awaiting_producers_ = false;
  tx_position_ = 0;
}

void SmartCraftSession::noteS0(const CanFrame &frame) {
  if (status_.state != SessionState::PassiveStartupWait ||
      frame.remote) {
    return;
  }

  if (frame.extended ||
      frame.dlc == 0 ||
      frame.dlc > 8 ||
      !isS0Page(frame.id, frame.data[0])) {
    status_.s0_safe = false;
    return;
  }

  if (frame.id == 0x170) {
    s0_170_mask_ |= pageBit(frame.data[0]);
  } else {
    s0_1a0_mask_ |= pageBit(frame.data[0]);
  }
}

void SmartCraftSession::noteS3(
    uint32_t id,
    uint8_t page,
    uint32_t now_ms) {
  const uint8_t bit = page == 0xFF ? 15 : page;
  if (bit > 15) {
    return;
  }

  const uint16_t mask = static_cast<uint16_t>(1u << bit);
  if (id == 0x170 && (kRequired170Mask & mask)) {
    s3_170_mask_ |= mask;
    s3_170_ms_[bit] = now_ms;
    status_.last_required_producer_ms = now_ms;
  } else if (id == 0x1A0 && (kRequired1A0Mask & mask)) {
    s3_1a0_mask_ |= mask;
    s3_1a0_ms_[bit] = now_ms;
    status_.last_required_producer_ms = now_ms;
  }
}

bool SmartCraftSession::s0Ready() const {
  const bool complete_170 =
      (s0_170_mask_ & 0x8049) == 0x8049;
  const bool complete_1a0 =
      (s0_1a0_mask_ & 0x8002) == 0x8002;
  return status_.s0_safe && complete_170 && complete_1a0;
}

bool SmartCraftSession::expandedFresh(uint32_t now_ms) const {
  if ((s3_170_mask_ & kRequired170Mask) != kRequired170Mask ||
      (s3_1a0_mask_ & kRequired1A0Mask) != kRequired1A0Mask) {
    return false;
  }

  for (uint8_t page = 0; page <= 6; ++page) {
    if (elapsed(now_ms, s3_170_ms_[page]) > kS3FreshnessMs) {
      return false;
    }
  }

  if (elapsed(now_ms, s3_170_ms_[15]) > kS3FreshnessMs) {
    return false;
  }

  for (uint8_t page = 0; page <= 12; ++page) {
    if (elapsed(now_ms, s3_1a0_ms_[page]) > kS3FreshnessMs) {
      return false;
    }
  }

  return elapsed(now_ms, s3_1a0_ms_[15]) <= kS3FreshnessMs;
}

void SmartCraftSession::acceptExternalSession() {
  status_.state = SessionState::ExternalSessionActive;
  status_.source = SessionSource::External;
  status_.s3_complete = true;
  stopApplicationTx();
  awaiting_producers_ = false;
  tx_position_ = 0;
}

void SmartCraftSession::onFrame(
    const CanFrame &frame,
    uint32_t now_ms) {
  if (frame.remote || frame.dlc > 8) {
    return;
  }

  engine_data_.decodeFrame(frame, now_ms);

  if (!frame.extended &&
      frame.dlc > 0 &&
      frame.id == 0x170 &&
      frame.data[0] == 0x00) {
    baseline_seen_ = true;
    baseline_last_ms_ = now_ms;

    if (!lifecycle_active_) {
      beginLifecycle(now_ms);
    }
  }

  if (!lifecycle_active_) {
    return;
  }

  noteS0(frame);

  if (!frame.extended && frame.dlc > 0) {
    noteS3(frame.id, frame.data[0], now_ms);
  }

  if (expandedFresh(now_ms)) {
    if (status_.state == SessionState::PassiveStartupWait) {
      acceptExternalSession();
    } else if (awaiting_producers_) {
      awaiting_producers_ = false;
      status_.state = SessionState::StandaloneSessionActive;
      status_.source = SessionSource::Standalone;
      status_.s3_complete = true;
      ++status_.startup_successes;
    }
  }

  if (frame.extended &&
      status_.state == SessionState::StandaloneSessionStarting) {
    if (frame.id == kEcuId) {
      handleDirectedResponse(frame, now_ms);
    } else {
      failStartup();
    }
  }
}

void SmartCraftSession::beginStartup(uint32_t now_ms) {
  ++status_.startup_attempts;
  status_.source = SessionSource::None;
  status_.state = SessionState::StandaloneSessionStarting;
  status_.application_tx_enabled = true;
  status_.s3_complete = false;
  tx_position_ = 0;
  awaiting_producers_ = false;
  s3_170_mask_ = 0;
  s3_1a0_mask_ = 0;
  s3_170_ms_.fill(0);
  s3_1a0_ms_.fill(0);
  sendUntilGate(now_ms);
}

void SmartCraftSession::sendUntilGate(uint32_t now_ms) {
  while (tx_position_ < kSteps.size()) {
    const Step &step = kSteps[tx_position_];

    if (step.dynamic != Dynamic::None) {
      failStartup();
      return;
    }

    CanFrame frame{};
    frame.id = step.id;
    frame.extended = true;
    frame.dlc = step.dlc;
    frame.data = step.payload;

    if (!frame_sender_.sendVerifiedStartupFrame(frame)) {
      failStartup();
      return;
    }

    ++tx_position_;
    ++status_.startup_tx_count;

    if (step.gate) {
      gate_deadline_ms_ = now_ms + kResponseGateTimeoutMs;
      return;
    }
  }
}

void SmartCraftSession::handleDirectedResponse(
    const CanFrame &frame,
    uint32_t now_ms) {
  if (awaiting_producers_ ||
      tx_position_ == 0 ||
      tx_position_ > kSteps.size()) {
    return;
  }

  const Step &sent_step = kSteps[tx_position_ - 1];
  const bool fixed_response_matches =
      std::equal(
          frame.data.begin(),
          frame.data.begin() + frame.dlc,
          sent_step.response.begin());

  if (!sent_step.gate ||
      frame.dlc != sent_step.response_dlc ||
      (!sent_step.challenge && !fixed_response_matches)) {
    failStartup();
    return;
  }

  if (sent_step.challenge) {
    const uint32_t challenge =
        readUint32BigEndian(frame.data.data());

    if (tx_position_ >= kSteps.size()) {
      failStartup();
      return;
    }

    const Step &dynamic_step = kSteps[tx_position_];
    uint32_t response_value{};
    uint8_t opcode{};

    switch (dynamic_step.dynamic) {
      case Dynamic::Fa0401:
        response_value = transform_fa_0401(challenge);
        opcode = 0xF9;
        break;
      case Dynamic::Fa0206:
        response_value = transform_fa_0206(challenge);
        opcode = 0xF9;
        break;
      case Dynamic::Sel8004:
        response_value = transform_80_04(challenge);
        opcode = 0x81;
        break;
      case Dynamic::None:
        failStartup();
        return;
    }

    CanFrame response{};
    response.id = dynamic_step.id;
    response.extended = true;
    response.dlc = 5;
    response.data = {
        opcode,
        static_cast<uint8_t>(response_value >> 24),
        static_cast<uint8_t>(response_value >> 16),
        static_cast<uint8_t>(response_value >> 8),
        static_cast<uint8_t>(response_value),
    };

    if (!frame_sender_.sendVerifiedStartupFrame(response)) {
      failStartup();
      return;
    }

    ++tx_position_;
    ++status_.startup_tx_count;
    gate_deadline_ms_ = now_ms + kResponseGateTimeoutMs;
    return;
  }

  if (tx_position_ == kSteps.size()) {
    awaiting_producers_ = true;
    stopApplicationTx();
    gate_deadline_ms_ = now_ms + kS3FreshnessMs;
    return;
  }

  sendUntilGate(now_ms);
}

void SmartCraftSession::failStartup() {
  ++status_.startup_failures;
  status_.state = SessionState::StartupFailed;
  status_.source = SessionSource::None;
  status_.s3_complete = false;
  stopApplicationTx();
  awaiting_producers_ = false;
  tx_position_ = 0;
  engine_data_.invalidateExpanded();
}

void SmartCraftSession::tick(uint32_t now_ms) {
  engine_data_.expire(now_ms);

  if (baseline_seen_ &&
      elapsed(now_ms, baseline_last_ms_) > kBaselineLossTimeoutMs) {
    baseline_seen_ = false;
    lifecycle_active_ = false;
    status_.state = SessionState::PassiveStartupWait;
    status_.source = SessionSource::None;
    status_.s3_complete = false;
    stopApplicationTx();
    engine_data_.invalidateExpanded();
    return;
  }

  if (!lifecycle_active_) {
    return;
  }

  if (status_.state == SessionState::PassiveStartupWait &&
      expandedFresh(now_ms)) {
    acceptExternalSession();
    return;
  }

  if (status_.state == SessionState::PassiveStartupWait &&
      elapsed(now_ms, lifecycle_start_ms_) >= kStartupWaitMs) {
    if (s0Ready()) {
      beginStartup(now_ms);
    } else {
      status_.state = SessionState::StartupBlocked;
      stopApplicationTx();
    }
    return;
  }

  if (status_.state == SessionState::StandaloneSessionStarting &&
      static_cast<int32_t>(now_ms - gate_deadline_ms_) >= 0) {
    failStartup();
    return;
  }

  const bool active_session =
      status_.state == SessionState::ExternalSessionActive ||
      status_.state == SessionState::StandaloneSessionActive;
  if (active_session && !expandedFresh(now_ms)) {
    engine_data_.invalidateExpanded();
    status_.state = SessionState::SessionLost;
    status_.source = SessionSource::None;
    status_.s3_complete = false;
    stopApplicationTx();
  }
}

} // namespace craftbridge
