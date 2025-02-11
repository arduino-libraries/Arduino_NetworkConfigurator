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

extern SerialAgentClass SerialAgent;
