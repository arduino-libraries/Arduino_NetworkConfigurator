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
#ifdef BOARD_HAS_WIFI
#include "WiFiConnectionHandler.h"
#endif
#include "NetworkConfigurator.h"
#define SERVICE_ID_FOR_AGENTMANAGER 0xB0
constexpr char *SSIDKEY{ "SSID" };
constexpr char *PSWKEY{ "PASSWORD" };

NetworkConfigurator::NetworkConfigurator(AgentsConfiguratorManager &agentManager, GenericConnectionHandler &connectionHandler, bool startConfigurationIfConnectionFails)
  : _agentManager{ &agentManager },
    _connectionHandler{ &connectionHandler },
    _startConfigurationIfConnectionFails{ startConfigurationIfConnectionFails } {
}

bool NetworkConfigurator::begin() {
  _networkSettingReceived = false;
  _enableAutoReconnect = true;
  _connectionLostStatus = false;
  _state = NetworkConfiguratorStates::INIT;
  _startConnectionAttempt = 0;

#ifdef ARDUINO_UNOR4_WIFI
  while (1) {
    if (!_preferences.begin("my-app", false)) {
      DEBUG_VERBOSE("Cannot initialize preferences");
      _preferences.end();
    } else {
      DEBUG_DEBUG("preferences initialized");
      break;
    }
  }
#endif

#ifdef BOARD_HAS_WIFI
#ifndef ARDUINO_ARCH_ESP32
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    DEBUG_ERROR(F("The current WiFi firmware version is not the latest and it may cause compatibility issues. Please upgrade the WiFi firmware"));
  }
#endif
  _networkSetting.type = NetworkAdapter::WIFI;
  if (!_agentManager->addRequestHandler(RequestType::SCAN, scanReqHandler)) {
    DEBUG_ERROR("NetworkConfigurator::%s Error registering \"scan request\" callback to AgentManager", __FUNCTION__);
  }
#endif

  if (!_agentManager->addRequestHandler(RequestType::CONNECT, connectReqHandler)) {
    DEBUG_ERROR("NetworkConfigurator::%s Error registering \"connect request\" callback to AgentManager", __FUNCTION__);
  }

  if (!_agentManager->addReturnNetworkSettingsCallback(setNetworkSettingsHandler)) {
    DEBUG_ERROR("NetworkConfigurator::%s Error registering \"network settings\" callback to AgentManager", __FUNCTION__);
  }

  updateNetworkOptions();

  return true;
}

NetworkConfiguratorStates NetworkConfigurator::poll() {

  switch (_state) {
    case NetworkConfiguratorStates::INIT:               _state = handleInit          (); break;
    case NetworkConfiguratorStates::WAITING_FOR_CONFIG: _state = handleWaitingForConf(); break;
    case NetworkConfiguratorStates::CONNECTING:         _state = handleConnecting    (); break;
    case NetworkConfiguratorStates::CONFIGURED:                                          break;
    case NetworkConfiguratorStates::END:                                                 return _state;
  }

  //Handle if the scan request command is received
  if (_scanReqReceived) {
    _scanReqReceived = false;
    updateNetworkOptions();
  }

  //Check if update the network options
  if (_enableNetworkOptionsAutoUpdate && (millis() - _lastOptionUpdate > 120000)) {
    //if board doesn't support wifi and ble connectivity at the same time and the configuration is in progress skip updateAvailableOptions
#ifdef BOARD_HAS_WIFI
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
    if (_agentManager->isConfigInProgress() == true) {
      return _state;
    }
#endif
#endif
    updateNetworkOptions();
  }

  if (_networkSettingReceived && _agentManager->isConfigInProgress() != true && (millis() - _lastConnectionAttempt > 120000 && _enableAutoReconnect)) {
    _state = NetworkConfiguratorStates::CONNECTING;
  }

  return _state;
}

