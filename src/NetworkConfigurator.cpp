/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <Arduino_DebugUtils.h>
#include "ConnectionHandlerDefinitions.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
#include "Arduino_ConnectionHandler.h"
#include "Utility/LEDFeedback/LEDFeedback.h"
#ifdef BOARD_HAS_WIFI
#include "WiFiConnectionHandler.h"
#endif
#include "NetworkConfigurator.h"
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
#define BOARD_HAS_KVSTORE
#include <Arduino_KVStore.h>
#endif

#define SERVICE_ID_FOR_AGENTMANAGER 0xB0

#define NC_CONNECTION_RETRY_TIMER_ms 120000
#define NC_CONNECTION_TIMEOUT_ms 15000
#define NC_UPDATE_NETWORK_OPTIONS_TIMER_ms 120000

#if defined(BOARD_HAS_KVSTORE)
  KVStore _kvstore;
#endif
constexpr char *STORAGE_KEY{ "NETWORK_CONFIGS" };

NetworkConfiguratorClass::NetworkConfiguratorClass(ConnectionHandler &connectionHandler)
  :
    _connectionHandler{ &connectionHandler },
    _connectionTimeout{ NC_CONNECTION_TIMEOUT_ms, NC_CONNECTION_TIMEOUT_ms },
    _connectionRetryTimer{ NC_CONNECTION_RETRY_TIMER_ms, NC_CONNECTION_RETRY_TIMER_ms },
    _optionUpdateTimer{ NC_UPDATE_NETWORK_OPTIONS_TIMER_ms, NC_UPDATE_NETWORK_OPTIONS_TIMER_ms } {
      _optionUpdateTimer.begin(NC_UPDATE_NETWORK_OPTIONS_TIMER_ms); //initialize the timer before calling begin
}

bool NetworkConfiguratorClass::begin() {
  if(_state != NetworkConfiguratorStates::END) {
    return true;
  }
  _state = NetworkConfiguratorStates::READ_STORED_CONFIG;
  memset(&_networkSetting, 0x00, sizeof(models::NetworkSetting));
  LEDFeedbackClass::getInstance().begin();
#ifdef BOARD_HAS_WIFI
#ifndef ARDUINO_ARCH_ESP32
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    DEBUG_ERROR(F("The current WiFi firmware version is not the latest and it may cause compatibility issues. Please upgrade the WiFi firmware"));
  }
#endif
  if (!AgentsManager.addRequestHandler(RequestType::SCAN, scanReqHandler)) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s Error registering \"scan request\" callback to AgentManager", __FUNCTION__);
  }
#endif

  if (!AgentsManager.addRequestHandler(RequestType::CONNECT, connectReqHandler)) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s Error registering \"connect request\" callback to AgentManager", __FUNCTION__);
  }

  if (!AgentsManager.addReturnNetworkSettingsCallback(setNetworkSettingsHandler)) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s Error registering \"network settings\" callback to AgentManager", __FUNCTION__);
  }
  //TODO fix when rebasing
  if(!AgentsManager.addRequestHandler(RequestType::GET_WIFI_FW_VERSION, getWiFiFWVersionHandler)) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s Error registering \"get wifi firmware version request\" callback to AgentManager", __FUNCTION__);
  }

  if (!AgentsManager.begin(SERVICE_ID_FOR_AGENTMANAGER)) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s Failed to initialize the AgentsManagerClass", __FUNCTION__);
  }

  _connectionTimeout.begin(NC_CONNECTION_TIMEOUT_ms);
  _connectionRetryTimer.begin(NC_CONNECTION_RETRY_TIMER_ms);

#ifdef BOARD_HAS_ETHERNET
  _networkSetting.type = NetworkAdapter::ETHERNET;
  _networkSetting.eth.timeout = 250;
  _networkSetting.eth.response_timeout = 500;
  if (!_connectionHandler->updateSetting(_networkSetting)) {
    DEBUG_WARNING("NetworkConfiguratorClass::%s Is not possible check the eth connectivity", __FUNCTION__);
    return true;
  }
  _connectionHandlerIstantiated = true;
  _connectionTimeout.reload();
  _state = NetworkConfiguratorStates::CHECK_ETH;
#endif
  return true;
}

