#include <Arduino.h>
#include <driver/twai.h>

// CraftBridge HW Gate 2 real-ECU harness. TEST FIRMWARE ONLY.
// Behavioral authority: FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE v1.0.0.
// It transmits only the frozen 30-frame startup, then permanently disables
// application TX and observes the expanded producer state for 300 seconds.

namespace {

constexpr char kFirmwareId[] = "CraftBridge-Gate2-Real-ECU/1.1.0";
constexpr gpio_num_t kCanTxGpio = GPIO_NUM_4;
constexpr gpio_num_t kCanRxGpio = GPIO_NUM_5;
constexpr uint32_t kClientId = 0x00000B73;
constexpr uint32_t kEcuId = 0x0000730B;
constexpr uint32_t kArmObservationMs = 8000;  // Inherited from verified Python tool config.
constexpr uint32_t kGateTimeoutMs = 250;       // Inherited from verified Python tool config.
constexpr uint32_t kS3DetectionTimeoutMs = 5000; // Gate-2 harness criterion, not Mercury timing.
constexpr uint32_t kS3FreshnessMs = 5000;      // Gate-2 harness criterion, not Mercury timing.
constexpr uint32_t kPassiveDurationMs = 300000;
constexpr uint16_t kRequired170 = 0x807F; // pages 00..06 and FF(bit 15)
constexpr uint16_t kRequired1A0 = 0x9FFF; // pages 00..0C and FF(bit 15)

enum class Transform : uint8_t { None, Fa0401, Fa0206, Selector8004 };
enum class GateKind : uint8_t { None, Fixed, Challenge };

struct Step {
  uint32_t id;
  uint8_t dlc;
  uint8_t data[8];
  Transform transform;
  GateKind gate;
  uint8_t expectedDlc;
  uint8_t expected[8];
  const char *name;
};

#define B(...) {__VA_ARGS__}
// Frozen verified sequence: 30 application TX, 27 gates, three live transforms.
// The three GateKind::None entries are the same ungated immediate steps as the
// Python source. Do not add, reorder, retry or expose arbitrary TX.
const Step kSteps[30] = {
 {kClientId,1,B(0x55),Transform::None,GateKind::Fixed,1,B(0xAA),"55/AA"},
 {kClientId,2,B(0xC0,0x00),Transform::None,GateKind::Fixed,1,B(0x1B),"C0 00"},
 {kClientId,2,B(0xC0,0x01),Transform::None,GateKind::Fixed,1,B(0x03),"C0 01"},
 {kClientId,2,B(0xC0,0x06),Transform::None,GateKind::Fixed,1,B(0x0C),"C0 06"},
 {kClientId,2,B(0xC0,0x05),Transform::None,GateKind::Fixed,1,B(0x0A),"C0 05"},
 {kClientId,3,B(0xFA,0x04,0x01),Transform::None,GateKind::Challenge,4,B(),"FA 04 01"},
 {kClientId,5,B(),Transform::Fa0401,GateKind::Fixed,2,B(0x04,0x01),"F9 live R32"},
 {kClientId,5,B(0x06,0x00,0x0C,0x00,0x00),Transform::None,GateKind::Fixed,3,B(0x0C,0x00,0x00),"identity 0C"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x4D,0x59,0x32,0x30),"identity 1"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x30,0x36,0x70,0x30),"identity 2"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x41,0x41,0x41,0x49),"identity 3"},
 {kClientId,2,B(0x00,0x01),Transform::None,GateKind::Fixed,1,B(0x00),"identity terminator"},
 {kClientId,5,B(0x06,0x00,0x0D,0x00,0x00),Transform::None,GateKind::Fixed,3,B(0x0D,0x00,0x00),"profile 0D"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x4D,0x59,0x32,0x30),"profile 1"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x30,0x36,0x70,0x30),"profile 2"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x41,0x41,0x41,0x49),"profile 3"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x5F,0x30,0x39,0x5F),"profile 4"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x33,0x63,0x79,0x6C),"profile 5"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x34,0x30,0x5F,0x30),"profile 6"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x31,0x5F,0x30,0x30),"profile 7"},
 {kClientId,2,B(0x03,0x01),Transform::None,GateKind::Fixed,4,B(0x30,0x00,0x00,0x00),"profile 8"},
 {kClientId,1,B(0x55),Transform::None,GateKind::None,0,B(),"mid 55 ungated"},
 {kClientId,1,B(0x55),Transform::None,GateKind::Fixed,1,B(0xAA),"mid 55/AA 1"},
 {kClientId,1,B(0x55),Transform::None,GateKind::Fixed,1,B(0xAA),"mid 55/AA 2"},
 {0x1608B073,8,B(0x00,0xFF,0xFF,0xFF,0xFF,0x7F,0xFF,0xFF),Transform::None,GateKind::None,0,B(),"B073 ungated"},
 {0x1608B173,8,B(0x00,0xFF,0xFF,0x7F,0xFF,0xFF,0xFF,0xFF),Transform::None,GateKind::None,0,B(),"B173 ungated"},
 {kClientId,3,B(0xFA,0x02,0x06),Transform::None,GateKind::Challenge,4,B(),"FA 02 06"},
 {kClientId,5,B(),Transform::Fa0206,GateKind::Fixed,2,B(0x02,0x06),"F9 live R32"},
 {kClientId,2,B(0x80,0x04),Transform::None,GateKind::Challenge,4,B(),"80 04"},
 {kClientId,5,B(),Transform::Selector8004,GateKind::Fixed,1,B(0x04),"81 live R32"},
};
#undef B

