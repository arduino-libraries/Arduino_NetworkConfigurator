/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ConnectionHandlerDefinitions.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
#include "Arduino_ConnectionHandler.h"
#ifdef BOARD_HAS_WIFI
#include "WiFiConnectionHandler.h"
#endif
#include "NetworkConfigurator.h"
#define SERVICE_ID_FOR_AGENTMANAGER 0xB0
constexpr char *SSIDKEY {"SSID"};
constexpr char *PSWKEY {"PASSWORD"};

NetworkConfigurator::NetworkConfigurator(AgentsConfiguratorManager &agentManager, GenericConnectionHandler &connectionHandler):
_agentManager{&agentManager},
_connectionHandler{&connectionHandler}
{
}

bool NetworkConfigurator::begin(bool initConfiguratorIfConnectionFails, String cause, bool resetStorage){
  _networkSettingReceived = false;
  _enableAutoReconnect = true;
  _initConfiguratorIfConnectionFails = initConfiguratorIfConnectionFails;
  _initReason = cause;
  _state = NetworkConfiguratorStates::INIT;
  _startConnectionAttempt = 0;
#ifdef ARDUINO_UNOR4_WIFI
  while(1){
    if (!_preferences.begin("my-app", false)) {
      Serial.println("Cannot initialize preferences");
      _preferences.end();
    }else{
      Serial.println("preferences initialized");
      break;
    }
  }

  if (resetStorage){
    _preferences.clear();
  }
#endif
#ifdef BOARD_HAS_WIFI
#ifndef ARDUINO_ARCH_ESP32
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
#endif
  _networkSetting.type = NetworkAdapter::WIFI;
  if(!_agentManager->addRequestHandler(RequestType::SCAN, scanReqHandler)){
    Serial.println("error adding scan request handler");
  }
#endif

  if(!_agentManager->addRequestHandler(RequestType::CONNECT, connectReqHandler)){
    Serial.println("error adding connect request handler");
  }

  if(!_agentManager->addReturnNetworkSettingsCallback(setNetworkSettingsHandler)){
    Serial.println("error adding network settings handler");
  }

  updateNetworkOptions();

  return true;
}

NetworkConfiguratorStates NetworkConfigurator::poll(){
  
  switch(_state){
    case NetworkConfiguratorStates::INIT:               _state = handleInit          (); break;
    case NetworkConfiguratorStates::WAITING_FOR_CONFIG: _state = handleWaitingForConf(); break;
    case NetworkConfiguratorStates::CONNECTING:         _state = handleConnecting    (); break;
    case NetworkConfiguratorStates::CONFIGURED:                                          break;
    case NetworkConfiguratorStates::END:                                                 return _state;
  }

  //Handle if the scan request command is received
  if(_scanReqReceived){
    _scanReqReceived = false;
    updateNetworkOptions();
  }

  //Check if update the network options 
  if(_enableNetworkOptionsAutoUpdate && (millis() - _lastOptionUpdate > 120000)){ 
    //if board doesn't support wifi and ble connectivty at the same time and the configuration is in progress skip updateAvailableOptions
#ifdef BOARD_HAS_WIFI
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || \
  defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
  if( _agentManager->isConfigInProgress() == true){
    return _state;
  }
#endif
#endif
    updateNetworkOptions();
  }

  if(_networkSettingReceived && _agentManager->isConfigInProgress() != true && (millis()-_lastConnectionAttempt > 120000 && _enableAutoReconnect)){
    _state = NetworkConfiguratorStates::CONNECTING;
  }

  return _state;
}

