/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include <Arduino_DebugUtils.h>
#include <algorithm>
#include <ArduinoBLE.h>
#include "utility/HCI.h"
#include "BLEStringCharacteristic.h"
#include "BLECharacteristic.h"
#include "configuratorAgents/agents/ConfiguratorAgent.h"
#include "configuratorAgents/agents/boardConfigurationProtocol/BoardConfigurationProtocol.h"
#include "utility/LEDFeedback.h"
#include "configuratorAgents/agents/boardConfigurationProtocol/cbor/CBORInstances.h"

#define BASE_LOCAL_NAME "Arduino"
#define ARDUINO_COMPANY_ID 0x09A3

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define VID USB_VID
#define PID USB_PID
#elif defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA)
#include "pins_arduino.h"
#define VID BOARD_VENDORID
#define PID BOARD_PRODUCTID
#elif defined(ARDUINO_OPTA)
#include "pins_arduino.h"
#define VID _BOARD_VENDORID
#define PID _BOARD_PRODUCTID
#elif defined(ARDUINO_PORTENTA_C33)
#include "pins_arduino.h"
#define VID USB_VID
#define PID USB_PID
#elif defined(ARDUINO_UNOR4_WIFI)
#define VID 0x2341
#define PID 0x1002
#else
#error "Board not supported for BLE configuration"
#endif
/**
 * @class BLEAgentClass
 * @brief This class is responsible for managing BLE communication with a peer device/client for board configuration purposes.
 * It extends the ConfiguratorAgent class and implements the BoardConfigurationProtocol interface to handle
 * communication, message exchange, and connection management over a Bluetooth Low Energy interface.
 */
class BLEAgentClass : public ConfiguratorAgent, private BoardConfigurationProtocol {
public:
  BLEAgentClass();
  AgentConfiguratorStates begin();
  AgentConfiguratorStates end();
  AgentConfiguratorStates update();
  void disconnectPeer();
  bool receivedMsgAvailable();
  bool getReceivedMsg(ProvisioningInputMessage &msg);
  bool sendMsg(ProvisioningOutputMessage &msg);
  bool isPeerConnected();
  inline AgentTypes getAgentType() {
    return AgentTypes::BLE;
  };
private:
  enum class BLEEvent { NONE,
                        DISCONNECTED,
                        SUBSCRIBED };

  static inline BLEEvent _bleEvent = BLEEvent::NONE;
  AgentConfiguratorStates _state = AgentConfiguratorStates::END;
  BLEService _confService;  // BLE Configuration Service
  BLECharacteristic _inputStreamCharacteristic;
  BLECharacteristic _outputStreamCharacteristic;
  String _localName;
  uint8_t _manufacturerData[8];
  size_t _readByte = 0;

  /*BLEAgent private methods*/
  AgentConfiguratorStates handlePeerConnected();
  bool setLocalName();
  bool setManufacturerData();

  /*ArduinoBLE events callback functions*/
  static void blePeripheralDisconnectHandler(BLEDevice central);
  static void bleOutputStreamSubscribed(BLEDevice central, BLECharacteristic characteristic);

  /*BoardConfigurationProtocol pure virtual methods implementation*/
  bool received();
  size_t available();
  uint8_t read();
  int write(const uint8_t *data, size_t len);
  void handleDisconnectRequest();
  void clearInputBuffer();
};

inline BLEAgentClass::BLEAgentClass()
  : _confService{ "5e5be887-c816-4d4f-b431-9eb34b02f4d9" },
    _inputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fc", BLEWrite, 256 },
    _outputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fa", BLEIndicate, 64 } {
}

inline ConfiguratorAgent::AgentConfiguratorStates BLEAgentClass::begin() {
  if (_state != AgentConfiguratorStates::END) {
    return _state;
  }

  if (!BLE.begin()) {
    DEBUG_ERROR("BLEAgentClass::%s Starting  Bluetooth® Low Energy module failed!", __FUNCTION__);

    return AgentConfiguratorStates::ERROR;
  }

  if (!setLocalName()) {
    DEBUG_WARNING("BLEAgentClass::%s fail to set local name", __FUNCTION__);
  }
  // set manufacturer data
  if (!setManufacturerData()) {
    DEBUG_WARNING("BLEAgentClass::%s fail to set manufacturer data", __FUNCTION__);
  }

  BLE.setAdvertisedService(_confService);

  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  _outputStreamCharacteristic.setEventHandler(BLESubscribed, bleOutputStreamSubscribed);

  // add the characteristic to the service
  _confService.addCharacteristic(_outputStreamCharacteristic);
  _confService.addCharacteristic(_inputStreamCharacteristic);

  // add service
  BLE.addService(_confService);

  // start advertising
  BLE.advertise();
  LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::BLE_AVAILABLE);
  _state = AgentConfiguratorStates::INIT;

  return _state;
}

inline ConfiguratorAgent::AgentConfiguratorStates BLEAgentClass::end() {

  if (_state != AgentConfiguratorStates::END) {
    if (_state != AgentConfiguratorStates::INIT) {
      disconnectPeer();
    }
    BLE.stopAdvertise();
    BLE.end();
    clear();
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::NONE);
    _state = AgentConfiguratorStates::END;
  }

  return _state;
}

