#pragma once
#include "settings/settings.h"
#include "ConfiguratorDefinitions.h"
#include "Arduino.h"

class ConfiguratorAgent {
  public:
    virtual ~ConfiguratorAgent() { }
    virtual ConfiguratorStates begin() = 0;
    virtual ConfiguratorStates end() = 0;
    virtual ConfiguratorStates poll() = 0;
    virtual bool getNetworkConfigurations(models::NetworkSetting *netSetting) = 0;
    virtual bool isPeerConnected() = 0;
    virtual bool setAvailableOptions(NetworkOptions netOptions) = 0;
    virtual bool setErrorCode(String error) = 0;
    virtual bool setInfoCode(String info) = 0;
    virtual AgentTypes getAgentType() = 0;
};