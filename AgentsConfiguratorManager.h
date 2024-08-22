#pragma once
#include <list>
#include "Arduino.h"
#include "ConfiguratorAgent.h"


enum class AgentsConfiguratorManagerStates {INIT, CONFIG_IN_PROGRESS, CONFIG_RECEIVED, END};

class AgentsConfiguratorManager {
  public:
    bool begin();
    bool end();
    AgentsConfiguratorManagerStates poll();
    bool getNetworkConfigurations(models::NetworkSetting *netSetting);
    bool setConnectionStatus(String res);
    bool addAgent(ConfiguratorAgent &agent);
    inline bool isConfigInProgress(){return _state == AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;};

  private:
    AgentsConfiguratorManagerStates _state = AgentsConfiguratorManagerStates::END;
    std::list<ConfiguratorAgent *> _agentsList;
    ConfiguratorAgent * _selectedAgent = nullptr;
    String _error;
    uint32_t _lastOptionUpdate = 0;
    AgentsConfiguratorManagerStates handleInit();
    AgentsConfiguratorManagerStates handleConfInProgress();
    
    bool updateAvailableOptions();
  
    bool scanWiFiNetworks(WiFiOption &wifiOptObj);
};