inline ConfiguratorAgent::AgentConfiguratorStates BLEAgentClass::update() {
  if (_state == AgentConfiguratorStates::END) {
    return _state;
  }
  BLE.poll();

  switch (_bleEvent) {
    case BLEEvent::SUBSCRIBED:
      if (_state != AgentConfiguratorStates::PEER_CONNECTED) {
        _state = AgentConfiguratorStates::PEER_CONNECTED;
        LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::PEER_CONNECTED);
      }
      break;
    case BLEEvent::DISCONNECTED:
      _inputStreamCharacteristic.writeValue("");
      clear();
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::BLE_AVAILABLE);
      _state = AgentConfiguratorStates::INIT;
      break;
    default:
      break;
  }
  _bleEvent = BLEEvent::NONE;

  switch (_state) {
    case AgentConfiguratorStates::INIT:                                           break;
    case AgentConfiguratorStates::PEER_CONNECTED: _state = handlePeerConnected(); break;
    case AgentConfiguratorStates::RECEIVED_DATA:                                  break;
    case AgentConfiguratorStates::ERROR:                                          break;
    case AgentConfiguratorStates::END:                                            break;
  }

  checkOutputPacketValidity();

  return _state;
}

inline bool BLEAgentClass::getReceivedMsg(ProvisioningInputMessage &msg) {
  bool res = BoardConfigurationProtocol::getMsg(msg);
  if (receivedMsgAvailable() == false) {
    _state = AgentConfiguratorStates::PEER_CONNECTED;
  }
  return res;
}

inline bool BLEAgentClass::receivedMsgAvailable() {
  return BoardConfigurationProtocol::msgAvailable();
}

inline bool BLEAgentClass::sendMsg(ProvisioningOutputMessage &msg) {
  return BoardConfigurationProtocol::sendMsg(msg);
}

inline bool BLEAgentClass::isPeerConnected() {
  return _outputStreamCharacteristic.subscribed() && (_state == AgentConfiguratorStates::PEER_CONNECTED || _state == AgentConfiguratorStates::RECEIVED_DATA);
}

inline void BLEAgentClass::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  _bleEvent = BLEEvent::DISCONNECTED;
  DEBUG_INFO("BLEAgentClass Disconnected event, central: %s", central.address().c_str());
}

inline void BLEAgentClass::bleOutputStreamSubscribed(BLEDevice central, BLECharacteristic characteristic) {
  _bleEvent = BLEEvent::SUBSCRIBED;
  DEBUG_INFO("BLEAgentClass Connected event, central: %s", central.address().c_str());
}

inline bool BLEAgentClass::received() {
  bool res = _inputStreamCharacteristic.written();
  if (res) {
    _readByte = 0;
  }
  return res;
}

inline size_t BLEAgentClass::available() {
  return _inputStreamCharacteristic.valueLength();
}

inline uint8_t BLEAgentClass::read() {
  const uint8_t *charValue = _inputStreamCharacteristic.value();
  if (_readByte < _inputStreamCharacteristic.valueLength()) {
    return charValue[_readByte++];
  }
  return 0;
}

inline int BLEAgentClass::write(const uint8_t *data, size_t len) {
  return _outputStreamCharacteristic.writeValue(data, len);
}

inline void BLEAgentClass::handleDisconnectRequest() {
}

inline ConfiguratorAgent::AgentConfiguratorStates BLEAgentClass::handlePeerConnected() {
  AgentConfiguratorStates nextState = _state;

  TransmissionResult res = sendAndReceive();
  switch (res) {
    case TransmissionResult::INVALID_DATA:
      // clear the input buffer
      _inputStreamCharacteristic.writeValue("");
      break;
    case TransmissionResult::PEER_NOT_AVAILABLE:
      disconnectPeer();
      nextState = AgentConfiguratorStates::INIT;
      break;
    case TransmissionResult::DATA_RECEIVED:
      nextState = AgentConfiguratorStates::RECEIVED_DATA;
      break;
    default:
      break;
  }

  return nextState;
}

//The local name is sent after a BLE scan Request
inline bool BLEAgentClass::setLocalName() {
  _Static_assert(sizeof(BASE_LOCAL_NAME) < 19, "Error BLE device Local Name too long. Reduce BASE_LOCAL_NAME length");  //Check at compile time if the local name length is valid
  char vid[5];
  char pid[5];
  sprintf(vid, "%04x", VID);
  sprintf(pid, "%04x", PID);

  _localName = BASE_LOCAL_NAME;
  _localName.concat("-");
  _localName.concat(vid);
  _localName.concat("-");
  _localName.concat(pid);
  BLE.setDeviceName(_localName.c_str());
  return BLE.setLocalName(_localName.c_str());
}

//The manufacturer data is sent with the service uuid as advertised data
inline bool BLEAgentClass::setManufacturerData() {
  uint8_t addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  HCI.readBdAddr(addr);
  uint16_t companyID = ARDUINO_COMPANY_ID;
  _manufacturerData[0] = (uint8_t)companyID & 0xFF;
  _manufacturerData[1] = (uint8_t)(companyID >> 8);
  for (int i = 2, j = 0; j < 6; j++, i++) {
    _manufacturerData[i] = addr[5 - j];
  }

  return BLE.setManufacturerData(_manufacturerData, sizeof(_manufacturerData));
}


inline void BLEAgentClass::disconnectPeer() {
  _inputStreamCharacteristic.writeValue("");
  uint32_t start = millis();
  BLE.disconnect();
  do {
    BLE.poll();
  } while (millis() - start < 200);
  clear();
  LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::BLE_AVAILABLE);
  _state = AgentConfiguratorStates::INIT;
  return;
}

inline void BLEAgentClass::clearInputBuffer() {
  // clear the input buffer
  _inputStreamCharacteristic.writeValue("");
}
