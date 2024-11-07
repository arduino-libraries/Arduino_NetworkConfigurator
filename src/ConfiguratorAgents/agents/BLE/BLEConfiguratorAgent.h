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
#include "ConfiguratorAgents/agents/PacketManager.h"
#include "BLEMessageCollection.h"

class BLEConfiguratorAgent: public ConfiguratorAgent  {
  public:
    BLEConfiguratorAgent();
    AgentConfiguratorStates begin();
    AgentConfiguratorStates end();
    AgentConfiguratorStates poll();
    bool receivedDataAvailable();
    bool getReceivedData(uint8_t * data, size_t *len);
    size_t getReceivedDataLength();
    bool sendData(const uint8_t *data, size_t len);
    bool isPeerConnected();
    inline AgentTypes getAgentType() { return AgentTypes::BLE;};
  private:
    enum class BLEEventType {NONE, CONNECTED, DISCONNECTED};
    enum class TransmissionResult {PEER_NOT_AVAILABLE = -1, NOT_COMPLETED = 0, COMPLETED = 1};
    typedef struct {
      BLEEventType type;
      bool newEvent;
    } BLEEvent;
    static inline BLEEvent _bleEvent = {BLEEventType::NONE, false};
    AgentConfiguratorStates  _state = AgentConfiguratorStates::END ;
    bool _deviceConnected = false;
    BLEService _confService; // Bluetooth® Low Energy LED Service
    BLECharacteristic _inputStreamCharacteristic;
    BLECharacteristic _outputStreamCharacteristic;
    std::list<OutputPacketBuffer> _outputMessagesList;
    BLEMessageCollection<InputPacketBuffer, 3> _inputMessages;
    String _localName;

    String generateLocalDeviceName();
    static void blePeripheralConnectHandler(BLEDevice central);
    static void blePeripheralDisconnectHandler(BLEDevice central);
    bool sendData(PacketManager::MessageType type, const uint8_t *data, size_t len);
    TransmissionResult transmitStream();
    bool sendNak();
    void checkOutputPacketValidity();


    
    
};

extern BLEConfiguratorAgent BLEAgent;