bool NetworkConfigurator::resetStoredConfiguration() {
#ifdef ARDUINO_UNOR4_WIFI
  bool initialized = false;
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    if (!_preferences.begin("my-app", false)) {
      DEBUG_VERBOSE("Cannot initialize preferences");
      _preferences.end();
    } else {
      DEBUG_DEBUG("preferences initialized");
      initialized = true;
      break;
    }
  }
  if(!initialized){
    return false;
  }
  
  bool res = _preferences.clear();

  _preferences.end();

  return res;
#endif
  return false;
}

bool NetworkConfigurator::end() {
  _lastConnectionAttempt = 0;
  _lastOptionUpdate = 0;
  _scanReqReceived = false;
  _agentManager->removeReturnNetworkSettingsCallback();
  _agentManager->removeRequestHandler(RequestType::SCAN);
  _agentManager->removeRequestHandler(RequestType::CONNECT);
  _state = NetworkConfiguratorStates::END;
#ifdef ARDUINO_UNOR4_WIFI
  _preferences.end();
#endif
  return _agentManager->end(SERVICE_ID_FOR_AGENTMANAGER);
}


NetworkConfiguratorStates NetworkConfigurator::connectToNetwork() {
  NetworkConfiguratorStates nextState = _state;
  DEBUG_DEBUG("Connecting to network");
  printNetworkSettings();
  if (_startConnectionAttempt == 0) {
    _startConnectionAttempt = millis();
  }
  NetworkConnectionState connectionRes = NetworkConnectionState::DISCONNECTED;

  connectionRes = _connectionHandler->check();
  delay(250);  //TODO remove
  if (connectionRes == NetworkConnectionState::CONNECTED) {
    _startConnectionAttempt = 0;
    DEBUG_INFO("Connected to network");
    _agentManager->setStatusMessage(MessageTypeCodes::CONNECTED);
    _enableAutoReconnect = false;
    delay(3000);  //TODO remove
    nextState = NetworkConfiguratorStates::CONFIGURED;
  } else if (connectionRes != NetworkConnectionState::CONNECTED && millis() - _startConnectionAttempt > NC_CONNECTION_TIMEOUT)  //connection attempt failed
  {
#ifdef BOARD_HAS_WIFI
    WiFi.end();
#endif
    _startConnectionAttempt = 0;
    int errorCode;
    String errorMsg = decodeConnectionErrorMessage(connectionRes, &errorCode);
    DEBUG_INFO("Connection fail: %s", errorMsg.c_str());
    if (_connectionLostStatus) {
      _agentManager->setStatusMessage(MessageTypeCodes::CONNECTION_LOST);
    } else {
      _agentManager->setStatusMessage((MessageTypeCodes)errorCode);
    }

    _lastConnectionAttempt = millis();
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
}

bool NetworkConfigurator::updateNetworkOptions() {
#ifdef BOARD_HAS_WIFI
  DEBUG_DEBUG("Scanning");
  _agentManager->setStatusMessage(MessageTypeCodes::SCANNING);  //Notify before scan

  WiFiOption wifiOptObj;

  if (!scanWiFiNetworks(wifiOptObj)) {
    DEBUG_WARNING("NetworkConfigurator::%s Error during scan for wifi networks", __FUNCTION__);

    _agentManager->setStatusMessage(MessageTypeCodes::HW_ERROR_CONN_MODULE);

    return false;
  }

  DEBUG_DEBUG("Scan completed");
  NetworkOptions netOption = { NetworkOptionsClass::WIFI, wifiOptObj };
#endif

  _agentManager->setNetworkOptions(netOption);

  _lastOptionUpdate = millis();

  return true;
}

#ifdef BOARD_HAS_WIFI
bool NetworkConfigurator::scanWiFiNetworks(WiFiOption &wifiOptObj) {
  wifiOptObj.numDiscoveredWiFiNetworks = 0;
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    DEBUG_WARNING("NetworkConfigurator::%s Communication with WiFi module failed!", __FUNCTION__);
    return false;
  }

  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    DEBUG_WARNING("NetworkConfigurator::%s Couldn't get any WiFi connection", __FUNCTION__);
    return false;
  }

  // print the list of networks seen:
  DEBUG_VERBOSE("NetworkConfigurator::%s number of available networks: %d", __FUNCTION__, numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid && thisNet < MAX_WIFI_NETWORKS; thisNet++) {
    DEBUG_VERBOSE("NetworkConfigurator::%s found network %d) %s \tSignal %d dbm", __FUNCTION__, thisNet, WiFi.SSID(thisNet), WiFi.RSSI(thisNet));

    wifiOptObj.discoveredWifiNetworks[thisNet].SSID = const_cast<char *>(WiFi.SSID(thisNet));


    wifiOptObj.discoveredWifiNetworks[thisNet].SSIDsize = strlen(WiFi.SSID(thisNet));
    wifiOptObj.discoveredWifiNetworks[thisNet].RSSI = WiFi.RSSI(thisNet);

    wifiOptObj.numDiscoveredWiFiNetworks++;
  }

  WiFi.end();
  return true;
}
#endif