uint8_t currentChallenge[4]{};
bool haveChallenge = false;
bool applicationTxEnabled = false;
bool attemptStarted = false;
bool twaiReady = false;
uint8_t successfulTx = 0;
uint8_t passedGates = 0;
uint16_t seen170 = 0;
uint16_t seen1A0 = 0;
uint32_t last170[16]{};
uint32_t last1A0[16]{};
uint32_t passiveRxFrames = 0;

void printPayload(const uint8_t *data, uint8_t dlc) {
  for (uint8_t i=0; i<dlc; ++i) { if (i) Serial.print(' '); if (data[i]<16) Serial.print('0'); Serial.print(data[i],HEX); }
}

uint32_t challengeValue() {
  return (uint32_t(currentChallenge[0])<<24)|(uint32_t(currentChallenge[1])<<16)|
         (uint32_t(currentChallenge[2])<<8)|currentChallenge[3];
}

uint32_t calculateResponse(Transform type, uint32_t e) {
  uint32_t multiplier=0, xorValue=0;
  if (type==Transform::Fa0401) { multiplier=0xD379A9C8; xorValue=0x1B4610CA; }
  else if (type==Transform::Fa0206) { multiplier=0xCF88B813; xorValue=0x4353E4D3; }
  else if (type==Transform::Selector8004) { multiplier=0xAB20FA1B; xorValue=0x208FB01A; }
  return uint32_t(uint64_t(e)*multiplier)^xorValue;
}

bool buildPayload(const Step &step, uint8_t out[8]) {
  if (step.transform==Transform::None) { memcpy(out,step.data,step.dlc); return true; }
  if (!haveChallenge) return false;
  const uint32_t e=challengeValue();
  const uint32_t r=calculateResponse(step.transform,e);
  out[0]=(step.transform==Transform::Selector8004)?0x81:0xF9;
  out[1]=uint8_t(r>>24); out[2]=uint8_t(r>>16); out[3]=uint8_t(r>>8); out[4]=uint8_t(r);
  Serial.print("[CHALLENGE] E="); printPayload(currentChallenge,4); Serial.println();
  Serial.print("[RESPONSE] R="); printPayload(out+1,4); Serial.println();
  haveChallenge=false;
  return true;
}

bool sendVerifiedStep(uint8_t position, const Step &step) {
  if (!applicationTxEnabled || position!=successfulTx || position>=30) return false;
  uint8_t payload[8]{};
  if (!buildPayload(step,payload)) return false;
  twai_message_t message{};
  message.identifier=step.id; message.extd=1; message.data_length_code=step.dlc;
  memcpy(message.data,payload,step.dlc);
  if (twai_transmit(&message,pdMS_TO_TICKS(20))!=ESP_OK) return false;
  ++successfulTx;
  Serial.printf("[TX %02u/30] 0x%08lX : ",successfulTx,(unsigned long)step.id);
  printPayload(payload,step.dlc); Serial.println();
  return true;
}

bool allowedAutonomousS0(const twai_message_t &m) {
  if (m.extd || m.rtr || m.data_length_code==0 || m.data_length_code>8) return false;
  const uint8_t p=m.data[0];
  return (m.identifier==0x170 && (p==0x00||p==0x03||p==0x06||p==0xFF)) ||
         (m.identifier==0x1A0 && (p==0x01||p==0xFF));
}

