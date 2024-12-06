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

enum class NetworkConfiguratorStates { CHECK_ETH,
                                       READ_STORED_CONFIG,
                                       TEST_STORED_CONFIG,
                                       WAITING_FOR_CONFIG,
                                       CONNECTING,
                                       CONFIGURED,
                                       UPDATING_CONFIG,
                                       END };

class NetworkConfigurator {
public:
  NetworkConfigurator(AgentsConfiguratorManager &agentManager, GenericConnectionHandler &connectionHandler, bool startBLEIfConnectionFails = true);
  bool begin();
  NetworkConfiguratorStates poll();
  void startBLEIfConnectionFails(bool enable) {
    _startBLEIfConnectionFails = enable;
  };
  bool resetStoredConfiguration();
  bool end();

private:
  NetworkConfiguratorStates _state = NetworkConfiguratorStates::END;
  AgentsConfiguratorManager *_agentManager;
  GenericConnectionHandler *_connectionHandler;
  static inline models::NetworkSetting _networkSetting;
  bool _startBLEIfConnectionFails;
  bool _connectionHandlerIstantiated = false;
  uint32_t _lastConnectionAttempt = 0;
  uint32_t _startConnectionAttempt;
  bool _connectionLostStatus = false;
  uint32_t _lastOptionUpdate = 0;
  enum class NetworkConfiguratorEvents { NONE,
                                         SCAN_REQ,
                                         CONNECT_REQ,
                                         NEW_NETWORK_SETTINGS };
  static inline NetworkConfiguratorEvents _receivedEvent = NetworkConfiguratorEvents::NONE;

  enum class ConnectionResult { SUCCESS,
                                FAILED,
                                IN_PROGRESS };

#ifdef ARDUINO_UNOR4_WIFI
  Preferences _preferences;
#endif
#ifdef BOARD_HAS_ETHERNET
  NetworkConfiguratorStates handleCheckEth();
#endif
  NetworkConfiguratorStates handleReadStorage();
  NetworkConfiguratorStates handleTestStoredConfig();
  NetworkConfiguratorStates handleWaitingForConf();
  NetworkConfiguratorStates handleConnecting();
  NetworkConfiguratorStates handleConfigured();
  NetworkConfiguratorStates handleUpdatingConfig();

  NetworkConfiguratorStates handleConnectRequest();
  void handleNewNetworkSettings();

  String decodeConnectionErrorMessage(NetworkConnectionState err, int *errorCode);
  ConnectionResult connectToNetwork(MessageTypeCodes *err);
  bool updateNetworkOptions();
  void printNetworkSettings();
#ifdef BOARD_HAS_WIFI
  bool scanWiFiNetworks(WiFiOption &wifiOptObj);
  bool insertWiFiAP(WiFiOption &wifiOptObj, char *ssid, int rssi);
#endif
  static void scanReqHandler();
  static void connectReqHandler();
  static void setNetworkSettingsHandler(models::NetworkSetting *netSetting);
};