void NetworkConfigurator::scanReqHandler() {
  _scanReqReceived = true;
}

void NetworkConfigurator::connectReqHandler() {
  _connectReqReceived = true;
}

void NetworkConfigurator::setNetworkSettingsHandler(models::NetworkSetting *netSetting) {
  memcpy(&_networkSetting, netSetting, sizeof(models::NetworkSetting));
  _networkSettingsToHandleReceived = true;
}

String NetworkConfigurator::decodeConnectionErrorMessage(NetworkConnectionState err, int *errorCode) {
  switch (err) {
    case NetworkConnectionState::ERROR:
      *errorCode = (int)MessageTypeCodes::HW_ERROR_CONN_MODULE;
      return "HW error";
    case NetworkConnectionState::INIT:
      *errorCode = (int)MessageTypeCodes::WIFI_IDLE;
      return "Peripheral in idle";
    case NetworkConnectionState::CLOSED:
      *errorCode = (int)MessageTypeCodes::WIFI_STOPPED;
      return "Peripheral stopped";
    case NetworkConnectionState::DISCONNECTED:
      *errorCode = (int)MessageTypeCodes::DISCONNECTED;
      return "Disconnected";
      //the connection handler doesn't have a state of "Fail to connect", in case of invalid credentials or
      //missing wifi network the FSM stays on Connecting state so use the connecting state to detect the fail to connect
    case NetworkConnectionState::CONNECTING:
      *errorCode = (int)MessageTypeCodes::FAILED_TO_CONNECT;
      return "failed to connect";
    default:
      *errorCode = (int)MessageTypeCodes::ERROR;
      return "generic error";
  }
}

