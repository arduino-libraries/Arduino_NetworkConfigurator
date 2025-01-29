/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include <ArduinoBLE.h>
#include "ConfiguratorAgents/agents/ConfiguratorAgent.h"
#include "ConfiguratorAgents/agents/BoardConfigurationProtocol/BoardConfigurationProtocol.h"

class BLEAgentClass : public ConfiguratorAgent, BoardConfigurationProtocol {
public:
  BLEAgentClass();
  AgentConfiguratorStates begin();
  AgentConfiguratorStates end();
  AgentConfiguratorStates poll();
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
  bool hasReceivedBytes();
  size_t receivedBytes();
  uint8_t readByte();
  int writeBytes(const uint8_t *data, size_t len);
  void handleDisconnectRequest();
};

extern BLEAgentClass BLEAgent;