bool armBaseline() {
  constexpr uint16_t required170=0x8049; // pages 00,03,06,FF
  constexpr uint16_t required1A0=0x8002; // pages 01,FF
  uint16_t arm170=0,arm1A0=0; const uint32_t start=millis();
  Serial.println("[ARM] observing fresh ECU-only S0 for 8 s; SmartCraft application TX=0");
  while (uint32_t(millis()-start)<kArmObservationMs) {
    twai_message_t m{};
    if (twai_receive(&m,pdMS_TO_TICKS(20))!=ESP_OK) continue;
    if (m.rtr) continue;
    if (m.extd || m.data_length_code==0 || m.data_length_code>8) {
      Serial.println("[ARM] blocked by extended/malformed frame"); return false;
    }
    const uint8_t page=m.data[0]; const int bit=(page==0xFF)?15:int(page);
    if (m.identifier==0x170 && (page==0x00||page==0x03||page==0x06||page==0xFF)) arm170|=uint16_t(1u<<bit);
    else if (m.identifier==0x1A0 && (page==0x01||page==0xFF)) arm1A0|=uint16_t(1u<<bit);
    else { Serial.print("[ARM] blocked by unexpected S0 frame ID=0x"); Serial.println(m.identifier,HEX); return false; }
  }
  const bool ready=(arm170&required170)==required170 && (arm1A0&required1A0)==required1A0;
  if (ready) Serial.println("[ARM] Fresh S0 verified; TX=0");
  else Serial.printf("[ARM] missing baseline pages: 170=0x%04X 1A0=0x%04X\n",arm170,arm1A0);
  return ready;
}
bool waitForGate(const Step &step) {
  const uint32_t start=millis();
  while (uint32_t(millis()-start)<kGateTimeoutMs) {
    twai_message_t m{};
    if (twai_receive(&m,pdMS_TO_TICKS(10))!=ESP_OK) continue;
    if (m.rtr) continue;
    if (allowedAutonomousS0(m)) continue;
    if (!m.extd || m.identifier!=kEcuId || m.data_length_code>8) {
      Serial.print("[RX GATE] unexpected frame ID=0x"); Serial.println(m.identifier,HEX); return false;
    }
    if (step.gate==GateKind::Challenge) {
      if (m.data_length_code!=4) return false;
      memcpy(currentChallenge,m.data,4); haveChallenge=true;
    } else {
      if (m.data_length_code!=step.expectedDlc || memcmp(m.data,step.expected,step.expectedDlc)!=0) return false;
    }
    ++passedGates;
    Serial.printf("[RX GATE %02u/27] 0x%08lX : ",passedGates,(unsigned long)m.identifier);
    printPayload(m.data,m.data_length_code); Serial.println(" PASS");
    return true;
  }
  Serial.println("[RX GATE] response timeout");
  return false;
}

void noteProducer(const twai_message_t &m) {
  if (m.extd || m.rtr || m.data_length_code==0 || m.data_length_code>8) return;
  int bit=(m.data[0]==0xFF)?15:int(m.data[0]);
  if (bit<0 || bit>15) return;
  const uint16_t mask=uint16_t(1u<<bit); const uint32_t now=millis();
  if (m.identifier==0x170) { seen170|=mask; last170[bit]=now; }
  else if (m.identifier==0x1A0) { seen1A0|=mask; last1A0[bit]=now; }
}

bool fullS3Seen() { return (seen170&kRequired170)==kRequired170 && (seen1A0&kRequired1A0)==kRequired1A0; }

bool s3Fresh(uint32_t now) {
  for (uint8_t p=0;p<=6;++p) if (uint32_t(now-last170[p])>kS3FreshnessMs) return false;
  if (uint32_t(now-last170[15])>kS3FreshnessMs) return false;
  for (uint8_t p=0;p<=12;++p) if (uint32_t(now-last1A0[p])>kS3FreshnessMs) return false;
  return uint32_t(now-last1A0[15])<=kS3FreshnessMs;
}

void stopApplicationTx() { applicationTxEnabled=false; }

void fail(const char *reason, uint8_t expectedGate=0) {
  stopApplicationTx();
  Serial.println(); Serial.println("GATE 2 FAIL");
  Serial.print("Reason: "); Serial.println(reason);
  Serial.printf("Last completed TX: %u/30\n",successfulTx);
  Serial.printf("Expected gate: %u/27\n",expectedGate);
  Serial.println("SmartCraft application TX stopped");
}

bool detectS3() {
  const uint32_t start=millis();
  while (uint32_t(millis()-start)<kS3DetectionTimeoutMs) {
    twai_message_t m{};
    if (twai_receive(&m,pdMS_TO_TICKS(20))==ESP_OK) { noteProducer(m); ++passiveRxFrames; }
    if (fullS3Seen()) return true;
  }
  return false;
}

