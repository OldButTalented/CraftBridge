#include <unity.h>

#include "RuntimeMetrics.h"
#include "SmartCraftSession.h"
#include "WebInterface.h"

#include <initializer_list>
#include <string>
#include <vector>

using namespace craftbridge;

namespace {

CanFrame frame(
    uint32_t id,
    std::initializer_list<uint8_t> bytes,
    bool extended = false,
    bool remote = false) {
  CanFrame result{};
  result.id = id;
  result.extended = extended;
  result.remote = remote;
  result.dlc = static_cast<uint8_t>(bytes.size());

  size_t index = 0;
  for (uint8_t value : bytes) {
    result.data[index++] = value;
  }
  return result;
}

class CaptureSender final : public StartupFrameSender {
public:
  std::vector<CanFrame> sent;
  size_t fail_attempt{};
  size_t attempts{};

private:
  bool sendVerifiedStartupFrame(const CanFrame &outgoing) override {
    ++attempts;
    if (fail_attempt != 0 && attempts == fail_attempt) {
      return false;
    }

    sent.push_back(outgoing);
    return true;
  }
};

void baseline(
    SmartCraftSession &session,
    uint32_t now_ms = 0,
    bool remote = false) {
  session.onFrame(
      frame(0x170, {0x00, 0x03, 0xE8}, false, remote),
      now_ms);
}

void s0(SmartCraftSession &session, uint32_t now_ms) {
  for (uint8_t page : {
           uint8_t(0x03),
           uint8_t(0x06),
           uint8_t(0xFF)}) {
    session.onFrame(frame(0x170, {page, 0, 0}), now_ms);
  }

  for (uint8_t page : {
           uint8_t(0x01),
           uint8_t(0xFF)}) {
    session.onFrame(frame(0x1A0, {page, 0, 0}), now_ms);
  }
}

void fullS3(SmartCraftSession &session, uint32_t now_ms) {
  for (uint8_t page = 0; page <= 6; ++page) {
    session.onFrame(
        frame(0x170, {page, 0, 40, 0, 0, 0, 0, 0}),
        now_ms);
  }
  session.onFrame(
      frame(0x170, {0xFF, 0, 0, 0, 0, 0, 0, 0}),
      now_ms);

  for (uint8_t page = 0; page <= 12; ++page) {
    session.onFrame(
        frame(0x1A0, {page, 0, 75, 0, 0, 0, 0, 0}),
        now_ms);
  }
  session.onFrame(
      frame(0x1A0, {0xFF, 0, 0, 0, 0, 0, 0, 0}),
      now_ms);
}

const std::vector<CanFrame> gates = {
    frame(0x730B, {0xAA}, true),
    frame(0x730B, {0x1B}, true),
    frame(0x730B, {0x03}, true),
    frame(0x730B, {0x0C}, true),
    frame(0x730B, {0x0A}, true),
    frame(0x730B, {0xFE, 0x3A, 0x8C, 0x49}, true),
    frame(0x730B, {0x04, 0x01}, true),
    frame(0x730B, {0x0C, 0, 0}, true),
    frame(0x730B, {0x4D, 0x59, 0x32, 0x30}, true),
    frame(0x730B, {0x30, 0x36, 0x70, 0x30}, true),
    frame(0x730B, {0x41, 0x41, 0x41, 0x49}, true),
    frame(0x730B, {0}, true),
    frame(0x730B, {0x0D, 0, 0}, true),
    frame(0x730B, {0x4D, 0x59, 0x32, 0x30}, true),
    frame(0x730B, {0x30, 0x36, 0x70, 0x30}, true),
    frame(0x730B, {0x41, 0x41, 0x41, 0x49}, true),
    frame(0x730B, {0x5F, 0x30, 0x39, 0x5F}, true),
    frame(0x730B, {0x33, 0x63, 0x79, 0x6C}, true),
    frame(0x730B, {0x34, 0x30, 0x5F, 0x30}, true),
    frame(0x730B, {0x31, 0x5F, 0x30, 0x30}, true),
    frame(0x730B, {0x30, 0, 0, 0}, true),
    frame(0x730B, {0xAA}, true),
    frame(0x730B, {0xAA}, true),
    frame(0x730B, {0xE3, 0x1A, 0xFA, 0x69}, true),
    frame(0x730B, {0x02, 0x06}, true),
    frame(0x730B, {0x79, 0x2E, 0xCF, 0xD1}, true),
    frame(0x730B, {0x04}, true),
};

uint32_t start(SmartCraftSession &session) {
  baseline(session, 0);
  s0(session, 1);
  baseline(session, kStartupWaitMs - 1);
  session.tick(kStartupWaitMs);
  return kStartupWaitMs + 1;
}

uint32_t complete(
    SmartCraftSession &session,
    uint32_t now_ms) {
  for (const CanFrame &gate : gates) {
    session.onFrame(gate, now_ms++);
  }
  return now_ms;
}

uint32_t sequenceHash(const std::vector<CanFrame> &frames) {
  uint32_t hash = 2166136261u;

  for (const CanFrame &item : frames) {
    for (int shift = 24; shift >= 0; shift -= 8) {
      hash ^= static_cast<uint8_t>(item.id >> shift);
      hash *= 16777619u;
    }

    hash ^= item.dlc;
    hash *= 16777619u;

    for (uint8_t index = 0; index < item.dlc; ++index) {
      hash ^= item.data[index];
      hash *= 16777619u;
    }
  }

  return hash;
}

} // namespace