bool NetworkConfigurator::end(){
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
  

NetworkConfiguratorStates NetworkConfigurator::connectToNetwork(){
  NetworkConfiguratorStates nextState = _state;
  Serial.println("Connecting to network");
  printNetworkSettings();
  if(_startConnectionAttempt == 0){
    _startConnectionAttempt = millis();
  }
  NetworkConnectionState connectionRes = NetworkConnectionState::DISCONNECTED;

  connectionRes = _connectionHandler->check();
  delay(250); //TODO remove
  if(connectionRes == NetworkConnectionState::CONNECTED){
    _startConnectionAttempt = 0;
    Serial.println("Connected");
    _agentManager->setStatusMessage(MessageTypeCodes::CONNECTED);
    _enableAutoReconnect = false;
    delay(3000);//TODO remove
    nextState = NetworkConfiguratorStates::CONFIGURED;
  }else if(connectionRes != NetworkConnectionState::CONNECTED && millis() -_startConnectionAttempt >NC_CONNECTION_TIMEOUT)//connection attempt failed
  {
#ifdef BOARD_HAS_WIFI
    WiFi.end();
#endif
    _startConnectionAttempt = 0;
    int errorCode;
    String errorMsg = decodeConnectionErrorMessage(connectionRes, &errorCode);
    Serial.print("connection fail: ");
    Serial.println(errorMsg);
    if(_initReason != ""){
      Serial.print("init reason: ");
      Serial.println(_initReason);
      _agentManager->setStatusMessage(MessageTypeCodes::CONNECTION_LOST);
    }else{
      _agentManager->setStatusMessage((MessageTypeCodes)errorCode);  
    }
          
    _lastConnectionAttempt = millis();
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
}

bool NetworkConfigurator::updateNetworkOptions()
{
#ifdef BOARD_HAS_WIFI
  Serial.println("Scanning");
  _agentManager->setStatusMessage(MessageTypeCodes::SCANNING);//Notify before scan

  WiFiOption wifiOptObj;

  String errMsg;
  if(!scanWiFiNetworks(wifiOptObj, &errMsg)){
    Serial.println("error scanning for wifi networks");
    Serial.println(errMsg);
    
    _agentManager->setStatusMessage(MessageTypeCodes::HW_ERROR_CONN_MODULE);
    
    return false;
  }

  NetworkOptions netOption = { NetworkOptionsClass::WIFI, wifiOptObj };
#endif
  
  _agentManager->setNetworkOptions(netOption);

  _lastOptionUpdate = millis();

  return true;
}

#ifdef BOARD_HAS_WIFI
bool NetworkConfigurator::scanWiFiNetworks(WiFiOption &wifiOptObj, String *err)
{
  Serial.println("Scanning");
  wifiOptObj.numDiscoveredWiFiNetworks = 0;
    // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    *err = "Communication with WiFi module failed!";
    return false;
  }

  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    *err = "Couldn't get any WiFi connection";
    return false;
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid && thisNet < MAX_WIFI_NETWORKS; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.println(" dBm");

    wifiOptObj.discoveredWifiNetworks[thisNet].SSID = const_cast<char*>(WiFi.SSID(thisNet));

    
    wifiOptObj.discoveredWifiNetworks[thisNet].SSIDsize = strlen(WiFi.SSID(thisNet));
    wifiOptObj.discoveredWifiNetworks[thisNet].RSSI = WiFi.RSSI(thisNet);

    wifiOptObj.numDiscoveredWiFiNetworks++;
  }

  WiFi.end();
  return true;
}
#endif

void NetworkConfigurator::scanReqHandler()
{
  _scanReqReceived = true;
}

void NetworkConfigurator::connectReqHandler()
{
  _connectReqReceived = true;
}

void NetworkConfigurator::setNetworkSettingsHandler(models::NetworkSetting *netSetting)
{
  memcpy(&_networkSetting, netSetting, sizeof(models::NetworkSetting));
  _networkSettingsToHandleReceived = true;
}

String NetworkConfigurator::decodeConnectionErrorMessage(NetworkConnectionState err, int *errorCode){
  switch (err){
    case NetworkConnectionState::ERROR:
      *errorCode = (int) MessageTypeCodes::HW_ERROR_CONN_MODULE;
      return "HW error";
    case NetworkConnectionState::INIT:
      *errorCode = (int) MessageTypeCodes::WIFI_IDLE;
      return "Peripheral in idle";
    case   NetworkConnectionState::CLOSED:
      *errorCode = (int) MessageTypeCodes::WIFI_STOPPED;
      return "Peripheral stopped";
    case NetworkConnectionState::DISCONNECTED:
      *errorCode = (int) MessageTypeCodes::DISCONNECTED;
      return "Disconnected";
      //the connection handler doesn't have a state of "Fail to connect", in case of invalid credetials or 
      //missing wifi network the FSM stays on Connecting state so use the connecting state to detect the fail to connect
    case NetworkConnectionState::CONNECTING: 
      *errorCode = (int) MessageTypeCodes::FAILED_TO_CONNECT;
      return "failed to connect";
    default:
      *errorCode = (int) MessageTypeCodes::ERROR;
      return "generic error";
  }
}