NetworkConfiguratorStates NetworkConfiguratorClass::poll() {
  NetworkConfiguratorStates nextState = _state;
  LEDFeedbackClass::getInstance().poll();

  switch (_state) {
#ifdef BOARD_HAS_ETHERNET
    case NetworkConfiguratorStates::CHECK_ETH:           nextState = handleCheckEth     (); break;
#endif
    case NetworkConfiguratorStates::READ_STORED_CONFIG: nextState = handleReadStorage   (); break;
    case NetworkConfiguratorStates::WAITING_FOR_CONFIG: nextState = handleWaitingForConf(); break;
    case NetworkConfiguratorStates::CONNECTING:         nextState = handleConnecting    (); break;
    case NetworkConfiguratorStates::CONFIGURED:         nextState = handleConfigured    (); break;
    case NetworkConfiguratorStates::UPDATING_CONFIG:    nextState = handleUpdatingConfig(); break;
    case NetworkConfiguratorStates::END:                                                    break;
  }

  if(_state != nextState){
    if(nextState == NetworkConfiguratorStates::CONNECTING){
      _connectionTimeout.reload();
    }
    _state = nextState;
  }

  return _state;
}

bool NetworkConfiguratorClass::resetStoredConfiguration() {
#if defined(BOARD_HAS_KVSTORE)
  bool res = false;
  if (_kvstore.begin()) {
    if(_kvstore.exists(STORAGE_KEY)) {
      res = _kvstore.remove(STORAGE_KEY);
    } else{
      res = true;
    }
    _kvstore.end();
  } else {
    DEBUG_DEBUG("Cannot initialize kvstore for deleting network settings");
  }
  if (!res) {
    return false;
  }
#endif

  memset(&_networkSetting, 0x00, sizeof(models::NetworkSetting));
  if(_connectionHandlerIstantiated) {
    disconnectFromNetwork();
    _connectionHandlerIstantiated = false;
  }

  if(_state != NetworkConfiguratorStates::END) {
    _state = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return true;
}

bool NetworkConfiguratorClass::end() {
  _lastConnectionAttempt = 0;
  _lastOptionUpdate = 0;
  AgentsManager.removeReturnNetworkSettingsCallback();
  AgentsManager.removeRequestHandler(RequestType::SCAN);
  AgentsManager.removeRequestHandler(RequestType::CONNECT);
  _state = NetworkConfiguratorStates::END;
  return AgentsManager.end(SERVICE_ID_FOR_AGENTMANAGER);
}


NetworkConfiguratorClass::ConnectionResult NetworkConfiguratorClass::connectToNetwork(StatusMessage *err) {
  ConnectionResult res = ConnectionResult::IN_PROGRESS;

  NetworkConnectionState connectionRes = NetworkConnectionState::DISCONNECTED;
  connectionRes = _connectionHandler->check();
  if (connectionRes == NetworkConnectionState::CONNECTED) {
    DEBUG_INFO("Connected to network");
    sendStatus(StatusMessage::CONNECTED);
    res = ConnectionResult::SUCCESS;
  } else if (connectionRes != NetworkConnectionState::CONNECTED && _connectionTimeout.isExpired())  //connection attempt failed
  {
#ifdef BOARD_HAS_WIFI
// Need for restoring the scan after a failed connection attempt
#if defined(ARDUINO_UNOR4_WIFI)
    WiFi.end();
#endif
#endif
    String errorMsg = decodeConnectionErrorMessage(connectionRes, err);
    DEBUG_INFO("Connection fail: %s", errorMsg.c_str());

    _connectionRetryTimer.reload();
    res = ConnectionResult::FAILED;
  }

  return res;
}

NetworkConfiguratorClass::ConnectionResult NetworkConfiguratorClass::disconnectFromNetwork() {
  _connectionHandler->disconnect();
  uint32_t startDisconnection = millis();
  NetworkConnectionState s;
  do {
    s = _connectionHandler->check();
  } while (s != NetworkConnectionState::CLOSED && millis() - startDisconnection < 5000);

  if (s != NetworkConnectionState::CLOSED) {
    return ConnectionResult::FAILED;
  }
  // Reset the connection handler to INIT state
  _connectionHandler->connect();

  return ConnectionResult::SUCCESS;
}

bool NetworkConfiguratorClass::updateNetworkOptions() {
#ifdef BOARD_HAS_WIFI
  DEBUG_DEBUG("Scanning");
  sendStatus(StatusMessage::SCANNING);  //Notify before scan

  WiFiOption wifiOptObj;

  if (!scanWiFiNetworks(wifiOptObj)) {
    DEBUG_WARNING("NetworkConfiguratorClass::%s Error during scan for wifi networks", __FUNCTION__);

    sendStatus(StatusMessage::HW_ERROR_CONN_MODULE);

    return false;
  }

  DEBUG_DEBUG("Scan completed");
  NetworkOptions netOption = { NetworkOptionsClass::WIFI, wifiOptObj };
#else
  NetworkOptions netOption = { NetworkOptionsClass::NONE };
#endif
  ProvisioningOutputMessage netOptionMsg = { MessageOutputType::NETWORK_OPTIONS };
  netOptionMsg.m.netOptions = &netOption;
  AgentsManager.sendMsg(netOptionMsg);

  _optionUpdateTimer.reload();
  return true;
}

#ifdef BOARD_HAS_WIFI
// insert a new WiFi network in the list of discovered networks
bool NetworkConfiguratorClass::insertWiFiAP(WiFiOption &wifiOptObj, char *ssid, int rssi) {
  if (wifiOptObj.numDiscoveredWiFiNetworks >= MAX_WIFI_NETWORKS) {
    return false;
  }

  // check if the network is already in the list and update the RSSI
  for (uint8_t i = 0; i < wifiOptObj.numDiscoveredWiFiNetworks; i++) {
    if (wifiOptObj.discoveredWifiNetworks[i].SSID == nullptr) {
      break;
    }
    if (strcmp(wifiOptObj.discoveredWifiNetworks[i].SSID, ssid) == 0) {
      if (wifiOptObj.discoveredWifiNetworks[i].RSSI < rssi) {
        wifiOptObj.discoveredWifiNetworks[i].RSSI = rssi;
      }
      return true;
    }
  }
  // add the network to the list
  wifiOptObj.discoveredWifiNetworks[wifiOptObj.numDiscoveredWiFiNetworks].SSID = ssid;
  wifiOptObj.discoveredWifiNetworks[wifiOptObj.numDiscoveredWiFiNetworks].SSIDsize = strlen(ssid);
  wifiOptObj.discoveredWifiNetworks[wifiOptObj.numDiscoveredWiFiNetworks].RSSI = rssi;
  wifiOptObj.numDiscoveredWiFiNetworks++;

  return true;
}

bool NetworkConfiguratorClass::scanWiFiNetworks(WiFiOption &wifiOptObj) {
  wifiOptObj.numDiscoveredWiFiNetworks = 0;
#if defined(ARDUINO_UNOR4_WIFI)
  WiFi.end();
#endif

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    DEBUG_WARNING("NetworkConfiguratorClass::%s Communication with WiFi module failed!", __FUNCTION__);
    return false;
  }

  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    DEBUG_WARNING("NetworkConfiguratorClass::%s Couldn't get any WiFi connection", __FUNCTION__);
    return false;
  }

  // print the list of networks seen:
  DEBUG_VERBOSE("NetworkConfiguratorClass::%s number of available networks: %d", __FUNCTION__, numSsid);

  // insert the networks in the list
  for (int thisNet = 0; thisNet < numSsid && thisNet < MAX_WIFI_NETWORKS; thisNet++) {
    DEBUG_VERBOSE("NetworkConfiguratorClass::%s found network %d) %s \tSignal %d dbm", __FUNCTION__, thisNet, WiFi.SSID(thisNet), WiFi.RSSI(thisNet));

    if (!insertWiFiAP(wifiOptObj, const_cast<char *>(WiFi.SSID(thisNet)), WiFi.RSSI(thisNet))) {
      DEBUG_WARNING("NetworkConfiguratorClass::%s The maximum number of WiFi networks has been reached", __FUNCTION__);
      break;
    }
  }

  return true;
}
#endif

