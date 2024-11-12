/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <Arduino_DebugUtils.h>
#include <algorithm>
#include "utility/HCI.h"
#include "BLEStringCharacteristic.h"
#include "BLECharacteristic.h"
#include "BLEConfiguratorAgent.h"
#define DEBUG_PACKET
#define BASE_LOCAL_NAME "Arduino"

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define VID USB_VID
#define PID USB_PID
#elif defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_OPTA) || defined(ARDUINO_GIGA)
#include "pins_arduino.h"
#define VID BOARD_VENDORID
#define PID BOARD_PRODUCTID
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

#define MAX_VALIDITY_TIME 30000

void PrintPacket(const char *label, const uint8_t *data, size_t len) {
  if (Debug.getDebugLevel() == DBG_VERBOSE) {
    DEBUG_VERBOSE("BLEConfiguratorAgent Print %s data:", label);
    Debug.newlineOff();
    for (size_t i = 0; i < len; i++) {
      DEBUG_VERBOSE("%02x ", data[i]);
      if ((i + 1) % 10 == 0) {
        DEBUG_VERBOSE("\n");
      }
    }
    DEBUG_VERBOSE("\n");
  }
  Debug.newlineOn();
}

BLEConfiguratorAgent::BLEConfiguratorAgent()
  : _confService{ "5e5be887-c816-4d4f-b431-9eb34b02f4d9" },
    _inputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fc", BLEWrite, 256 },
    _outputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fa", BLEIndicate, 64 } {
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::begin() {
  if (_state != AgentConfiguratorStates::END) {
    return _state;
  }

  if (!BLE.begin()) {
    DEBUG_ERROR("BLEConfiguratorAgent::%s Starting  Bluetooth® Low Energy module failed!", __FUNCTION__);

    return AgentConfiguratorStates::ERROR;
  }

  if (!setLocalName()) {
    DEBUG_WARNING("BLEConfiguratorAgent::%s fail to set local name", __FUNCTION__);
  }
  // set manufacturer data
  if (!setManufacturerData()) {
    DEBUG_WARNING("BLEConfiguratorAgent::%s fail to set manufacturer data", __FUNCTION__);
  }

  BLE.setAdvertisedService(_confService);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // add the characteristic to the service
  _confService.addCharacteristic(_outputStreamCharacteristic);
  _confService.addCharacteristic(_inputStreamCharacteristic);

  // add service
  BLE.addService(_confService);

  // start advertising
  BLE.advertise();
  _state = AgentConfiguratorStates::INIT;
  DEBUG_DEBUG("BLEConfiguratorAgent begin completed");
  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::end() {
  if (_deviceConnected) {
    _deviceConnected = false;
    BLE.disconnect();
  }
  if (_state != AgentConfiguratorStates::END) {
    BLE.stopAdvertise();
    BLE.end();
    Packet.clear();
    _outputMessagesList.clear();
    _inputMessages.clear();
    _state = AgentConfiguratorStates::END;
  }

  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::poll() {
  BLE.poll();
  if (_bleEvent.newEvent) {
    _bleEvent.newEvent = false;
    switch (_bleEvent.type) {
      case BLEEventType::CONNECTED:
        _deviceConnected = true;
        _state = AgentConfiguratorStates::PEER_CONNECTED;
        break;
      case BLEEventType::DISCONNECTED:
        _deviceConnected = false;
        Packet.clear();
        _outputMessagesList.clear();
        _inputMessages.clear();
        _state = AgentConfiguratorStates::INIT;
        break;
      default:
        break;
    }
  }


  if (_deviceConnected) {
    if (_inputStreamCharacteristic.written()) {
      int receivedDataLen = _inputStreamCharacteristic.valueLength();
      const uint8_t *val = _inputStreamCharacteristic.value();
      PacketManager::ReceivingState res;
      PacketManager::ReceivedData receivedData;
      PrintPacket("received", val, receivedDataLen);
      for (int i = 0; i < receivedDataLen; i++) {
        res = Packet.handleReceivedByte(receivedData, val[i]);
        if (res == PacketManager::ReceivingState::ERROR) {
          DEBUG_DEBUG("BLEConfiguratorAgent::%s Error receiving packet", __FUNCTION__);
          sendNak();
          transmitStream();
          _inputStreamCharacteristic.writeValue("");
          break;
        } else if (res == PacketManager::ReceivingState::RECEIVED) {
          switch (receivedData.type) {
            case PacketManager::MessageType::DATA:
              {
                DEBUG_DEBUG("BLEConfiguratorAgent::%s Received data packet", __FUNCTION__);
                PrintPacket("payload", &receivedData.payload[0], receivedData.payload.len());
                _inputMessages.addMessage(receivedData.payload);
                //Consider all sent data as received
                _outputMessagesList.clear();
                _state = AgentConfiguratorStates::RECEIVED_DATA;
              }
              break;
            case PacketManager::MessageType::RESPONSE:
              {
                DEBUG_DEBUG("BLEConfiguratorAgent::%s Received response packet", __FUNCTION__);
                for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
                  packet->startProgress();
                }
              }
              break;
            default:
              break;
          }
        }
      }
    }

    if (_outputStreamCharacteristic.subscribed() && _outputMessagesList.size() > 0) {
      transmitStream();  // TODO remove
    }
  }

  if (_outputMessagesList.size() > 0) {
    checkOutputPacketValidity();
  }

  return _state;
}

bool BLEConfiguratorAgent::receivedDataAvailable() {
  return _inputMessages.numMessages() > 0;
}

bool BLEConfiguratorAgent::getReceivedData(uint8_t *data, size_t *len) {
  if (_inputMessages.numMessages() > 0) {
    InputPacketBuffer *msg = _inputMessages.frontMessage();
    if (msg->len() <= *len) {
      *len = msg->len();
      memset(data, 0x00, *len);
      memcpy(data, &(*msg)[0], *len);
      _inputMessages.popMessage();
      if (_inputMessages.numMessages() == 0) {
        _state = AgentConfiguratorStates::PEER_CONNECTED;
      }
      return true;
    }
  }
  return false;
}

size_t BLEConfiguratorAgent::getReceivedDataLength() {
  if (_inputMessages.numMessages() > 0) {
    InputPacketBuffer *msg = _inputMessages.frontMessage();
    return msg->len();
  }
  return 0;
}

bool BLEConfiguratorAgent::sendData(const uint8_t *data, size_t len) {
  return sendData(PacketManager::MessageType::DATA, data, len);
}

bool BLEConfiguratorAgent::isPeerConnected() {
  return _deviceConnected;
}

void BLEConfiguratorAgent::blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  _bleEvent.type = BLEEventType::CONNECTED;
  _bleEvent.newEvent = true;

  DEBUG_INFO("BLEConfiguratorAgent Connected event, central: %s", central.address().c_str());
}