bool passive300Seconds() {
  const uint32_t start=millis(); uint32_t nextReport=60000;
  while (uint32_t(millis()-start)<kPassiveDurationMs) {
    twai_message_t m{};
    if (twai_receive(&m,pdMS_TO_TICKS(20))==ESP_OK) { noteProducer(m); ++passiveRxFrames; }
    const uint32_t elapsed=uint32_t(millis()-start);
    if (!s3Fresh(millis())) return false;
    if (elapsed>=nextReport) {
      Serial.printf("[PASSIVE] %lu / 300 s RX=%lu SmartCraft application TX=0\n",
                    (unsigned long)(nextReport/1000),(unsigned long)passiveRxFrames);
      nextReport+=60000;
    }
  }
  Serial.printf("[PASSIVE] 300 / 300 s RX=%lu SmartCraft application TX=0\n",(unsigned long)passiveRxFrames);
  return successfulTx==30 && passedGates==27;
}

void runGate2() {
  Serial.println("START accepted");
  Serial.println("Beginning 8 s fresh ECU-only S0 verification");
  Serial.println("DO NOT RESET OR DISCONNECT");

  if (!armBaseline()) { fail("fresh ECU-only S0 baseline not verified"); return; }
  applicationTxEnabled=true;

  for (uint8_t i=0;i<30;++i) {
    if (!sendVerifiedStep(i,kSteps[i])) { fail("TX or dynamic-response construction failure",passedGates+1); return; }
    if (kSteps[i].gate!=GateKind::None && !waitForGate(kSteps[i])) { fail("response gate failure",passedGates+1); return; }
  }
  if (successfulTx!=30 || passedGates!=27) { fail("startup totals mismatch",passedGates+1); return; }

  // From this point forward the harness cannot call application TX again.
  stopApplicationTx();
  Serial.println("STARTUP COMPLETE");
  if (!detectS3()) { fail("complete expanded S3 producer set not detected"); return; }
  Serial.println("S3 DETECTED");
  Serial.println("ENTERING 300 s PASSIVE PHASE");
  Serial.println("SmartCraft application TX = 0");
  if (!passive300Seconds()) { fail("expanded S3 producer freshness lost during passive phase"); return; }
  Serial.println(); Serial.println("GATE 2 PASS");
}

char commandBuffer[32]{};
uint8_t commandLength=0;
bool commandOverflow=false;

void processCommand() {
  if (!twaiReady || attemptStarted || commandOverflow) return;
  uint8_t first=0, last=commandLength;
  while (first<last && (commandBuffer[first]==' ' || commandBuffer[first]=='\t')) ++first;
  while (last>first && (commandBuffer[last-1]==' ' || commandBuffer[last-1]=='\t')) --last;
  if (last-first==5 && memcmp(commandBuffer+first,"START",5)==0) {
    attemptStarted=true;  // Lock this boot before entering any active state.
    runGate2();
  }
}

void pollStartCommand() {
  while (Serial.available()>0) {
    const char c=char(Serial.read());
    if (c=='\r' || c=='\n') {
      if (commandLength || commandOverflow) processCommand();
      commandLength=0; commandOverflow=false;
    } else if (commandLength<sizeof(commandBuffer)) {
      commandBuffer[commandLength++]=c;
    } else {
      commandOverflow=true;
    }
  }
}

} // namespace

void setup() {
  Serial.begin(115200); delay(1200);
  const twai_general_config_t general=TWAI_GENERAL_CONFIG_DEFAULT(kCanTxGpio,kCanRxGpio,TWAI_MODE_NORMAL);
  const twai_timing_config_t timing=TWAI_TIMING_CONFIG_250KBITS();
  const twai_filter_config_t filter=TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&general,&timing,&filter)!=ESP_OK || twai_start()!=ESP_OK) {
    Serial.println("GATE 2 FAIL\nReason: TWAI initialization failed\nSmartCraft application TX stopped"); return;
  }
  twaiReady=true;
  Serial.println("CraftBridge HW Gate 2 - Real ECU Test");
  Serial.println("ESP32-S3 N16R8"); Serial.println("CAN 250 kbit/s");
  Serial.println("TX GPIO4 / RX GPIO5"); Serial.print("Firmware/version identifier: "); Serial.println(kFirmwareId);
  Serial.println("SAFE IDLE");
  Serial.println("SmartCraft application TX = 0");
  Serial.println("Waiting for START command");
}

void loop() {
  pollStartCommand();
  if (twaiReady) {
    twai_message_t ignored{};
    while (twai_receive(&ignored,0)==ESP_OK) {}
  }
  delay(2);
}