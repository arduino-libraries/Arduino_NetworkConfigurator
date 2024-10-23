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
    typedef void (*DataReceivedCallback)();
    enum class AgentConfiguratorStates {INIT, PEER_CONNECTED, RECEIVED_DATA, END, ERROR};
    enum class AgentTypes {BLE, AP, WIRED_USB};
    virtual ~ConfiguratorAgent() { }
    virtual AgentConfiguratorStates begin() = 0;
    virtual AgentConfiguratorStates end() = 0;
    virtual AgentConfiguratorStates poll() = 0;
    virtual bool receivedDataAvailable() = 0;
    virtual bool getReceivedData(uint8_t * data, size_t *len) = 0;
    virtual size_t getReceivedDataLength() = 0;
    virtual bool sendData(const uint8_t *data, size_t len) = 0;
    virtual bool isPeerConnected() = 0;
    virtual AgentTypes getAgentType() = 0;
};