void NetworkConfiguratorClass::scanReqHandler() {
  _receivedEvent = NetworkConfiguratorEvents::SCAN_REQ;
}

void NetworkConfiguratorClass::connectReqHandler() {
  _receivedEvent = NetworkConfiguratorEvents::CONNECT_REQ;
}

void NetworkConfiguratorClass::setNetworkSettingsHandler(models::NetworkSetting *netSetting) {
  memcpy(&_networkSetting, netSetting, sizeof(models::NetworkSetting));
  printNetworkSettings();
  _receivedEvent = NetworkConfiguratorEvents::NEW_NETWORK_SETTINGS;
}

void NetworkConfiguratorClass::getWiFiFWVersionHandler() {
  _receivedEvent = NetworkConfiguratorEvents::GET_WIFI_FW_VERSION;
}

NetworkConfiguratorStates NetworkConfiguratorClass::handleConnectRequest() {
  NetworkConfiguratorStates nextState = _state;
  if (_networkSetting.type == NetworkAdapter::NONE) {
    DEBUG_DEBUG("NetworkConfiguratorClass::%s Connect request received but network settings not received yet", __FUNCTION__);
    sendStatus(StatusMessage::PARAMS_NOT_FOUND);
    return nextState;
  }

  sendStatus(StatusMessage::CONNECTING);

#if defined(BOARD_HAS_KVSTORE)
  if (!_kvstore.begin()) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s error initializing kvstore", __FUNCTION__);
    sendStatus(StatusMessage::ERROR_STORAGE_BEGIN);
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
    return nextState;
  }
  if (!_kvstore.putBytes(STORAGE_KEY, (uint8_t *)&_networkSetting, sizeof(models::NetworkSetting))) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s error saving network settings", __FUNCTION__);
    sendStatus(StatusMessage::ERROR);
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
    return nextState;
  }

  _kvstore.end();
