/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "settings/settings.h"
#include "ConfiguratorAgents/NetworkOptionsDefinitions.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
#include "Arduino.h"

class ConfiguratorAgent {
public:
  enum class AgentConfiguratorStates { INIT,
                                       PEER_CONNECTED,
                                       RECEIVED_DATA,
                                       END,
                                       ERROR };
  enum class AgentTypes { BLE,
                          USB_SERIAL,
  };
  virtual ~ConfiguratorAgent() {}
  virtual AgentConfiguratorStates begin() = 0;
  virtual AgentConfiguratorStates end() = 0;
  virtual AgentConfiguratorStates update() = 0;
  virtual void disconnectPeer() = 0;
  virtual bool receivedMsgAvailable() = 0;
  virtual bool getReceivedMsg(ProvisioningInputMessage &msg) = 0;
  virtual bool sendMsg(ProvisioningOutputMessage &msg) = 0;
  virtual bool isPeerConnected() = 0;
  virtual AgentTypes getAgentType() = 0;
};
