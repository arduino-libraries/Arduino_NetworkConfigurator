#pragma once
#include "Arduino.h"
#include "Arduino_ConnectionHandler.h"
#include "ConfiguratorAgents/AgentsConfiguratorManager.h"
#ifdef ARDUINO_UNOR4_WIFI
#include <Preferences.h> //TODO REPLACE with final lib
#endif
#include <settings/settings.h>
#define NC_CONNECTION_TIMEOUT 10000

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
    bool _initConfiguratorIfConnectionFails;
    uint32_t _lastConnectionAttempt = 0;
    uint32_t _startConnectionAttempt;
    String _initReason;
#ifdef ARDUINO_UNOR4_WIFI
    Preferences _preferences;
#endif

    NetworkConfiguratorStates handleInit();
    NetworkConfiguratorStates handleConnecting();
    NetworkConfiguratorStates handleWaitingForConf();

    String decodeConnectionErrorMessage(NetworkConnectionState err);
    NetworkConfiguratorStates connectToNetwork();
};