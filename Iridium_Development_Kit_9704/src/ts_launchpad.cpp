/* SPDX-License-Identifier: MIT */
#include "ts_launchpad.h"

#include <Arduino.h>
#include <string.h>
#include "9704LaunchPad.h"

#include "app_payload.h"
#include "ts_activation.h"
#include "ts_config.h"
#include "ts_mo_tracker.h"

namespace {

const int kCrcBytes = 2;
const uint32_t kStatusPeriodMs = 30000UL;
const int kSerialWaitMs = 3000;

ts_mo_tracker_t tracker;
uint8_t wire[TS_MAX_PAYLOAD + kCrcBytes]; // Payload bytes, then the CRC.
int wireLength = 0;
uint32_t sequence = 0;
int nextReference = MIN_MESSAGE_REQUEST_REFERENCE;
uint32_t lastStatusMs = 0;
bool readyAnnounced = false;
char command[40];
size_t commandLength = 0;

bool onSendData(int topic, int messageId, int length, int start,
                unsigned char output[], int outputCapacity);
void onSendStatus(int topic, int requestReference, int messageId,
                  SendMessageStatus status);
void onReceiveData(int topic, int messageId, int length, int start,
                   const unsigned char data[]);
void onReceiveStatus(int topic, int messageId, int maxLength,
                     ReceiveMessageStatus status);

void printHelp() {
  Serial.println();
  Serial.println("BEAR Module 9704 kit | Telemetry Station");
  Serial.println("  s        send ONE demo message (or press the USER button)");
  Serial.println("  i        how to find your modem IMEI and activate");
  Serial.println("  i <IMEI> show your activation steps for that IMEI");
  Serial.println("  d        show status");
  Serial.println("  u        release an UNCONFIRMED request (duplicate risk!)");
  Serial.println("  ?        show this help");
  Serial.println("Accepted by the modem is NOT the same as delivered.");
}

void printStatus() {
  bool visible = false;
  int bars = 0;
  int dbm = 0;
  transceiver.getLastConstellationState(visible, bars, dbm);
  Serial.println("--- STATUS ---");
  Serial.print("topic="); Serial.print(TS_TOPIC_ID);
  Serial.print(" schema="); Serial.println(app_payload_schema_id());
  Serial.print("maxPayloadBytes="); Serial.println(TS_MAX_PAYLOAD);
  Serial.print("battery="); Serial.print(batteryCharger.isBatteryConnected() ? "connected" : "NOT CONNECTED");
  Serial.print(" V="); Serial.println(analogInputs.batteryVoltageRead(), 2);
  Serial.print("modemReady="); Serial.println(transceiver.isReadyToSendMessage() ? "yes" : "no");
  Serial.print("signalBars="); Serial.print(bars);
  Serial.print(" dBm="); Serial.println(dbm);
  Serial.print("sendState="); Serial.println(ts_mo_phase_name(tracker.phase));
  Serial.println("--------------");
}

// The library does not expose the IMEI as a value, so we point to where the
// modem shows it, and build the activation steps from what the user types.
void showActivation(const char *imei) {
  if (*imei == '\0') {
    Serial.println("Find your modem's 15-digit IMEI in the startup log above, or on");
    Serial.println("the modem's label. Then type:  i <IMEI>");
    Serial.print("Payload code and bytes per message: "); Serial.println(TS_OPTIMIZER_URL);
    return;
  }
  if (!ts_imei_is_valid(imei)) {
    Serial.println("An IMEI is exactly 15 digits. Check it and try again.");
    return;
  }
  uint8_t sample[TS_MAX_PAYLOAD];
  size_t bytes = 0, bits = 0;
  const app_reading_t demo = app_demo_reading(0, 0, 0);
  if (!app_payload_encode(&demo, sample, sizeof sample, &bytes, &bits)) bytes = 0;
  char text[300];
  if (ts_activation_text(imei, static_cast<unsigned>(bytes), text, sizeof text))
    Serial.print(text);
  Serial.println("Set the topic you receive as TS_TOPIC_ID in src/ts_config.h.");
}

void printHex(const uint8_t *bytes, int length) {
  static const char digits[] = "0123456789abcdef";
  for (int i = 0; i < length; ++i) {
    Serial.write(digits[(bytes[i] >> 4) & 15]);
    Serial.write(digits[bytes[i] & 15]);
  }
  Serial.println();
}

void requestOneMessage() {
  if (TS_TOPIC_ID < 64) {
    Serial.println("SEND BLOCKED: set TS_TOPIC_ID (64 or higher) in src/ts_config.h.");
    Serial.println("Type 'i' to see how to get one from Telemetry Station.");
    return;
  }
  if (tracker.phase != TS_MO_IDLE || !transceiver.isReadyToSendMessage() ||
      !batteryCharger.isBatteryConnected()) {
    Serial.print("SEND BLOCKED: state="); Serial.println(ts_mo_phase_name(tracker.phase));
    Serial.println("The modem must be ready and the battery connected. No automatic retry.");
    return;
  }

  bool visible = false;
  int bars = 0;
  int dbm = 0;
  transceiver.getLastConstellationState(visible, bars, dbm);
  const int batteryMv = static_cast<int>(analogInputs.batteryVoltageRead() * 1000.0f);
  const app_reading_t reading = app_demo_reading(sequence + 1u, batteryMv, bars);

  size_t bytes = 0, bits = 0;
  if (!app_payload_encode(&reading, wire, TS_MAX_PAYLOAD, &bytes, &bits)) {
    Serial.println("ENCODE FAILED: a value does not fit the generated schema.");
    return;
  }

  const int crc = transceiver.crc(reinterpret_cast<const char *>(wire),
                                  static_cast<int>(bytes), 0);
  wire[bytes] = static_cast<uint8_t>((crc >> 8) & 0xff);
  wire[bytes + 1] = static_cast<uint8_t>(crc & 0xff);
  wireLength = static_cast<int>(bytes) + kCrcBytes;

  const int reference = nextReference;
  if (!ts_mo_begin(&tracker, TS_TOPIC_ID, reference, millis())) {
    Serial.println("SEND BLOCKED: request could not start.");
    return;
  }
  sequence = reading.sequence;
  nextReference = reference >= MAX_MESSAGE_REQUEST_REFERENCE
      ? MIN_MESSAGE_REQUEST_REFERENCE : reference + 1;

  Serial.print("REQUESTED topic="); Serial.print(TS_TOPIC_ID);
  Serial.print(" payloadBytes="); Serial.print(bytes);
  Serial.print(" bits="); Serial.print(bits);
  Serial.print(" hex="); printHex(wire, wireLength);
  Serial.println("DEMO data only: the coordinates and temperature are not measured.");
  transceiver.messageSend(TS_TOPIC_ID, wireLength, reference);
}

void handleCommand(const char *line) {
  switch (line[0]) {
    case 's': case 'S': requestOneMessage(); break;
    case 'i': case 'I': {
      const char *arg = line + 1;
      while (*arg == ' ') ++arg;
      showActivation(arg);
      break;
    }
    case 'd': case 'D': printStatus(); break;
    case 'u': case 'U':
      if (ts_mo_unlock(&tracker)) {
        Serial.println("Local lock released. The modem was NOT changed; a duplicate is possible.");
      } else {
        Serial.println("Nothing to release. Only an UNCONFIRMED request can be.");
      }
      break;
    case '?': printHelp(); break;
    default: break;
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      command[commandLength] = '\0';
      if (commandLength > 0) handleCommand(command);
      commandLength = 0;
    } else if (commandLength < sizeof(command) - 1) {
      command[commandLength++] = c;
    }
  }
}

