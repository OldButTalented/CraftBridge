#pragma once

#include <cstdint>

namespace craftbridge::config {

inline constexpr char kFirmwareIdentity[] = "CraftBridge-Motor-Node/2.0.0";
inline constexpr char kAccessPointName[] = "CraftBridge-Motor";
inline constexpr char kAccessPointAddress[] = "http://192.168.4.1/";

inline constexpr bool kEspNowEnabled = true;
inline constexpr uint8_t kWifiChannel = 6;
inline constexpr uint8_t kEspNowPeerMac[6] = {
    0x28, 0x84, 0x85, 0x4D, 0x09, 0x28};
inline constexpr uint32_t kEspNowSendIntervalMs = 250;

inline constexpr int kCanTxGpio = 4;
inline constexpr int kCanRxGpio = 5;
inline constexpr uint32_t kCanBitrate = 250000;
// The verified S3 scheduler emits bursts of up to 11 frames.
// Sixteen slots preserve five additional frames of startup margin.
inline constexpr uint32_t kCanRxQueueLength = 16;
inline constexpr uint32_t kSerialMonitorBaud = 115200;
inline constexpr uint32_t kWebSnapshotIntervalMs = 250;
inline constexpr uint16_t kWebServerPort = 80;

} // namespace craftbridge::config
