#include "Arduino_ConnectionHandlerDefinitions.h"
#include "NetworkConfigurator.h"
constexpr char *SSIDKEY {"SSID"};
constexpr char *PSWKEY {"PASSWORD"};

NetworkConfigurator::NetworkConfigurator(AgentsConfiguratorManager &agentManager, ConnectionHandler &connectionHandler):
_agentManager{&agentManager},
_connectionHandler{&connectionHandler}
{
}

bool NetworkConfigurator::begin(bool initConfiguratorIfConnectionFails, String cause, bool resetStorage){
  _networkSettingReceived = false;
  _enableAutoReconnect = true;
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
#ifdef BOARD_HAS_WIFI
  _networkSetting.type = NetworkAdapter::WIFI;
#endif

#ifdef BOARD_HAS_WIFI
  String SSID = _preferences.getString(SSIDKEY, "");
  String Password = _preferences.getString(PSWKEY, "");
  //preferences.end();
  if (SSID != "" && Password != ""){
    Serial.print("found credential WiFi: ");
    Serial.print(SSID);
    Serial.print(" PSW: ");
    Serial.println(Password);
    
    memcpy(_networkSetting.values.wifi.ssid, SSID.c_str(), SSID.length());
    memcpy(_networkSetting.values.wifi.pwd, Password.c_str(), Password.length());
    _networkSettingReceived = true;
  }
#endif

  if(_networkSettingReceived){
    if(initConfiguratorIfConnectionFails){
      _state = handleConnecting();
      if(_state != NetworkConfiguratorStates::CONNECTED){
        _agentManager->begin();
      }
    }
  }else{
    _agentManager->begin();
    if(cause != ""){
      _agentManager->setConnectionStatus(cause);
    }
    _state = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }
  
  return true;
}

NetworkConfiguratorStates NetworkConfigurator::poll(){
  
  switch(_state){
    case NetworkConfiguratorStates::WAITING_FOR_CONFIG: _state = handleWaitingForConf(); break;
    case NetworkConfiguratorStates::CONNECTING: _state = handleConnecting(); break;
    case NetworkConfiguratorStates::CONFIGURED                               break;
    case NetworkConfiguratorStates::END                                      break;
  }
  if(_state == NetworkConfiguratorStates::TRY_TO_CONNECT){
    forceConnect = true;
  }else if(_state == NetworkConfiguratorStates::WAITING_FOR_CONFIG){
    

    }
  }else if( _state == NetworkConfiguratorStates::CONFIGURED ||  _state == NetworkConfiguratorStates::END){
    return _state;
  }

  if(_networkSettingReceived && _agentManager->isConfigInProgress() != true && (millis()-_lastConnectionAttempt > 120000 && _enableAutoReconnect)){
    _state = NetworkConfiguratorStates::CONNECTING;
  }

  return _state;
}

bool NetworkConfigurator::end(){
  _lastConnectionAttempt = 0;
  _state = NetworkConfiguratorStates::END;
  _preferences.end();
  return _agentManager->end();
}
  

NetworkConnectionState NetworkConfigurator::connectToNetwork(){
  _connectionHandler->updateSetting(_networkSetting);
#ifdef BOARD_HAS_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.print(_networkSetting.values.wifi.ssid);
  Serial.print(" PSW: ");
  Serial.println(_networkSetting.values.wifi.pwd);
#endif
  uint32_t startAttempt = millis();
  NetworkConnectionState res = NetworkConnectionState::DISCONNECTED;
  do{
    res = _connectionHandler->check();
    delay(500);
  }while(res != NetworkConnectionState::CONNECTED && millis()- startAttempt<35000);

  if(res != NetworkConnectionState::CONNECTED){
#ifdef BOARD_HAS_WIFI
    WiFi.end();
#endif
  }
  

  return res;

}

String NetworkConfigurator::decodeConnectionErrorMessage(NetworkConnectionState err){
  switch (err){
    case NetworkConnectionState::ERROR:
      return "HW error";
    case NetworkConnectionState::INIT:
      return "Peripheral in idle";
    case   NetworkConnectionState::CLOSED:
      return "Peripheral stopped";
    case NetworkConnectionState::DISCONNECTED:
      return "Connection lost";
      //the connection handler doesn't have a state of "Fail to connect", in case of invalid credetials or 
      //missing wifi network the FSM stays on Connecting state so use the connecting state to detect the fail to connect
    case NetworkConnectionState::CONNECTING: 
      return "failed to connect";
    default:
      return "generic error";
  }
}

NetworkConfiguratorStates NetworkConfigurator::handleWaitingForConf(){
  NetworkConfiguratorStates nextState = _state;
  AgentsConfiguratorManagerStates configurationState = _agentManager->poll();
  if (configurationState == AgentsConfiguratorManagerStates::CONFIG_RECEIVED){
    if(_agentManager->getNetworkConfigurations(&_networkSetting)){
#ifdef BOARD_HAS_WIFI
      Serial.print("Cred received: SSID: ");
      Serial.print(_networkSetting.values.wifi.ssid);
      Serial.print("PSW: ");
      Serial.println(_networkSetting.values.wifi.pwd);

      _preferences.remove(SSIDKEY);
      _preferences.putString(SSIDKEY, _networkSetting.values.wifi.ssid);
      _preferences.remove(PSWKEY);
      _preferences.putString(PSWKEY,_networkSetting.values.wifi.pwd);

#endif
      _networkSettingReceived = true;
      nextState = NetworkConfiguratorStates::CONNECTING;
    }else{
      Serial.println("invalid credentials");
      _agentManager->setConnectionStatus("invalid credentials");
    }
  }
  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleConnecting(){
  NetworkConfiguratorStates nextState = _state;
  NetworkConnectionState status = connectToNetwork();
  if (status == NetworkConnectionState::CONNECTED)
    {
      String res = "connected";
      Serial.println(res);
      _agentManager->setConnectionStatus(res);
      _enableAutoReconnect = false;
      delay(3000);
      nextState = NetworkConfiguratorStates::CONFIGURED;
    }else{
      String error = decodeConnectionErrorMessage(status);
      Serial.print("connection fail: ");
      Serial.println(error);
      /*if(_state == NetworkConfiguratorStates::TRY_TO_CONNECT){
        _agentManager->begin();
      }*/

      _agentManager->setConnectionStatus(error);        
      _lastConnectionAttempt = millis();
      nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
    }
  }
  return nextState;
}
