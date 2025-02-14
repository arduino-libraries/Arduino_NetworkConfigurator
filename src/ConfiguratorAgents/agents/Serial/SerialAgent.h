/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include "ConfiguratorAgents/agents/ConfiguratorAgent.h"
#include "ConfiguratorAgents/agents/BoardConfigurationProtocol/BoardConfigurationProtocol.h"
#include "ConfiguratorAgents/agents/BoardConfigurationProtocol/cbor/CBORInstances.h"
#include "Utility/LEDFeedback/LEDFeedback.h"

class SerialAgentClass : public ConfiguratorAgent, BoardConfigurationProtocol {
public:
  SerialAgentClass();
  AgentConfiguratorStates begin();
  AgentConfiguratorStates end();
  AgentConfiguratorStates poll();
  void disconnectPeer();
  bool receivedMsgAvailable();
  bool getReceivedMsg(ProvisioningInputMessage &msg);
  bool sendMsg(ProvisioningOutputMessage &msg);
  bool isPeerConnected();
  inline AgentTypes getAgentType() {
    return AgentTypes::USB_SERIAL;
  };
private:
  AgentConfiguratorStates _state = AgentConfiguratorStates::END;
  bool _disconnectRequest = false;
  /*SerialAgent private methods*/
  AgentConfiguratorStates handleInit();
  AgentConfiguratorStates handlePeerConnected();

  /*BoardConfigurationProtocol pure virtual methods implementation*/
  bool hasReceivedBytes();
  size_t receivedBytes();
  uint8_t readByte();
  int writeBytes(const uint8_t *data, size_t len);
  void handleDisconnectRequest();
  void clearInputBuffer();
};

inline SerialAgentClass::SerialAgentClass() {
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::begin() {
  if (_state == AgentConfiguratorStates::END){
    _state = AgentConfiguratorStates::INIT;
  }
  return _state;
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::end() {
  if (_state != AgentConfiguratorStates::END) {
    if (isPeerConnected()) {
      disconnectPeer();
    }
    clear();
    _state = AgentConfiguratorStates::END;
  }
  return _state;
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::poll() {

  switch (_state) {
    case AgentConfiguratorStates::INIT:           _state = handleInit         (); break;
    case AgentConfiguratorStates::RECEIVED_DATA:
    case AgentConfiguratorStates::PEER_CONNECTED: _state = handlePeerConnected(); break;
    case AgentConfiguratorStates::ERROR:                                          break;
    case AgentConfiguratorStates::END:                                            break;
  }

  if (_disconnectRequest) {
    _disconnectRequest = false;
    clear();
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::NONE);
    _state = AgentConfiguratorStates::INIT;
  }

  checkOutputPacketValidity();

  return _state;
}

inline void SerialAgentClass::disconnectPeer() {
  uint8_t data = 0x02;
  sendData(PacketManager::MessageType::TRANSMISSION_CONTROL, &data, sizeof(data));
  clear();
  LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::NONE);
  _state = AgentConfiguratorStates::INIT;
}

inline bool SerialAgentClass::getReceivedMsg(ProvisioningInputMessage &msg) {
  bool res = BoardConfigurationProtocol::getMsg(msg);
  if (receivedMsgAvailable() == false) {
    _state = AgentConfiguratorStates::PEER_CONNECTED;
  }
  return res;
}

inline bool SerialAgentClass::receivedMsgAvailable() {
  return BoardConfigurationProtocol::msgAvailable();
}

inline bool SerialAgentClass::sendMsg(ProvisioningOutputMessage &msg) {
  return BoardConfigurationProtocol::sendNewMsg(msg);
}

inline bool SerialAgentClass::isPeerConnected() {
  return Serial && (_state == AgentConfiguratorStates::PEER_CONNECTED || _state == AgentConfiguratorStates::RECEIVED_DATA);
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::handleInit() {
  AgentConfiguratorStates nextState = _state;
  if (Serial) {
    PacketManager::ReceivedData receivedData;
    while (Serial.available()) {
      uint8_t byte = Serial.read();
      PacketManager::ReceivingState res = PacketManager::getInstance().handleReceivedByte(receivedData, byte);
      if (res == PacketManager::ReceivingState::RECEIVED) {
        if (receivedData.type == PacketManager::MessageType::TRANSMISSION_CONTROL) {
          if (receivedData.payload.len() == 1 && receivedData.payload[0] == 0x01) {
            //CONNECT
            nextState = AgentConfiguratorStates::PEER_CONNECTED;
            PacketManager::getInstance().clear();
          }
        }
      } else if (res == PacketManager::ReceivingState::ERROR) {
        DEBUG_DEBUG("SerialAgentClass::%s Error receiving packet", __FUNCTION__);
        PacketManager::getInstance().clear();
        clearInputBuffer();
      }
    }
  }

  return nextState;
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::handlePeerConnected() {
  AgentConfiguratorStates nextState = _state;

  TransmissionResult res = sendAndReceive();

  switch (res) {
    case TransmissionResult::PEER_NOT_AVAILABLE:
      clear();
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

inline bool SerialAgentClass::hasReceivedBytes() {
  return Serial.available() > 0;
}

inline size_t SerialAgentClass::receivedBytes() {
  return Serial.available();
}

inline uint8_t SerialAgentClass::readByte() {
  return Serial.read();
}

inline int SerialAgentClass::writeBytes(const uint8_t *data, size_t len) {
  return Serial.write(data, len);
}

inline void SerialAgentClass::handleDisconnectRequest() {
  _disconnectRequest = true;
}

inline void SerialAgentClass::clearInputBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}