void test_decoders_scaling() {
  EngineData data{};

  data.decodeFrame(frame(0x170, {0, 0x0D, 0xAC}), 10);
  TEST_ASSERT_EQUAL_UINT16(3500, data.rpm);

  data.decodeFrame(frame(0x1A0, {7, 0, 83}), 20);
  TEST_ASSERT_EQUAL_UINT8(83, data.coolant_temperature_c);

  data.decodeFrame(frame(0x1A0, {2, 0, 0, 0x0E, 0x10}), 30);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.0f, data.runtime_hours);

  data.decodeFrame(frame(0x1A0, {9, 0, 0, 0, 0x30, 0xD4}), 40);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.5f, data.battery_voltage_v);

  data.decodeFrame(frame(0x170, {1, 0, 50}), 50);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, data.fuel_flow_lph);
}

void test_numeric_oil_and_quality() {
  EngineData data{};

  data.decodeFrame(frame(0x1A0, {5, 0, 0, 0x9B, 0x82}), 1);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 398.10f, data.oil_pressure_kpa);
  TEST_ASSERT_EQUAL(
      SignalQuality::Fresh,
      data.oil_pressure_state.quality(2));

  data.decodeFrame(frame(0x1A0, {5, 0, 0, 0x00, 0x01}), 3);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.01f, data.oil_pressure_kpa);
  TEST_ASSERT_EQUAL(
      SignalQuality::Fresh,
      data.oil_pressure_state.quality(4));

  data.decodeFrame(frame(0x1A0, {5, 0, 0, 0xFF, 0xFF}), 5);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 655.35f, data.oil_pressure_kpa);
  TEST_ASSERT_EQUAL(
      SignalQuality::Fresh,
      data.oil_pressure_state.quality(6));
}

void test_fresh_stale_missing() {
  EngineData data{};
  TEST_ASSERT_EQUAL(
      SignalQuality::Missing,
      data.rpm_state.quality(0));

  data.decodeFrame(frame(0x170, {0, 0, 1}), 1);
  TEST_ASSERT_EQUAL(
      SignalQuality::Fresh,
      data.rpm_state.quality(2));

  data.expire(kSignalFreshnessMs + 2);
  TEST_ASSERT_EQUAL(
      SignalQuality::Stale,
      data.rpm_state.quality(kSignalFreshnessMs + 2));
}

void test_malformed_and_rtr() {
  EngineData data{};

  TEST_ASSERT_EQUAL(
      DecodeResult::Ignored,
      data.decodeFrame(frame(0x170, {0, 1}), 1));
  TEST_ASSERT_EQUAL(
      DecodeResult::Ignored,
      data.decodeFrame(frame(0x170, {0, 0, 1}, false, true), 1));
  TEST_ASSERT_FALSE(data.rpm_state.seen);
}