#endif

  if (_connectionHandlerIstantiated) {
    if(disconnectFromNetwork() == ConnectionResult::FAILED) {
      DEBUG_ERROR("NetworkConfiguratorClass::%s Impossible to disconnect the network", __FUNCTION__);
      sendStatus(StatusMessage::ERROR);
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
      return nextState;
    }
  }

  if (!_connectionHandler->updateSetting(_networkSetting)) {
    DEBUG_WARNING("NetworkConfiguratorClass::%s The received network parameters are not supported", __FUNCTION__);
    sendStatus(StatusMessage::INVALID_PARAMS);
    return nextState;
  }

  _connectionHandlerIstantiated = true;
  nextState = NetworkConfiguratorStates::CONNECTING;
  LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::CONNECTING_TO_NETWORK);
  return nextState;
}

String NetworkConfiguratorClass::decodeConnectionErrorMessage(NetworkConnectionState err, StatusMessage *errorCode) {
  switch (err) {
    case NetworkConnectionState::ERROR:
      *errorCode = StatusMessage::HW_ERROR_CONN_MODULE;
      return "HW error";
    case NetworkConnectionState::INIT:
      //the connection handler doesn't have a state of "Fail to connect", in case of invalid credentials or
      //missing network configuration the FSM stays on Init state so use this state to detect the fail to connect
      *errorCode = StatusMessage::FAILED_TO_CONNECT;
      return "Failed to connect";
    case NetworkConnectionState::CLOSED:
      *errorCode = StatusMessage::HW_CONN_MODULE_STOPPED;
      return "Peripheral stopped";
    case NetworkConnectionState::DISCONNECTED:
      *errorCode = StatusMessage::DISCONNECTED;
      return "Disconnected";
    case NetworkConnectionState::CONNECTING:
      //the connection handler doesn't have a state of "Internet not available", in case of it's impossible to reach internet
      //the FSM stays on Connecting state so use this state to detect if internet is not available
      *errorCode = StatusMessage::INTERNET_NOT_AVAILABLE;
      return "Internet not available";
    default:
      *errorCode = StatusMessage::ERROR;
      return "Generic error";
  }
}

void NetworkConfiguratorClass::handleGetWiFiFWVersion() {
  String fwVersion = WiFi.firmwareVersion();
  ProvisioningOutputMessage fwVersionMsg = { MessageOutputType::WIFI_FW_VERSION };
  fwVersionMsg.m.wifiFwVersion = fwVersion.c_str();
  _agentManager->sendMsg(fwVersionMsg);
}

#ifdef BOARD_HAS_ETHERNET
NetworkConfiguratorStates NetworkConfiguratorClass::handleCheckEth() {
  NetworkConfiguratorStates nextState = _state;
  StatusMessage err;
  ConnectionResult connectionRes = connectToNetwork(&err);
  if (connectionRes == ConnectionResult::SUCCESS) {
    nextState = NetworkConfiguratorStates::CONFIGURED;
  } else if (connectionRes == ConnectionResult::FAILED) {
    DEBUG_VERBOSE("NetworkConfiguratorClass::%s Connection eth fail", __FUNCTION__);
    if(disconnectFromNetwork() == ConnectionResult::FAILED) {
      sendStatus(StatusMessage::ERROR);
    }
    _connectionHandlerIstantiated = false;
    nextState = NetworkConfiguratorStates::READ_STORED_CONFIG;
  }
  return nextState;
}
#endif

