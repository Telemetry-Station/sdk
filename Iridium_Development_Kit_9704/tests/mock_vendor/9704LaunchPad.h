/* SPDX-License-Identifier: MIT */
#ifndef TS_TEST_9704_LAUNCH_PAD_H
#define TS_TEST_9704_LAUNCH_PAD_H
#include "Arduino.h"
// API shape only. This stub makes no real satellite or modem operations.
static const int MIN_MESSAGE_REQUEST_REFERENCE = 1;
static const int MAX_MESSAGE_REQUEST_REFERENCE = 100;
enum SendMessageStatus {
 SEND_MESSAGE_STATUS_INVALID,
 SEND_MESSAGE_STATUS_ACCEPTED,
 SEND_MESSAGE_STATUS_SUBSCRIPTION_INVALID,
 SEND_MESSAGE_STATUS_SUCCESS,
 SEND_MESSAGE_STATUS_CANCELLED_PRE_TRANSIT,
 SEND_MESSAGE_STATUS_CANCELLED_IN_TRANSIT,
 SEND_MESSAGE_STATUS_DISCARDED_ON_OVERFLOW,
 SEND_MESSAGE_STATUS_EXPIRED,
 SEND_MESSAGE_STATUS_TRANSFER_TIMEOUT,
 SEND_MESSAGE_STATUS_SEGMENT_NOT_SUPPLIED,
 SEND_MESSAGE_STATUS_SEGMENT_INCORRECT,
 SEND_MESSAGE_STATUS_NETWORK_ERROR,
 SEND_MESSAGE_STATUS_PROTOCOL_ERROR,
 SEND_MESSAGE_STATUS_LOCAL_CRC_ERROR,
 SEND_MESSAGE_STATUS_CRC_ERROR_IN_TRANSFER,
 SEND_MESSAGE_STATUS_USER_SUPPLIED_CRC_ERROR,
};
enum ReceiveMessageStatus {
 RECEIVE_MESSAGE_STATUS_INVALID,
 RECEIVE_MESSAGE_START,
 RECEIVE_MESSAGE_COMPLETE,
 RECEIVE_MESSAGE_CANCELLED,
 RECEIVE_MESSAGE_FAILED_TIME_OUT,
 RECEIVE_MESSAGE_FAILED_CRC_ERROR,
};
using MoDataCallback = bool (*)(int,int,int,int,unsigned char[],int);
using MoStatusCallback = void (*)(int,int,int,SendMessageStatus);
using MtDataCallback = void (*)(int,int,int,int,const unsigned char[]);
using MtStatusCallback = void (*)(int,int,int,ReceiveMessageStatus);
struct MockPeripheral {
  void logDisable() {}
  void logEnable() {}
  void begin() {}
  void loop() {}
};
struct MockAnalog : MockPeripheral { float batteryVoltageRead() { return 3.8f; } };
struct MockBattery : MockPeripheral {
  bool isBatteryConnected() { return true; }
  const char *chargerStatusString() { return "OK"; }
};
struct MockBuzzer : MockPeripheral { void playToneWhenTransmitting() {} };
struct MockUserLed : MockPeripheral { void showTransceiverSignalStrength() {} };
struct MockUserButton : MockPeripheral { bool pushed(int &) { return false; } };
struct MockTransceiver : MockPeripheral {
  void getLastConstellationState(bool &,int &,int &) {}
  bool isBooted() { return true; }
  bool isReady() { return true; }
  bool isReadyToSendMessage() { return true; }
  void getSystemMetrics(int &) {}
  const char* getStatusDescription(SendMessageStatus) { return "status"; }
  const char* getStatusDescription(ReceiveMessageStatus) { return "status"; }
  int crc(const char[],int,int) { return 42; }
  void messageSend(int,int,int) {}
  void setMessageCallbacks(MoDataCallback,MoStatusCallback,MtDataCallback,MtStatusCallback) {}
};
extern MockAnalog analogInputs;
extern MockBattery batteryCharger;
extern MockBuzzer buzzer;
extern MockUserLed userLed;
extern MockUserButton userButton;
extern MockPeripheral gnss;
extern MockTransceiver transceiver;
inline void logSetup() {}
inline void waitForSerialPortReady(int) {}
inline bool TIL_isMessagingServicesProvisioned() { return true; }
inline int TIL_getNumberOfTopicsProvisioned() { return 1; }
#endif
