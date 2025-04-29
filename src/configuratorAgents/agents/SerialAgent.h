/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include "configuratorAgents/agents/ConfiguratorAgent.h"
#include "configuratorAgents/agents/boardConfigurationProtocol/BoardConfigurationProtocol.h"
#include "configuratorAgents/agents/boardConfigurationProtocol/cbor/CBORInstances.h"
#include "utility/LEDFeedback.h"

/**
 * @class SerialAgentClass
 * @brief This class is responsible for managing serial communication with a peer device/client for board configuration purposes.
 * It extends the ConfiguratorAgent class and implements the BoardConfigurationProtocol interface to handle
 * communication, message exchange, and connection management over a USB serial interface.
 */
class SerialAgentClass : public ConfiguratorAgent, private BoardConfigurationProtocol {
public:
  SerialAgentClass();
  AgentConfiguratorStates begin();
  AgentConfiguratorStates end();
  AgentConfiguratorStates update();
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
  PacketManager::Packet_t _packet;
  /*SerialAgent private methods*/
  AgentConfiguratorStates handleInit();
  AgentConfiguratorStates handlePeerConnected();

  /*BoardConfigurationProtocol pure virtual methods implementation*/
  bool received();
  size_t available();
  uint8_t read();
  int write(const uint8_t *data, size_t len);
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

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::update() {

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
  return BoardConfigurationProtocol::sendMsg(msg);
}

inline bool SerialAgentClass::isPeerConnected() {
  return Serial && (_state == AgentConfiguratorStates::PEER_CONNECTED || _state == AgentConfiguratorStates::RECEIVED_DATA);
}

inline ConfiguratorAgent::AgentConfiguratorStates SerialAgentClass::handleInit() {
  AgentConfiguratorStates nextState = _state;
  if (!Serial) {
    return nextState;
  }

  while (Serial.available()) {
    uint8_t byte = Serial.read();
    PacketManager::ReceivingState res = PacketManager::PacketReceiver::getInstance().handleReceivedByte(_packet, byte);
    if (res == PacketManager::ReceivingState::RECEIVED) {
      if (_packet.Type == PacketManager::MessageType::TRANSMISSION_CONTROL) {
        if (_packet.Payload.len() == 1 && _packet.Payload[0] == (uint8_t)PacketManager::TransmissionControlMessage::CONNECT) {
          //CONNECT
          nextState = AgentConfiguratorStates::PEER_CONNECTED;
        }
      }
      PacketManager::PacketReceiver::getInstance().clear(_packet);
    } else if (res == PacketManager::ReceivingState::ERROR) {
      DEBUG_DEBUG("SerialAgentClass::%s Error receiving packet", __FUNCTION__);
      clearInputBuffer();
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

inline bool SerialAgentClass::received() {
  return Serial.available() > 0;
}

inline size_t SerialAgentClass::available() {
  return Serial.available();
}

inline uint8_t SerialAgentClass::read() {
  return Serial.read();
}

inline int SerialAgentClass::write(const uint8_t *data, size_t len) {
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