void test_transforms() {
  TEST_ASSERT_EQUAL_HEX32(
      0xA69CDAC2,
      transform_fa_0401(0xFE3A8C49));
  TEST_ASSERT_EQUAL_HEX32(
      0x6E97E918,
      transform_fa_0206(0xE31AFA69));
  TEST_ASSERT_EQUAL_HEX32(
      0x348DB511,
      transform_80_04(0x792ECFD1));
}

void test_safe_idle_incomplete_s0_zero_tx() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  baseline(session, 0);
  baseline(session, kStartupWaitMs - 1);
  session.tick(kStartupWaitMs);

  TEST_ASSERT_EQUAL_UINT32(0, sender.sent.size());
  TEST_ASSERT_EQUAL(
      SessionState::StartupBlocked,
      session.status().state);
  TEST_ASSERT_FALSE(session.status().application_tx_enabled);
}

void test_unexpected_s0_blocks_tx() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  baseline(session, 0);
  s0(session, 1);
  session.onFrame(frame(0x123, {0}), 2);
  baseline(session, kStartupWaitMs - 1);
  session.tick(kStartupWaitMs);

  TEST_ASSERT_EQUAL_UINT32(0, sender.sent.size());
  TEST_ASSERT_EQUAL(
      SessionState::StartupBlocked,
      session.status().state);
}

void test_external_full_s3_zero_tx() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  baseline(session, 0);
  fullS3(session, 100);
  session.tick(101);

  TEST_ASSERT_EQUAL(
      SessionState::ExternalSessionActive,
      session.status().state);
  TEST_ASSERT_TRUE(session.status().s3_complete);
  TEST_ASSERT_EQUAL_UINT32(0, sender.sent.size());
}

void test_exact_startup_sequence() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  const uint32_t now_ms = start(session);
  complete(session, now_ms);

  TEST_ASSERT_EQUAL_UINT32(30, sender.sent.size());
  TEST_ASSERT_EQUAL_HEX32(0x7B13B034, sequenceHash(sender.sent));
  TEST_ASSERT_EQUAL_UINT32(30, session.status().startup_tx_count);
  TEST_ASSERT_FALSE(session.status().application_tx_enabled);
}

void test_full_s3_required_for_success() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  const uint32_t now_ms = complete(session, start(session));
  session.onFrame(frame(0x170, {1, 0, 1}), now_ms);
  session.onFrame(frame(0x1A0, {2, 0, 0, 0, 1}), now_ms);

  TEST_ASSERT_EQUAL(
      SessionState::StandaloneSessionStarting,
      session.status().state);

  fullS3(session, now_ms + 1);
  TEST_ASSERT_EQUAL(
      SessionState::StandaloneSessionActive,
      session.status().state);
  TEST_ASSERT_TRUE(session.status().s3_complete);
}

void test_gate_timeout_250ms_terminal() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  start(session);
  session.tick(kStartupWaitMs + kResponseGateTimeoutMs);

  TEST_ASSERT_EQUAL(
      SessionState::StartupFailed,
      session.status().state);

  const size_t sent_count = sender.sent.size();
  session.tick(50000);
  TEST_ASSERT_EQUAL_UINT32(sent_count, sender.sent.size());
  TEST_ASSERT_FALSE(session.status().application_tx_enabled);
}

void test_tx_failure_terminal() {
  CaptureSender sender;
  sender.fail_attempt = 2;
  SmartCraftSession session(sender);

  const uint32_t now_ms = start(session);
  session.onFrame(gates[0], now_ms);

  TEST_ASSERT_EQUAL(
      SessionState::StartupFailed,
      session.status().state);

  const size_t sent_count = sender.sent.size();
  session.tick(50000);
  TEST_ASSERT_EQUAL_UINT32(sent_count, sender.sent.size());
}

void test_session_loss_no_replay() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  const uint32_t now_ms = complete(session, start(session));
  fullS3(session, now_ms);

  const size_t sent_count = sender.sent.size();
  baseline(session, now_ms + kS3FreshnessMs);
  session.tick(now_ms + kS3FreshnessMs + 1);

  TEST_ASSERT_EQUAL(
      SessionState::SessionLost,
      session.status().state);

  session.tick(now_ms + 50000);
  TEST_ASSERT_EQUAL_UINT32(sent_count, sender.sent.size());
}