ts_mo_status_t toTrackerStatus(SendMessageStatus code) {
  switch (code) {
    case SEND_MESSAGE_STATUS_ACCEPTED: return TS_MO_STATUS_ACCEPTED;
    case SEND_MESSAGE_STATUS_SUCCESS: return TS_MO_STATUS_DELIVERED;
    case SEND_MESSAGE_STATUS_INVALID: return TS_MO_STATUS_INVALID;
    default: return TS_MO_STATUS_FAILED;
  }
}

// The modem asks for a range of our message bytes.
bool onSendData(int topic, int messageId, int length, int start,
                unsigned char output[], int outputCapacity) {
  const bool ok = output != NULL && ts_mo_segment_ok(&tracker, topic, messageId,
      start, length, wireLength, outputCapacity);
  Serial.print("SEGMENT id="); Serial.print(messageId);
  Serial.print(" start="); Serial.print(start);
  Serial.print(" length="); Serial.println(length);
  if (!ok) return false;
  memcpy(output, wire + start, static_cast<size_t>(length));
  ts_mo_segment_sent(&tracker, messageId);
  return true;
}

void onSendStatus(int topic, int requestReference, int messageId,
                  SendMessageStatus code) {
  Serial.print("MO STATUS: ");
  Serial.println(transceiver.getStatusDescription(code));
  switch (ts_mo_on_status(&tracker, topic, requestReference, messageId,
                          toTrackerStatus(code))) {
    case TS_MO_OUTCOME_ACCEPTED:
      Serial.println("ACCEPTED: the modem queued the message (not delivered yet).");
      break;
    case TS_MO_OUTCOME_DELIVERED:
      Serial.println("DELIVERED: Iridium confirmed receipt.");
      break;
    case TS_MO_OUTCOME_FAILED:
      Serial.println("FAILED: see the status above and the modem log.");
      if (code == SEND_MESSAGE_STATUS_SUBSCRIPTION_INVALID)
        Serial.println("This modem does not have that topic. Check TS_TOPIC_ID.");
      break;
    case TS_MO_OUTCOME_IGNORED:
      Serial.println("(status is not about our message)");
      break;
  }
}

