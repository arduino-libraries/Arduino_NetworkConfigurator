/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
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
#elif defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_PORTENTA_H7_M7) || \
defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_OPTA) || defined(ARDUINO_GIGA)
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

BLEConfiguratorAgent::BLEConfiguratorAgent()
  : _confService{ "5e5be887-c816-4d4f-b431-9eb34b02f4d9" },
    _inputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fc", BLEWrite, 256 },
    _outputStreamCharacteristic{ "0000ffe1-0000-1000-8000-00805f9b34fa", BLEIndicate, 64 } {
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::begin() {
  if (_state != AgentConfiguratorStates::END) {
    return _state;
  }
  //BLE.debug(Serial);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    return AgentConfiguratorStates::ERROR;
  }

  if (!setLocalName()) {
    Serial.println("fail to set local name");
  }
  // set manufacturer data
  if (!setManufacturerData()) {
    Serial.println("fail to set manufacturer data");
  }

  BLE.setAdvertisedService(_confService);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // add the characteristic to the service
  _confService.addCharacteristic(_outputStreamCharacteristic);
  _confService.addCharacteristic(_inputStreamCharacteristic);

  // add service
  BLE.addService(_confService);
  Serial.println("BLEConfiguratorAgent::begin starting adv");

  // start advertising
  BLE.advertise();
  _state = AgentConfiguratorStates::INIT;
  Serial.println("BLEConfiguratorAgent::begin started adv");
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
      Serial.println("inputStreamCharacteristic written");
      int receivedDataLen = _inputStreamCharacteristic.valueLength();
      const uint8_t *val = _inputStreamCharacteristic.value();
      PacketManager::ReceivingState res;
      PacketManager::ReceivedData receivedData;
#ifdef DEBUG_PACKET
      Serial.print("Received byte: ");
#endif
      for (int i = 0; i < receivedDataLen; i++) {
#ifdef DEBUG_PACKET
        Serial.print(val[i], HEX);
        Serial.print(" ");
#endif
        res = Packet.handleReceivedByte(receivedData, val[i]);
        if (res == PacketManager::ReceivingState::ERROR) {
          Serial.println("Error receiving packet");
          sendNak();
          transmitStream();
          _inputStreamCharacteristic.writeValue("");
          break;
        } else if (res == PacketManager::ReceivingState::RECEIVED) {
          switch (receivedData.type) {
            case PacketManager::MessageType::DATA:
              {
                Serial.println("Received data packet");
#ifdef DEBUG_PACKET
                Serial.println("************************************");
                for (uint16_t i = 0; i < receivedData.payload.len(); i++) {
                  Serial.print(receivedData.payload[i], HEX);
                  Serial.print(" ");
                  if ((i + 1) % 10 == 0) {
                    Serial.println();
                  }
                }
                Serial.println();
                Serial.println("************************************");
#endif
                _inputMessages.addMessage(receivedData.payload);
                //Consider all sent data as received
                _outputMessagesList.clear();
                _state = AgentConfiguratorStates::RECEIVED_DATA;
              }
              break;
            case PacketManager::MessageType::RESPONSE:
              {
                Serial.println("Received response packet");
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
#ifdef DEBUG_PACKET
      Serial.println();
#endif
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

  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void BLEConfiguratorAgent::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  _bleEvent.type = BLEEventType::DISCONNECTED;
  _bleEvent.newEvent = true;
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
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
    Serial.println("outputCharacteristic not subscribed");
    return TransmissionResult::PEER_NOT_AVAILABLE;
  }
  if (_outputMessagesList.size() == 0) {
    Serial.println("outputMessages empty");
    return TransmissionResult::COMPLETED;
  }

  TransmissionResult res = TransmissionResult::COMPLETED;

  for (std::list<OutputPacketBuffer>::iterator packet = _outputMessagesList.begin(); packet != _outputMessagesList.end(); ++packet) {
    if (packet->hasBytesToSend()) {
      res = TransmissionResult::NOT_COMPLETED;
      packet->incrementBytesSent(_outputStreamCharacteristic.write(&(*packet)[packet->bytesSent()], packet->bytesToSend()));
      Serial.print("outputCharacteristic transferred: ");
      Serial.print(packet->bytesSent());
      Serial.print(" of ");
      Serial.println(packet->len());
      if (!packet->hasBytesToSend()) {
        Serial.println("outputCharacteristictransfer completed");
      }
      delay(500);
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
      Serial.println("expired output message");
      return true;
    }
    return false;
  });
}

bool BLEConfiguratorAgent::sendData(PacketManager::MessageType type, const uint8_t *data, size_t len) {
  OutputPacketBuffer outputMsg;
  outputMsg.setValidityTs(millis() + MAX_VALIDITY_TIME);

  if (!Packet.createPacket(outputMsg, type, data, len)) {
    Serial.println("Failed to create packet");
    return false;
  }

#ifdef DEBUG_PACKET
  Serial.println("************************************");
  Serial.println("Output message");
  for (int i = 0; i < outputMsg.len(); i++) {
    Serial.print(outputMsg[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 10 == 0) {
      Serial.println();
    }
  }
  Serial.println();
  Serial.println("************************************");
#endif

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