NetworkConfiguratorStates NetworkConfigurator::handleInit(){
  NetworkConfiguratorStates nextState = _state;
  if(!_networkSettingReceived){
#ifdef BOARD_HAS_WIFI
#ifdef ARDUINO_UNOR4_WIFI
    String SSID = _preferences.getString(SSIDKEY, "");
    String Password = _preferences.getString(PSWKEY, "");
    //preferences.end();
    if (SSID != "" && Password != ""){
      Serial.print("found credential WiFi: ");
      Serial.print(SSID);
      Serial.print(" PSW: ");
      Serial.println(Password);
      
      memcpy(_networkSetting.wifi.ssid, SSID.c_str(), SSID.length());
      memcpy(_networkSetting.wifi.pwd, Password.c_str(), Password.length());
      _networkSettingReceived = true;
    }
#endif
#endif
  }
  if(_networkSettingReceived){
    if(!_connectionHandlerIstantiated){
      if(!_connectionHandler->updateSetting(_networkSetting)){
        Serial.println("Network parameters not supported");
        _agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER);
        return NetworkConfiguratorStates::WAITING_FOR_CONFIG;
      }
      _connectionHandlerIstantiated = true;
    }

    if(_initConfiguratorIfConnectionFails){
      _enableAutoReconnect = false;
      nextState = connectToNetwork();

      if(nextState == _state){
        return _state;
      }

      if(nextState != NetworkConfiguratorStates::CONFIGURED){
        _enableAutoReconnect = true;

        if(!_agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER)){
          Serial.println("failed to init agent");
        }
        if(_initReason != ""){
          _agentManager->setStatusMessage(MessageTypeCodes::CONNECTION_LOST);
        }
      }
    }else{
      nextState = NetworkConfiguratorStates::CONFIGURED;
    }
  }else{

    _agentManager->begin(SERVICE_ID_FOR_AGENTMANAGER);
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleWaitingForConf(){
  NetworkConfiguratorStates nextState = _state;
  _agentManager->poll();
  if(_networkSettingsToHandleReceived){
    _networkSettingsToHandleReceived = false;
    if(_initReason != ""){
      _initReason = ""; //reset initReason if set, for updating the failure reason
    }
   
    if(!_connectionHandler->updateSetting(_networkSetting)){
      Serial.println("Network parameters not supported");
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
    _preferences.putString(PSWKEY,_networkSetting.wifi.pwd);

#endif
    //Disable the auto update of wifi network for avoiding to perform a wifi scan while trying to connect to a wifi network
    _enableNetworkOptionsAutoUpdate = false;
#endif
    _networkSettingReceived = true;
  }else if(_connectReqReceived){
    _connectReqReceived = false;
    if(!_networkSettingReceived){
      Serial.println("Parameters not provided");
      _agentManager->setStatusMessage(MessageTypeCodes::PARAMS_NOT_FOUND);
    }else{
      _agentManager->setStatusMessage(MessageTypeCodes::CONNECTING);
      nextState = NetworkConfiguratorStates::CONNECTING;
    }
  }

  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleConnecting(){
#ifdef BOARD_HAS_WIFI
  //Disable the auto update of wifi network for avoiding to perform a wifi scan while trying to connect to a wifi network
  _enableNetworkOptionsAutoUpdate = false;
#endif
  _agentManager->poll(); //To keep alive the connection with the configurator  
  NetworkConfiguratorStates nextState = connectToNetwork();


  // Exiting from connecting state
  if(nextState != _state && nextState != NetworkConfiguratorStates::CONFIGURED){
#ifdef BOARD_HAS_WIFI
    _enableNetworkOptionsAutoUpdate = true;
#endif  
  }

  return nextState;
}

 #if defined(BOARD_HAS_ETHERNET)
void PrintIP(models::ip_addr *ip){
  int len = 0;
  if(ip->type == IPType::IPv4){
    len = 4;
  }else{
    len = 16;
  }

  for(int i = 0; i < len; i++){
    Serial.print(ip->bytes[i]);
    Serial.print(" ");
  }
  Serial.println();
}
#endif

void NetworkConfigurator::printNetworkSettings(){
  Serial.print("Network settings received: ");
  switch(_networkSetting.type) {
#if defined(BOARD_HAS_WIFI)
      case NetworkAdapter::WIFI:
        Serial.println("WIFI");
        Serial.print("SSID: ");
        Serial.print(_networkSetting.wifi.ssid);
        Serial.print(" PSW: ");
        Serial.println(_networkSetting.wifi.pwd);
        break;
      #endif

      #if defined(BOARD_HAS_ETHERNET)
      case NetworkAdapter::ETHERNET:
          Serial.println("ETHERNET");
          Serial.print("IP: ");
          PrintIP(&_networkSetting.eth.ip);
          Serial.print(" DNS: ");
          PrintIP(&_networkSetting.eth.dns);
          Serial.print(" Gateway: ");
          PrintIP(&_networkSetting.eth.gateway);
          Serial.print(" Netmask: ");
          PrintIP(&_networkSetting.eth.netmask);
          Serial.print(" Timeout: ");
          Serial.println(_networkSetting.eth.timeout);
          Serial.print(" Response timeout: ");
          Serial.println(_networkSetting.eth.response_timeout);
          break;
      #endif

      #if defined(BOARD_HAS_NB)
      case NetworkAdapter::NB:
          Serial.println("NB");
          Serial.print("PIN: ");
          Serial.println(_networkSetting.nb.pin);
          Serial.print(" APN: ");
          Serial.println(_networkSetting.nb.apn);
          Serial.print(" Login: ");
          Serial.println(_networkSetting.nb.login);
          Serial.print(" Pass: ");
          Serial.println(_networkSetting.nb.pass);
          break;
      #endif

      #if defined(BOARD_HAS_GSM)
      case NetworkAdapter::GSM:
          Serial.println("GSM");
          Serial.print("PIN: ");
          Serial.println(_networkSetting.gsm.pin);
          Serial.print(" APN: ");
          Serial.println(_networkSetting.gsm.apn);
          Serial.print(" Login: ");
          Serial.println(_networkSetting.gsm.login);
          Serial.print(" Pass: ");
          Serial.println(_networkSetting.gsm.pass);
          break;
      #endif

      #if defined(BOARD_HAS_CATM1_NBIOT)
      case NetworkAdapter::CATM1:
          Serial.println("CATM1");
          Serial.print("PIN: ");
          Serial.println(_networkSetting.catm1.pin);
          Serial.print(" APN: ");
          Serial.println(_networkSetting.catm1.apn);
          Serial.print(" Login: ");
          Serial.println(_networkSetting.catm1.login);
          Serial.print(" Pass: ");
          Serial.println(_networkSetting.catm1.pass);
          Serial.print(" Band:");
          Serial.println(_networkSetting.catm1.band);
          break;
      #endif

      #if defined(BOARD_HAS_CELLULAR)
      case NetworkAdapter::CELL:
          Serial.println("CELL");
          Serial.print("PIN: ");
          Serial.println(_networkSetting.cell.pin);
          Serial.print(" APN: ");
          Serial.println(_networkSetting.cell.apn);
          Serial.print(" Login: ");
          Serial.println(_networkSetting.cell.login);
          Serial.print(" Pass: ");
          Serial.println(_networkSetting.cell.pass);
          break;
      #endif

      #if defined(BOARD_HAS_LORA)
      case NetworkAdapter::LORA:
          Serial.println("LORA");
          Serial.print("AppEUI: ");
          Serial.println(_networkSetting.lora.appeui);
          Serial.print(" AppKey: ");
          Serial.println(_networkSetting.lora.appkey);
          Serial.print(" Band: ");
          Serial.println(_networkSetting.lora.band);
          Serial.print(" Channel mask: ");
          Serial.println(_networkSetting.lora.channelMask);
          Serial.print(" Device class: ");
          Serial.println(_networkSetting.lora.deviceClass);
      #endif
      default:
          Serial.println("Network type not supported");
      
  }
}