// This kit only sends. Messages sent to the modem are logged, not stored.
void onReceiveData(int, int, int, int, const unsigned char[]) {}

void onReceiveStatus(int topic, int messageId, int, ReceiveMessageStatus status) {
  Serial.print("MT (incoming) topic="); Serial.print(topic);
  Serial.print(" id="); Serial.print(messageId);
  Serial.print(": "); Serial.println(transceiver.getStatusDescription(status));
}

void checkRequestTimeout() {
  if (ts_mo_check_timeout(&tracker, millis(), TS_ACCEPT_TIMEOUT_MS)) {
    Serial.println("WARNING: the modem did not accept the request in time.");
    Serial.println("The outcome is UNKNOWN. Check the modem log and topic provisioning.");
    Serial.println("Type 'u' only after confirming nothing is queued.");
  }
}

} // namespace

void tsLaunchpadBegin() {
  Serial.begin(115200);
  waitForSerialPortReady(kSerialWaitMs);
  logSetup();

  analogInputs.logDisable();
  batteryCharger.logDisable();
  buzzer.logDisable();
  gnss.logDisable();
  userButton.logDisable();
  userLed.logDisable();
  transceiver.logEnable(); // The startup log shows the modem's IMEI and topics.

  analogInputs.begin();
  userButton.begin();
  userLed.begin();
  buzzer.begin();
  batteryCharger.begin();
  gnss.begin();
  userLed.showTransceiverSignalStrength();
  buzzer.playToneWhenTransmitting();

  ts_mo_init(&tracker);
  // Register callbacks before begin(), or early events could be missed.
  transceiver.setMessageCallbacks(onSendData, onSendStatus,
                                  onReceiveData, onReceiveStatus);
  printHelp();
  transceiver.begin();
  lastStatusMs = millis();
}

void tsLaunchpadLoop() {
  batteryCharger.loop();
  userLed.loop();
  userButton.loop();
  buzzer.loop();
  gnss.loop();
  transceiver.loop();

  if (!readyAnnounced && transceiver.isReadyToSendMessage()) {
    readyAnnounced = true;
    Serial.println("MODEM READY. To activate with Telemetry Station, type 'i'.");
  }

  readSerial();
  int pressedMs = 0;
  if (userButton.pushed(pressedMs)) requestOneMessage();

  checkRequestTimeout();
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastStatusMs) >= kStatusPeriodMs) {
    lastStatusMs = now;
    printStatus();
  }
}
