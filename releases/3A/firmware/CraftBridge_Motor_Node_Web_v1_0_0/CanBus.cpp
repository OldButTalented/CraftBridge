#include "CanBus.h"

#include "Config.h"

#include <Arduino.h>
#include <driver/twai.h>

#include <cstring>

namespace craftbridge {

bool CanBus::begin() {
  twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(
      static_cast<gpio_num_t>(config::kCanTxGpio),
      static_cast<gpio_num_t>(config::kCanRxGpio),
      TWAI_MODE_NORMAL);
  general_config.rx_queue_len = config::kCanRxQueueLength;
  const twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_250KBITS();
  const twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  ready_ = twai_driver_install(
               &general_config,
               &timing_config,
               &filter_config) == ESP_OK &&
           twai_start() == ESP_OK;
  return ready_;
}

bool CanBus::receive(CanFrame &frame) {
  if (!ready_) {
    return false;
  }

  twai_message_t message{};
  while (twai_receive(&message, 0) == ESP_OK) {
    ++diagnostics_.rx_total;

    if (message.rtr) {
      ++diagnostics_.rx_rtr;
      continue;
    }

    if (message.data_length_code > 8) {
      ++diagnostics_.rx_malformed;
      continue;
    }

    if (message.extd) {
      ++diagnostics_.rx_extended;
    } else {
      ++diagnostics_.rx_standard;
    }

    frame = {};
    frame.id = message.identifier;
    frame.extended = message.extd;
    frame.dlc = message.data_length_code;
    std::memcpy(frame.data.data(), message.data, frame.dlc);
    return true;
  }

  return false;
}

void CanBus::updateDiagnostics() {
  if (!ready_) {
    return;
  }

  twai_status_info_t status{};
  if (twai_get_status_info(&status) != ESP_OK) {
    return;
  }

  diagnostics_.rx_missed = status.rx_missed_count;
  diagnostics_.rx_overrun = status.rx_overrun_count;
  diagnostics_.bus_errors = status.bus_error_count;
}

bool CanBus::ready() const {
  return ready_;
}

const CanDiagnostics &CanBus::diagnostics() const {
  return diagnostics_;
}

bool CanBus::sendVerifiedStartupFrame(const CanFrame &frame) {
  if (!ready_) {
    ++diagnostics_.tx_failed;
    return false;
  }

  twai_message_t message{};
  message.identifier = frame.id;
  message.extd = 1;
  message.data_length_code = frame.dlc;
  std::memcpy(message.data, frame.data.data(), frame.dlc);

  if (twai_transmit(&message, pdMS_TO_TICKS(20)) != ESP_OK) {
    ++diagnostics_.tx_failed;
    return false;
  }

  ++diagnostics_.tx_successful;
  return true;
}

} // namespace craftbridge
