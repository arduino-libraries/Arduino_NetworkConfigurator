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
  _initConfiguratorIfConnectionFails = initConfiguratorIfConnectionFails;
  _initReason = cause;
  _state = NetworkConfiguratorStates::INIT;
  _startConnectionAttempt = 0;
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

  return true;
}

NetworkConfiguratorStates NetworkConfigurator::poll(){
  
  switch(_state){
    case NetworkConfiguratorStates::INIT:               _state = handleInit          (); break;
    case NetworkConfiguratorStates::WAITING_FOR_CONFIG: _state = handleWaitingForConf(); break;
    case NetworkConfiguratorStates::CONNECTING:         _state = handleConnecting    (); break;
    case NetworkConfiguratorStates::CONFIGURED:                                          break;
    case NetworkConfiguratorStates::END:                                                 break;
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
  

NetworkConfiguratorStates NetworkConfigurator::connectToNetwork(){
  NetworkConfiguratorStates nextState = _state;
  _connectionHandler->updateSetting(_networkSetting);
#ifdef BOARD_HAS_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.print(_networkSetting.values.wifi.ssid);
  Serial.print(" PSW: ");
  Serial.println(_networkSetting.values.wifi.pwd);
#endif
  if(_startConnectionAttempt == 0){
    _startConnectionAttempt = millis();
  }
  NetworkConnectionState connectionRes = NetworkConnectionState::DISCONNECTED;

  connectionRes = _connectionHandler->check();
  delay(250);
  if(connectionRes == NetworkConnectionState::CONNECTED){
    _startConnectionAttempt = 0;
    String msg = "connected";
    Serial.println(msg);
    _agentManager->setConnectionStatus({.type=ConnectionStatusMessageType::INFO, .msg=msg});
    _enableAutoReconnect = false;
    delay(3000);
    nextState = NetworkConfiguratorStates::CONFIGURED;
  }else if(connectionRes != NetworkConnectionState::CONNECTED && millis() -_startConnectionAttempt >35000)//connection attempt failed
  {
#ifdef BOARD_HAS_WIFI
    WiFi.end();
#endif
    _startConnectionAttempt = 0;
    String error = decodeConnectionErrorMessage(connectionRes);
    Serial.print("connection fail: ");
    Serial.println(error);
    /*if(_state == NetworkConfiguratorStates::TRY_TO_CONNECT){
      _agentManager->begin();
    }*/

    _agentManager->setConnectionStatus({.type=ConnectionStatusMessageType::ERROR, .msg=error});        
    _lastConnectionAttempt = millis();
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
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

NetworkConfiguratorStates NetworkConfigurator::handleInit(){
  NetworkConfiguratorStates nextState = _state;
  if(!_networkSettingReceived){
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
  }
  if(_networkSettingReceived){
    if(_initConfiguratorIfConnectionFails){
      _enableAutoReconnect = false;
      nextState = connectToNetwork();

      if(nextState == _state){
        return _state;
      }

      if(nextState != NetworkConfiguratorStates::CONFIGURED){
        _enableAutoReconnect = true;
        if(!_agentManager->begin()){
          Serial.println("failed to init agent");
        }
      }
    }else{
      nextState = NetworkConfiguratorStates::CONFIGURED;
    }
  }else{
    _agentManager->begin();
    if(_initReason != ""){
      _agentManager->setConnectionStatus({.type=ConnectionStatusMessageType::ERROR, .msg=_initReason});
    }
    nextState = NetworkConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return nextState;
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
      String error = "invalid credentials";
      Serial.println(error);
      _agentManager->setConnectionStatus({.type=ConnectionStatusMessageType::ERROR, .msg=error});
    }
  }
  return nextState;
}

NetworkConfiguratorStates NetworkConfigurator::handleConnecting(){
#ifdef BOARD_HAS_WIFI
  //Disable the auto update of wifi network for avoiding to perform a wifi scan while trying to connect to a wifi network
  _agentManager->disableConnOptionsAutoUpdate();
#endif
  _agentManager->poll(); //To keep alive the connection with the configurator  
  NetworkConfiguratorStates nextState = connectToNetwork();


  // Exiting from connecting state
  if(nextState != _state){
#ifdef BOARD_HAS_WIFI
    _agentManager->enableConnOptionsAutoUpdate();
#endif  
  }

  return nextState;
}
