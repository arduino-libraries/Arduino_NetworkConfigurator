#pragma once
#include "Arduino.h"
#include "GenericConnectionHandler.h"
#include "ConfiguratorAgents/AgentsConfiguratorManager.h"
#ifdef ARDUINO_UNOR4_WIFI
#include <Preferences.h> //TODO REPLACE with final lib
#endif
#include <settings/settings.h>
#define NC_CONNECTION_TIMEOUT 15000

enum class NetworkConfiguratorStates {INIT, CONNECTING, WAITING_FOR_CONFIG, CONFIGURED, END};

class NetworkConfigurator{
  public:
    NetworkConfigurator(AgentsConfiguratorManager &agentManager, GenericConnectionHandler &connectionHandler);
    bool begin(bool initConfiguratorIfConnectionFails, String cause = "", bool resetStorage = false);
    NetworkConfiguratorStates poll();
    bool end();

  private:
    NetworkConfiguratorStates _state = NetworkConfiguratorStates::END;
    AgentsConfiguratorManager *_agentManager; 
    GenericConnectionHandler *_connectionHandler;
    static inline models::NetworkSetting _networkSetting;
    bool _networkSettingReceived;
    bool _enableAutoReconnect;
    bool _initConfiguratorIfConnectionFails;
    uint32_t _lastConnectionAttempt = 0;
    uint32_t _startConnectionAttempt;
    String _initReason;
    uint32_t _lastOptionUpdate = 0;
    bool _enableNetworkOptionsAutoUpdate = true;
    bool _connectionInProgress = false;
    static inline bool _scanReqReceived = false;
    static inline bool _connectReqReceived = false;
    static inline bool _networkSettingsToHandleReceived = false;
#ifdef ARDUINO_UNOR4_WIFI
    Preferences _preferences;
#endif

    NetworkConfiguratorStates handleInit();
    NetworkConfiguratorStates handleConnecting();
    NetworkConfiguratorStates handleWaitingForConf();

    String decodeConnectionErrorMessage(NetworkConnectionState err, int *errorCode);
    NetworkConfiguratorStates connectToNetwork();
    bool updateNetworkOptions();
#ifdef BOARD_HAS_WIFI
    bool scanWiFiNetworks(WiFiOption &wifiOptObj, String *err);
#endif
    static void scanReqHandler();
    static void connectReqHandler();
    static void setNetworkSettingsHandler(models::NetworkSetting *netSetting);
};