NetworkConfiguratorStates NetworkConfiguratorClass::handleReadStorage() {
  NetworkConfiguratorStates nextState = _state;
#if defined(BOARD_HAS_KVSTORE)
  if (!_kvstore.begin()) {
    DEBUG_ERROR("NetworkConfiguratorClass::%s error initializing kvstore", __FUNCTION__);
    sendStatus(StatusMessage::ERROR_STORAGE_BEGIN);
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
    return nextState;
  }

  if (_kvstore.exists(STORAGE_KEY)) {
    _kvstore.getBytes(STORAGE_KEY, (uint8_t *)&_networkSetting, sizeof(models::NetworkSetting));
    printNetworkSettings();
    if (!_connectionHandler->updateSetting(_networkSetting)) {
      DEBUG_WARNING("NetworkConfiguratorClass::%s Network parameters found on storage are not supported.", __FUNCTION__);
      nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
    } else {
      _connectionHandlerIstantiated = true;
      nextState = NetworkConfiguratorStates::CONFIGURED;
    }

  } else {
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }
  _kvstore.end();
#else
  nextState = NetworkConfiguratorStates::CONFIGURED;
#endif

  if (nextState == NetworkConfiguratorStates::WAITING_FOR_CONFIG && _optionUpdateTimer.getWaitTime() == 0) {
    updateNetworkOptions();
  }
  return nextState;
}

