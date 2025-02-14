/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#if defined(ARDUINO_SAMD_MKRGSM1400) || defined(ARDUINO_SAMD_MKRNB1500) || defined(ARDUINO_SAMD_MKRWAN1300) || defined(ARDUINO_SAMD_MKRWAN1310)
#error "Board not supported"
#endif
#include "Arduino.h"
#include "GenericConnectionHandler.h"
#include "ConfiguratorAgents/AgentsManager.h"
#include <settings/settings.h>
#include <Arduino_TimedAttempt.h>
#include <Arduino_KVStore.h>

enum class NetworkConfiguratorStates { CHECK_ETH,
                                       READ_STORED_CONFIG,
                                       WAITING_FOR_CONFIG,
                                       CONNECTING,
                                       CONFIGURED,
                                       UPDATING_CONFIG,
                                       END };

class NetworkConfiguratorClass {
public:
  NetworkConfiguratorClass(ConnectionHandler &connectionHandler);
  bool begin();
  NetworkConfiguratorStates poll();
  bool resetStoredConfiguration();
  bool end();
  bool updateNetworkOptions();
  void setStorage(KVStore &kvstore) {
    _kvstore = &kvstore;
  }

  bool isBLEenabled();
  void enableBLE(bool enable);
  void configurationCompleted();
  bool addAgent(ConfiguratorAgent &agent);

private:
  NetworkConfiguratorStates _state = NetworkConfiguratorStates::END;
  ConnectionHandler *_connectionHandler;
  static inline models::NetworkSetting _networkSetting;
  bool _connectionHandlerIstantiated = false;
  TimedAttempt _connectionTimeout;
  TimedAttempt _connectionRetryTimer;
  TimedAttempt _optionUpdateTimer;

  enum class NetworkConfiguratorEvents { NONE,
                                         SCAN_REQ,
                                         CONNECT_REQ,
                                         NEW_NETWORK_SETTINGS,
                                         GET_WIFI_FW_VERSION };
  static inline NetworkConfiguratorEvents _receivedEvent = NetworkConfiguratorEvents::NONE;

  enum class ConnectionResult { SUCCESS,
                                FAILED,
                                IN_PROGRESS };

  KVStore *_kvstore = nullptr;
  AgentsManagerClass *_agentsManager;

#ifdef BOARD_HAS_ETHERNET
  NetworkConfiguratorStates handleCheckEth();
#endif
  NetworkConfiguratorStates handleReadStorage();
  NetworkConfiguratorStates handleWaitingForConf();
  NetworkConfiguratorStates handleConnecting();
  NetworkConfiguratorStates handleConfigured();
  NetworkConfiguratorStates handleUpdatingConfig();

  NetworkConfiguratorStates handleConnectRequest();
  void handleGetWiFiFWVersion();

  String decodeConnectionErrorMessage(NetworkConnectionState err, StatusMessage *errorCode);
  ConnectionResult connectToNetwork(StatusMessage *err);
  ConnectionResult disconnectFromNetwork();
  bool sendStatus(StatusMessage msg);
  static void printNetworkSettings();
#ifdef BOARD_HAS_WIFI
  bool scanWiFiNetworks(WiFiOption &wifiOptObj);
  bool insertWiFiAP(WiFiOption &wifiOptObj, char *ssid, int rssi);
#endif
  static void scanReqHandler();
  static void connectReqHandler();
  static void setNetworkSettingsHandler(models::NetworkSetting *netSetting);
  static void getWiFiFWVersionHandler();
};
