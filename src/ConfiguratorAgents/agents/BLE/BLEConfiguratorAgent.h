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
#include "ConfiguratorAgents/agents/BoardConfigurationProtocol/BoardConfigurationProtocol.h"

class BLEConfiguratorAgent : public BoardConfigurationProtocol {
public:
  BLEConfiguratorAgent();
  AgentConfiguratorStates begin();
  AgentConfiguratorStates end();
  AgentConfiguratorStates poll();
  void disconnectPeer();
  bool getReceivedMsg(ProvisioningInputMessage &msg) override;
  bool isPeerConnected();
  inline AgentTypes getAgentType() {
    return AgentTypes::BLE;
  };
private:
  enum class BLEEventType { NONE,
                            CONNECTED,
                            DISCONNECTED,
                            SUBSCRIBED };
  typedef struct {
    BLEEventType type;
    bool newEvent;
  } BLEEvent;
  static inline BLEEvent _bleEvent = { BLEEventType::NONE, false };
  AgentConfiguratorStates _state = AgentConfiguratorStates::END;
  BLEService _confService;  // Bluetooth® Low Energy LED Service
  BLECharacteristic _inputStreamCharacteristic;
  BLECharacteristic _outputStreamCharacteristic;
  String _localName;
  uint8_t _manufacturerData[8];
  size_t _readByte = 0;
  AgentConfiguratorStates handlePeerConnected();
  bool setLocalName();
  bool setManufacturerData();
  static void blePeripheralConnectHandler(BLEDevice central);
  static void blePeripheralDisconnectHandler(BLEDevice central);
  static void bleOutputStreamSubscribed(BLEDevice central, BLECharacteristic characteristic);
  virtual bool hasReceivedBytes();
  virtual size_t receivedBytes();
  virtual uint8_t readByte();
  virtual int writeBytes(const uint8_t *data, size_t len);
  virtual void handleDisconnectRequest();
};

extern BLEConfiguratorAgent BLEAgent;
