/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "GenericConnectionHandler.h"
#include "ConfiguratorAgents/AgentsConfiguratorManager.h"
#ifdef ARDUINO_UNOR4_WIFI
#include <Preferences.h>  //TODO REPLACE with final lib
#endif
#include <settings/settings.h>
#define NC_CONNECTION_TIMEOUT 15000

enum class NetworkConfiguratorStates { INIT,
                                       CONNECTING,
                                       WAITING_FOR_CONFIG,
                                       CONFIGURED,
                                       END };

class NetworkConfigurator {
public:
  NetworkConfigurator(AgentsConfiguratorManager &agentManager, GenericConnectionHandler &connectionHandler, bool startConfigurationIfConnectionFails = true);
  bool begin();
  NetworkConfiguratorStates poll();
  void startConfigurationIfConnectionFails(bool enable){
    _startConfigurationIfConnectionFails = enable;
  };
  bool resetStoredConfiguration();
  bool end();

private:
  NetworkConfiguratorStates _state = NetworkConfiguratorStates::END;
  AgentsConfiguratorManager *_agentManager;
  GenericConnectionHandler *_connectionHandler;
  static inline models::NetworkSetting _networkSetting;
  bool _networkSettingReceived;
  bool _enableAutoReconnect;
  bool _startConfigurationIfConnectionFails;
  bool _connectionHandlerIstantiated = false;
  uint32_t _lastConnectionAttempt = 0;
  uint32_t _startConnectionAttempt;
  bool _connectionLostStatus = false;
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
  void printNetworkSettings();
#ifdef BOARD_HAS_WIFI
  bool scanWiFiNetworks(WiFiOption &wifiOptObj);
#endif
  static void scanReqHandler();
  static void connectReqHandler();
  static void setNetworkSettingsHandler(models::NetworkSetting *netSetting);
};