NetworkConfiguratorStates NetworkConfiguratorClass::handleWaitingForConf() {
  NetworkConfiguratorStates nextState = _state;

  AgentsManager.poll();

  switch (_receivedEvent) {
    case NetworkConfiguratorEvents::SCAN_REQ:                updateNetworkOptions    (); break;
    case NetworkConfiguratorEvents::CONNECT_REQ: nextState = handleConnectRequest    (); break;
    case NetworkConfiguratorEvents::GET_WIFI_FW_VERSION:     handleGetWiFiFWVersion  (); break;
    case NetworkConfiguratorEvents::NEW_NETWORK_SETTINGS:                                break;
  }
  _receivedEvent = NetworkConfiguratorEvents::NONE;
  if (nextState == _state) {
    //Check if update the network options
    if (_optionUpdateTimer.isExpired()) {
      updateNetworkOptions();
    }

    if (_connectionHandlerIstantiated && AgentsManager.isConfigInProgress() != true && _connectionRetryTimer.isExpired()) {
      sendStatus(StatusMessage::CONNECTING);
      nextState = NetworkConfiguratorStates::CONNECTING;
    }
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfiguratorClass::handleConnecting() {
  NetworkConfiguratorStates nextState = _state;
  AgentsManager.poll();  //To keep alive the connection with the configurator
  StatusMessage err;
  ConnectionResult res = connectToNetwork(&err);

  if (res == ConnectionResult::SUCCESS) {
    nextState = NetworkConfiguratorStates::CONFIGURED;
  } else if (res == ConnectionResult::FAILED) {
    sendStatus(err);
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfiguratorClass::handleConfigured() {
  NetworkConfiguratorStates nextState = _state;
  //Return if the user is still connected to the agent, but has just finished configuring the network
  if (_agentManager->isConfigInProgress()) {
    return nextState;
  }

  _agentManager->poll();
  // If the agent manager changes state, it means that user is trying to configure the network, so the network configurator should change state
  if (_agentManager->isConfigInProgress()) {
    nextState = NetworkConfiguratorStates::UPDATING_CONFIG;

#ifdef BOARD_HAS_WIFI
    updateNetworkOptions();
#endif
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfiguratorClass::handleUpdatingConfig() {
  NetworkConfiguratorStates nextState = _state;
  if (AgentsManager.isConfigInProgress() == false) {
    //If peer disconnects without updating the network settings, go to connecting state for check the connection
    sendStatus(StatusMessage::CONNECTING);
    nextState = NetworkConfiguratorStates::CONNECTING;
  } else {
    nextState = handleWaitingForConf();
  }

  return nextState;
}

bool NetworkConfiguratorClass::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage statusMsg = { MessageOutputType::STATUS, { msg } };
  return AgentsManager.sendMsg(statusMsg);
}

#if defined(BOARD_HAS_ETHERNET)
void PrintIP(models::ip_addr *ip) {
  int len = 0;
  if (ip->type == IPType::IPv4) {
    len = 4;
  } else {
    len = 16;
  }
  Debug.newlineOff();
  for (int i = 0; i < len; i++) {
    DEBUG_INFO("%d ", ip->bytes[i]);
  }
  DEBUG_INFO("\n");
  Debug.newlineOn();
}
#endif

void NetworkConfiguratorClass::printNetworkSettings() {
  DEBUG_INFO("Network settings received:");
  switch (_networkSetting.type) {
#if defined(BOARD_HAS_WIFI)
    case NetworkAdapter::WIFI:
      DEBUG_INFO("WIFI");
      DEBUG_INFO("SSID: %s PSW: %s", _networkSetting.wifi.ssid, _networkSetting.wifi.pwd);
      break;
#endif

#if defined(BOARD_HAS_ETHERNET)
    case NetworkAdapter::ETHERNET:
      DEBUG_INFO("ETHERNET");
      DEBUG_INFO("IP: ");
      PrintIP(&_networkSetting.eth.ip);
      DEBUG_INFO(" DNS: ");
      PrintIP(&_networkSetting.eth.dns);
      DEBUG_INFO(" Gateway: ");
      PrintIP(&_networkSetting.eth.gateway);
      DEBUG_INFO(" Netmask: ");
      PrintIP(&_networkSetting.eth.netmask);
      DEBUG_INFO(" Timeout: %d , Response timeout: %d", _networkSetting.eth.timeout, _networkSetting.eth.response_timeout);
      break;
#endif

#if defined(BOARD_HAS_NB)
    case NetworkAdapter::NB:
      DEBUG_INFO("NB-IoT");
      DEBUG_INFO("PIN: %s", _networkSetting.nb.pin);
      DEBUG_INFO("APN: %s", _networkSetting.nb.apn);
      DEBUG_INFO("Login: %s", _networkSetting.nb.login);
      DEBUG_INFO("Pass: %s", _networkSetting.nb.pass);
      break;
#endif

#if defined(BOARD_HAS_GSM)
    case NetworkAdapter::GSM:
      DEBUG_INFO("GSM");
      DEBUG_INFO("PIN: %s", _networkSetting.gsm.pin);
      DEBUG_INFO("APN: %s", _networkSetting.gsm.apn);
      DEBUG_INFO("Login: %s", _networkSetting.gsm.login);
      DEBUG_INFO("Pass: %s", _networkSetting.gsm.pass);
      break;
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
    case NetworkAdapter::CATM1:
      DEBUG_INFO("CATM1");
      DEBUG_INFO("PIN: %s", _networkSetting.catm1.pin);
      DEBUG_INFO("APN: %s", _networkSetting.catm1.apn);
      DEBUG_INFO("Login: %s", _networkSetting.catm1.login);
      DEBUG_INFO("Pass: %s", _networkSetting.catm1.pass);
      DEBUG_INFO("Band: %d", _networkSetting.catm1.band);
      break;
#endif

#if defined(BOARD_HAS_CELLULAR)
    case NetworkAdapter::CELL:
      DEBUG_INFO("CELLULAR");
      DEBUG_INFO("PIN: %s", _networkSetting.cell.pin);
      DEBUG_INFO("APN: %s", _networkSetting.cell.apn);
      DEBUG_INFO("Login: %s", _networkSetting.cell.login);
      DEBUG_INFO("Pass: %s", _networkSetting.cell.pass);
      break;
#endif

#if defined(BOARD_HAS_LORA)
    case NetworkAdapter::LORA:
      DEBUG_INFO("LORA");
      DEBUG_INFO("AppEUI: %s", _networkSetting.lora.appeui);
      DEBUG_INFO("AppKey: %s", _networkSetting.lora.appkey);
      DEBUG_INFO("Band: %d", _networkSetting.lora.band);
      DEBUG_INFO("Channel mask: %s", _networkSetting.lora.channelMask);
      DEBUG_INFO("Device class: %c", _networkSetting.lora.deviceClass);
#endif
    default:
      break;
  }
}
