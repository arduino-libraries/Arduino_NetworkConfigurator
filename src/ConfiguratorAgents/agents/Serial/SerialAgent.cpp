/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <Arduino_DebugUtils.h>
#include "SerialAgent.h"
#include "Utility/LEDFeedback/LEDFeedback.h"

SerialAgentClass::SerialAgentClass() {
}

ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::begin() {
  if (_state == AgentConfiguratorStates::END){
    _state = AgentConfiguratorStates::INIT;
  }
  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::end() {
  if (_state != AgentConfiguratorStates::END) {
    if (isPeerConnected()) {
      disconnectPeer();
    }
    clear();
    _state = AgentConfiguratorStates::END;
  }
  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::poll() {

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

void SerialAgentClass::disconnectPeer() {
  uint8_t data = 0x02;
  sendData(PacketManager::MessageType::TRANSMISSION_CONTROL, &data, sizeof(data));
  clear();
  LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::NONE);
  _state = AgentConfiguratorStates::INIT;
}

bool SerialAgentClass::getReceivedMsg(ProvisioningInputMessage &msg) {
  bool res = BoardConfigurationProtocol::getMsg(msg);
  if (receivedMsgAvailable() == false) {
    _state = AgentConfiguratorStates::PEER_CONNECTED;
  }
  return res;
}

bool SerialAgentClass::receivedMsgAvailable() {
  return BoardConfigurationProtocol::msgAvailable();
}

bool SerialAgentClass::sendMsg(ProvisioningOutputMessage &msg) {
  return BoardConfigurationProtocol::sendNewMsg(msg);
}

bool SerialAgentClass::isPeerConnected() {
  return Serial && (_state == AgentConfiguratorStates::PEER_CONNECTED || _state == AgentConfiguratorStates::RECEIVED_DATA);
}

ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::handleInit() {
  AgentConfiguratorStates nextState = _state;
  if (Serial) {
    PacketManager::ReceivedData receivedData;
    while (Serial.available()) {
      uint8_t byte = Serial.read();
      PacketManager::ReceivingState res = Packet.handleReceivedByte(receivedData, byte);
      if (res == PacketManager::ReceivingState::RECEIVED) {
        if (receivedData.type == PacketManager::MessageType::TRANSMISSION_CONTROL) {
          if (receivedData.payload.len() == 1 && receivedData.payload[0] == 0x01) {
            //CONNECT
            nextState = AgentConfiguratorStates::PEER_CONNECTED;
            Packet.clear();
          }
        }
      } else if (res == PacketManager::ReceivingState::ERROR) {
        DEBUG_DEBUG("SerialAgentClass::%s Error receiving packet", __FUNCTION__);
        Packet.clear();
        clearInputBuffer();
      }
    }
  }

  return nextState;
}

ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::handlePeerConnected() {
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

bool SerialAgentClass::hasReceivedBytes() {
  return Serial.available() > 0;
}

size_t SerialAgentClass::receivedBytes() {
  return Serial.available();
}

uint8_t SerialAgentClass::readByte() {
  return Serial.read();
}

int SerialAgentClass::writeBytes(const uint8_t *data, size_t len) {
  return Serial.write(data, len);
}

void SerialAgentClass::handleDisconnectRequest() {
  _disconnectRequest = true;
}

void SerialAgentClass::clearInputBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

SerialAgentClass SerialAgent;
