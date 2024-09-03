#pragma once
#include <list>
#include "Arduino.h"
#include "agents/ConfiguratorAgent.h"


enum class AgentsConfiguratorManagerStates {INIT, CONFIG_IN_PROGRESS, CONFIG_RECEIVED, END};

enum class ConnectionStatusMessageType {INFO, CONNECTING, ERROR};
typedef struct{
  ConnectionStatusMessageType type;
  String msg;
}ConnectionStatusMessage;

class AgentsConfiguratorManager {
  public:
    bool begin();
    bool end();
    AgentsConfiguratorManagerStates poll();
    bool getNetworkConfigurations(models::NetworkSetting *netSetting);
    bool setConnectionStatus(ConnectionStatusMessage msg);
    bool addAgent(ConfiguratorAgent &agent);
    inline bool isConfigInProgress(){return _state == AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;};
    inline void disableConnOptionsAutoUpdate() {_enableOptionsAutoUpdate = false;};
    inline void enableConnOptionsAutoUpdate () {_enableOptionsAutoUpdate = true;};
  private:
    AgentsConfiguratorManagerStates _state = AgentsConfiguratorManagerStates::END;
    std::list<ConfiguratorAgent *> _agentsList;
    ConfiguratorAgent * _selectedAgent = nullptr;
    String _error;
    uint32_t _lastOptionUpdate = 0;
    bool _enableOptionsAutoUpdate = true;
    bool _connectionInProgress = false;

    AgentsConfiguratorManagerStates handleInit();
    AgentsConfiguratorManagerStates handleConfInProgress();

    bool updateAvailableOptions();
  
    bool scanWiFiNetworks(WiFiOption &wifiOptObj);
};