NetworkConfiguratorStates NetworkConfigurator::handleInit() {
  NetworkConfiguratorStates nextState = _state;
  if (!_networkSettingReceived) {
#ifdef BOARD_HAS_WIFI
#ifdef ARDUINO_UNOR4_WIFI
    String SSID = _preferences.getString(SSIDKEY, "");
    String Password = _preferences.getString(PSWKEY, "");
    //preferences.end();
    if (SSID != "" && Password != "") {
      DEBUG_INFO("Found WiFi configuration on storage.\n SSID: %s PSW: %s", SSID.c_str(), Password.c_str());

      memcpy(_networkSetting.wifi.ssid, SSID.c_str(), SSID.length());
      memcpy(_networkSetting.wifi.pwd, Password.c_str(), Password.length());
      _networkSettingReceived = true;
    }
#endif
#endif
  }
  if (_networkSettingReceived) {
    if (!_connectionHandlerIstantiated) {
      if (!_connectionHandler->updateSetting(_networkSetting)) {
        DEBUG_WARNING("NetworkConfigurator::%s Network parameters found on storage are not supported.", __FUNCTION__);
        _agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER);
        return NetworkConfiguratorStates::WAITING_FOR_CONFIG;
      }
      _connectionHandlerIstantiated = true;
    }

    // Test connection
    _enableAutoReconnect = false;
    nextState = connectToNetwork();

    if (nextState == _state) {
      return _state;
    }

    if (nextState != NetworkConfiguratorStates::CONFIGURED && _startConfigurationIfConnectionFails) {
      _enableAutoReconnect = true;

      if (!_agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER)) {
        DEBUG_ERROR("NetworkConfigurator::%s Failed to initialize the AgentsConfiguratorManager", __FUNCTION__);
      }
      _connectionLostStatus = true;
      _agentManager->setStatusMessage(MessageTypeCodes::CONNECTION_LOST);

    } else {
      nextState = NetworkConfiguratorStates::CONFIGURED;
    }
  } else {

    _agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER);
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleWaitingForConf() {
  NetworkConfiguratorStates nextState = _state;
  _agentManager->poll();
  if (_networkSettingsToHandleReceived) {
    _networkSettingsToHandleReceived = false;

    _connectionLostStatus = false;  //reset for updating the failure reason


    if (!_connectionHandler->updateSetting(_networkSetting)) {
      DEBUG_WARNING("NetworkConfigurator::%s The received network parameters are not supported", __FUNCTION__);
      _agentManager->setStatusMessage(MessageTypeCodes::INVALID_PARAMS);
      return NetworkConfiguratorStates::WAITING_FOR_CONFIG;
    }

    _connectionHandlerIstantiated = true;

    printNetworkSettings();

#ifdef BOARD_HAS_WIFI
#ifdef ARDUINO_UNOR4_WIFI
    _preferences.remove(SSIDKEY);
    _preferences.putString(SSIDKEY, _networkSetting.wifi.ssid);
    _preferences.remove(PSWKEY);
    _preferences.putString(PSWKEY, _networkSetting.wifi.pwd);

#endif
    //Disable the auto update of wifi network for avoiding to perform a wifi scan while trying to connect to a wifi network
    _enableNetworkOptionsAutoUpdate = false;
#endif
    _networkSettingReceived = true;
  } else if (_connectReqReceived) {
    _connectReqReceived = false;
    if (!_networkSettingReceived) {
      DEBUG_DEBUG("NetworkConfigurator::%s Connect request received but network settings not received yet", __FUNCTION__);
      _agentManager->setStatusMessage(MessageTypeCodes::PARAMS_NOT_FOUND);
    } else {
      _agentManager->setStatusMessage(MessageTypeCodes::CONNECTING);
      nextState = NetworkConfiguratorStates::CONNECTING;
    }
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleConnecting() {
#ifdef BOARD_HAS_WIFI
  //Disable the auto update of wifi network for avoiding to perform a wifi scan while trying to connect to a wifi network
  _enableNetworkOptionsAutoUpdate = false;
#endif
  _agentManager->poll();  //To keep alive the connection with the configurator
  NetworkConfiguratorStates nextState = connectToNetwork();


  // Exiting from connecting state
  if (nextState != _state && nextState != NetworkConfiguratorStates::CONFIGURED) {
#ifdef BOARD_HAS_WIFI
    _enableNetworkOptionsAutoUpdate = true;
#endif
  }

  return nextState;
}

#if defined(BOARD_HAS_ETHERNET)
void PrintIP(models::ip_addr *ip) {
  int len = 0;
  if (ip->type == IPType::IPv4) {
    len = 4;
  } else {
    len = 16;
  }

  for (int i = 0; i < len; i++) {
    DEBUG_INFO("%d ", ip->bytes[i]);
  }
  DEBUG_INFO("\n");
}
#endif

void NetworkConfigurator::printNetworkSettings() {
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
