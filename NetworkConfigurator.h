#pragma once
#include "Arduino.h"
#include "Arduino_ConnectionHandler.h"
#include "AgentsConfiguratorManager.h"
#include <Preferences.h>
#include <settings/settings.h>

enum class NetworkConfiguratorStates {INIT, CONNECTING, WAITING_FOR_CONFIG, CONFIGURED, END};

class NetworkConfigurator{
  public:
    NetworkConfigurator(AgentsConfiguratorManager &agentManager, ConnectionHandler &connectionHandler);
    bool begin(bool initConfiguratorIfConnectionFails, String cause = "", bool resetStorage = false);
    NetworkConfiguratorStates poll();
    bool end();

  private:
    NetworkConfiguratorStates _state = NetworkConfiguratorStates::END;
    AgentsConfiguratorManager *_agentManager; 
    ConnectionHandler *_connectionHandler;
    models::NetworkSetting _networkSetting;
    bool _networkSettingReceived;
    bool _enableAutoReconnect;
    uint32_t _lastConnectionAttempt = 0;
    Preferences _preferences;

    NetworkConfiguratorStates handleConnecting();
    NetworkConfiguratorStates handleWaitingForConf();

    String decodeConnectionErrorMessage(NetworkConnectionState err);
    NetworkConnectionState connectToNetwork();
};