void test_new_ignition_lifecycle_allows_new_start() {
  CaptureSender sender;
  SmartCraftSession session(sender);

  const uint32_t now_ms = complete(session, start(session));
  fullS3(session, now_ms);
  session.tick(now_ms + kBaselineLossTimeoutMs + 1);
  baseline(session, now_ms + kBaselineLossTimeoutMs + 2);
  s0(session, now_ms + kBaselineLossTimeoutMs + 3);
  baseline(
      session,
      now_ms + kBaselineLossTimeoutMs + kStartupWaitMs + 1);
  session.tick(
      now_ms + kBaselineLossTimeoutMs + kStartupWaitMs + 2);

  TEST_ASSERT_TRUE(sender.sent.size() > 30);
}

void test_web_interface_simulated_data() {
  EngineData data{};
  data.rpm = 3500;
  data.coolant_temperature_c = 83;
  data.runtime_hours = 60.0f;
  data.oil_pressure_kpa = 398.10f;
  data.battery_voltage_v = 12.5f;
  data.fuel_flow_lph = 36.0f;
  data.rpm_state.markFresh(1000);
  data.coolant_temperature_state.markFresh(1000);
  data.runtime_state.markFresh(1000);
  data.oil_pressure_state.markFresh(1000);
  data.battery_voltage_state.markFresh(1000);
  data.fuel_flow_state.markFresh(1000);

  SessionStatus session_status{};
  session_status.state = SessionState::StandaloneSessionActive;
  session_status.source = SessionSource::Standalone;
  session_status.s3_complete = true;

  CanDiagnostics diagnostics{};
  diagnostics.rx_total = 100;
  diagnostics.rx_standard = 70;
  diagnostics.rx_extended = 30;
  RuntimeMetrics runtime_metrics;
  runtime_metrics.update(data, 0);
  runtime_metrics.update(data, 1000);
  WebInterface web_interface;
  web_interface.update(
      data,
      session_status,
      diagnostics,
      runtime_metrics,
      1000);

  const std::string html = web_interface.renderDashboardHtml();
  const std::string diagnostics_html =
      web_interface.renderDiagnosticsHtml();
  const std::string json = web_interface.renderStatusJson();

  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("Engine speed"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("id='v0' class='reading'>3500</span>"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("id='v3' class='reading'>398</span>"));
  TEST_ASSERT_EQUAL(std::string::npos, html.find("398.10"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Fuel used"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Trip time"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("Engine data available"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("CAN OK"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("setInterval(poll,1000)"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("href='/diagnostics'"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("</html>"));
  TEST_ASSERT_TRUE(html.size() < 8192);

  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      diagnostics_html.find("Application TX"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      diagnostics_html.find("Missed / overrun / bus"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      diagnostics_html.find("Validity"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, diagnostics_html.find("href='/'"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, diagnostics_html.find("</html>"));
  TEST_ASSERT_TRUE(diagnostics_html.size() < 8192);

  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"status\":\"fresh\""));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"value\":\"3500\""));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find(
          "\"name\":\"Oil pressure\",\"value\":\"398.10\","
          "\"unit\":\"kPa\""));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"fuel_used_l\":0.010000"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"trip_time\":\"00:00:01\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, json.find("\"can_ok\":true"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"engine_data_available\":true"));
}

void test_runtime_metrics_freshness_and_wrap() {
  EngineData data{};
  data.fuel_flow_lph = 36.0f;
  data.rpm = 1200;
  data.fuel_flow_state.markFresh(0);
  data.rpm_state.markFresh(0);

  RuntimeMetrics metrics;
  metrics.update(data, 0);
  metrics.update(data, 1000);
  TEST_ASSERT_FLOAT_WITHIN(
      0.000001f,
      0.01f,
      static_cast<float>(metrics.fuelUsedLiters()));
  TEST_ASSERT_EQUAL_UINT64(1000, metrics.tripTimeMs());

  data.fuel_flow_state.markStale();
  data.rpm_state.markStale();
  metrics.update(data, 2000);
  TEST_ASSERT_FLOAT_WITHIN(
      0.000001f,
      0.01f,
      static_cast<float>(metrics.fuelUsedLiters()));
  TEST_ASSERT_EQUAL_UINT64(1000, metrics.tripTimeMs());

  data.fuel_flow_state.markFresh(2000);
  data.rpm = 0;
  data.rpm_state.markFresh(2000);
  metrics.update(data, 3000);
  TEST_ASSERT_FLOAT_WITHIN(
      0.000001f,
      0.02f,
      static_cast<float>(metrics.fuelUsedLiters()));
  TEST_ASSERT_EQUAL_UINT64(1000, metrics.tripTimeMs());

  EngineData wrap_data{};
  wrap_data.fuel_flow_lph = 3600.0f;
  wrap_data.rpm = 900;
  wrap_data.fuel_flow_state.markFresh(UINT32_MAX - 500);
  wrap_data.rpm_state.markFresh(UINT32_MAX - 500);
  RuntimeMetrics wrap_metrics;
  wrap_metrics.update(wrap_data, UINT32_MAX - 500);
  wrap_data.fuel_flow_state.markFresh(499);
  wrap_data.rpm_state.markFresh(499);
  wrap_metrics.update(wrap_data, 499);
  TEST_ASSERT_FLOAT_WITHIN(
      0.000001f,
      1.0f,
      static_cast<float>(wrap_metrics.fuelUsedLiters()));
  TEST_ASSERT_EQUAL_UINT64(1000, wrap_metrics.tripTimeMs());
}

void test_runtime_metrics_trip_over_24_hours() {
  EngineData data{};
  data.rpm = 1000;
  data.rpm_state.markFresh(0);
  RuntimeMetrics runtime_metrics;
  runtime_metrics.update(data, 0);
  data.rpm_state.markFresh(90061000);
  runtime_metrics.update(data, 90061000);

  SessionStatus session_status{};
  CanDiagnostics diagnostics{};
  WebInterface web_interface;
  web_interface.update(
      data,
      session_status,
      diagnostics,
      runtime_metrics,
      90061000);
  const std::string json = web_interface.renderStatusJson();
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      json.find("\"trip_time\":\"25:01:01\""));
}

void test_main_page_hides_stale_signal_value() {
  EngineData data{};
  data.oil_pressure_kpa = 398.10f;
  data.oil_pressure_state.markFresh(0);
  data.oil_pressure_state.markStale();
  RuntimeMetrics runtime_metrics;
  SessionStatus session_status{};
  CanDiagnostics diagnostics{};
  WebInterface web_interface;
  web_interface.update(
      data,
      session_status,
      diagnostics,
      runtime_metrics,
      10);

  const std::string html = web_interface.renderDashboardHtml();
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find(
          "id='v3' class='reading nodata'>No current data</span>"));
  TEST_ASSERT_EQUAL(std::string::npos, html.find(">398</span>"));
}

void setUp() {
}

void tearDown() {
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_decoders_scaling);
  RUN_TEST(test_numeric_oil_and_quality);
  RUN_TEST(test_fresh_stale_missing);
  RUN_TEST(test_malformed_and_rtr);
  RUN_TEST(test_transforms);
  RUN_TEST(test_safe_idle_incomplete_s0_zero_tx);
  RUN_TEST(test_unexpected_s0_blocks_tx);
  RUN_TEST(test_external_full_s3_zero_tx);
  RUN_TEST(test_exact_startup_sequence);
  RUN_TEST(test_full_s3_required_for_success);
  RUN_TEST(test_gate_timeout_250ms_terminal);
  RUN_TEST(test_tx_failure_terminal);
  RUN_TEST(test_session_loss_no_replay);
  RUN_TEST(test_new_ignition_lifecycle_allows_new_start);
  RUN_TEST(test_web_interface_simulated_data);
  RUN_TEST(test_runtime_metrics_freshness_and_wrap);
  RUN_TEST(test_runtime_metrics_trip_over_24_hours);
  RUN_TEST(test_main_page_hides_stale_signal_value);
  return UNITY_END();
}