void BLEConfiguratorAgent::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  _bleEvent.type = BLEEventType::DISCONNECTED;
  _bleEvent.newEvent = true;
  DEBUG_INFO("BLEConfiguratorAgent Disconnected event, central: %s", central.address().c_str());
}

//The local name is sent after a BLE scan Request
bool BLEConfiguratorAgent::setLocalName() {
  _Static_assert(sizeof(BASE_LOCAL_NAME) < 21, "Error BLE device Local Name too long. Reduce BASE_LOCAL_NAME length");  //Check at compile time if the local name length is valid
  char vid[5];
  char pid[5];
  sprintf(vid, "%04x", VID);
  sprintf(pid, "%04x", PID);

  _localName = BASE_LOCAL_NAME;
  _localName.concat("-");
  _localName.concat(vid);
  _localName.concat("-");
  _localName.concat(pid);

  return BLE.setLocalName(_localName.c_str());
}

//The manufacturer data is sent with the service uuid as advertised data
bool BLEConfiguratorAgent::setManufacturerData() {
  uint8_t addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  HCI.readBdAddr(addr);

  for (int i = 0; i < 6; i++) {
    _manufacturerData[i] = addr[5 - i];
  }

  return BLE.setManufacturerData(_manufacturerData, sizeof(_manufacturerData));
}

BLEConfiguratorAgent::TransmissionResult BLEConfiguratorAgent::transmitStream() {

  if (!_outputStreamCharacteristic.subscribed()) {
    return TransmissionResult::PEER_NOT_AVAILABLE;
  }
  if (_outputMessagesList.size() == 0) {
    return TransmissionResult::COMPLETED;
  }

  TransmissionResult res = TransmissionResult::COMPLETED;

  for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
    if (packet->hasBytesToSend()) {
      res = TransmissionResult::NOT_COMPLETED;
      packet->incrementBytesSent(_outputStreamCharacteristic.write(&(*packet)[packet->bytesSent()], packet->bytesToSend()));
      DEBUG_DEBUG("BLEConfiguratorAgent::%s outputCharacteristic transferred: %d of %d", __FUNCTION__, packet->bytesSent(), packet->len());
      delay(500);  //TODO test if remove
      break;
    }
  }

  return res;
}

bool BLEConfiguratorAgent::sendNak() {
  uint8_t data = 0x03;
  return sendData(PacketManager::MessageType::RESPONSE, &data, sizeof(data));
}

void BLEConfiguratorAgent::checkOutputPacketValidity() {
  _outputMessagesList.remove_if([](OutputPacketBuffer &packet) {
    if (packet.getValidityTs() != 0 && packet.getValidityTs() < millis()) {
      return true;
    }
    return false;
  });
}

bool BLEConfiguratorAgent::sendData(PacketManager::MessageType type, const uint8_t *data, size_t len) {
  OutputPacketBuffer outputMsg;
  outputMsg.setValidityTs(millis() + MAX_VALIDITY_TIME);

  if (!Packet.createPacket(outputMsg, type, data, len)) {
    DEBUG_WARNING("BLEConfiguratorAgent::%s Failed to create packet", __FUNCTION__);
    return false;
  }

  PrintPacket("output message", &outputMsg[0], outputMsg.len());

  _outputMessagesList.push_back(outputMsg);

  TransmissionResult res = TransmissionResult::NOT_COMPLETED;
  do {
    res = transmitStream();
    if (res == TransmissionResult::PEER_NOT_AVAILABLE) {
      break;
    }
  } while (res == TransmissionResult::NOT_COMPLETED);

  return true;
}

BLEConfiguratorAgent BLEAgent;
