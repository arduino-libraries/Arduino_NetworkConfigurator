#include <algorithm>
#include "AgentsConfiguratorManager.h"
#include "ConfiguratorDefinitions.h"
#include "Arduino_WiFiConnectionHandler.h"
bool AgentsConfiguratorManager::addAgent(ConfiguratorAgent &agent){
  _agentsList.push_back(&agent);
}

bool AgentsConfiguratorManager::begin(){

  for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
    if ((*agent)->begin() != ConfiguratorStates::INIT){
      return false;
    }
  }
  
  updateAvailableOptions();


  _state = AgentsConfiguratorManagerStates::INIT;
  return true;

}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::poll(){
  bool forceUpdateOptions = false;
  
  switch(_state){
    case AgentsConfiguratorManagerStates::INIT               _state = handleInit          (); break;
    case AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS _state = handleConfInProgress(); break;
    case AgentsConfiguratorManagerStates::CONFIG_RECEIVED                                     break;
    case AgentsConfiguratorManagerStates::END                                         return _state;
  }

  if((millis() - _lastOptionUpdate > 120000)/*&& !BLEConf.isPeerConnected()*/){ //uncomment if board doesn't support wifi and ble connectivty at the same time
    updateAvailableOptions();
  }

  return _state;
}

bool AgentsConfiguratorManager::end(){
  std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent){
    agent->end();
  });

   _selectedAgent = nullptr;
   _lastOptionUpdate = 0;

  return true;
}

bool AgentsConfiguratorManager::getNetworkConfigurations(models::NetworkSetting *netSetting){
  if (_state != AgentsConfiguratorManagerStates::CONFIG_RECEIVED || _selectedAgent == nullptr){
    Serial.println("_selectedAgent is empty or the state is not CONFIG_RECEIVED");
    return false;
  }

  return _selectedAgent->getNetworkConfigurations(netSetting);
}

bool AgentsConfiguratorManager::setConnectionStatus(String res){
  //Update all the agents in list and not only the _selectedAgent in case the configuration process is triggered by a disconnection
  for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
    (*agent)->setErrorCode(res);
  }

  if(_state == AgentsConfiguratorManagerStates::CONFIG_RECEIVED){
    _state = AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;
  }

  return true;
}

bool AgentsConfiguratorManager::scanWiFiNetworks(WiFiOption &wifiOptObj){
  Serial.println("Scanning");
  wifiOptObj.numDiscoveredWiFiNetworks = 0;
    // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    return false;
  }
#ifndef ARDUINO_ARCH_ESP32
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
#endif
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a WiFi connection");
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
    wifiOptObj.discoveredWifiNetworks[thisNet].SSID = WiFi.SSID(thisNet);

    
    wifiOptObj.discoveredWifiNetworks[thisNet].SSIDsize = strlen(WiFi.SSID(thisNet));
    wifiOptObj.discoveredWifiNetworks[thisNet].RSSI = WiFi.RSSI(thisNet);

    wifiOptObj.numDiscoveredWiFiNetworks++;
  }
  return true;
}



bool AgentsConfiguratorManager::updateAvailableOptions(){
#ifdef BOARD_HAS_WIFI
  WiFiOption wifiOptObj;


  if(!scanWiFiNetworks(wifiOptObj)){
    Serial.println("error scanning for wifi networks");
    return false;
  }

  NetworkOptions netOption = { NetworkOptionsClass::WIFI, wifiOptObj };
  //netOption.option.wifi = wifiOptObj;
#endif

  for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
    (*agent)->setAvailableOptions(netOption);
  }

  _lastOptionUpdate = millis();
  return true;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleInit(){
    AgentsConfiguratorManagerStates nextState = _state;
    
    for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
      if ((*agent)->poll() == ConfiguratorStates::WAITING_FOR_CONFIG){
        _selectedAgent = *agent;
        nextState = AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;
        Serial.println("found peer connected to agent");
        break;
      }
      
    }
    //stop all other agents
    if(_selectedAgent != nullptr){
      for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
        if (*agent != _selectedAgent){
          (*agent)->end();
        }
      }
    }
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleConfInProgress(){
  AgentsConfiguratorManagerStates nextState = _state;
  
  ConfiguratorStates agentConfState = _selectedAgent->poll();
  switch(agentConfState){
    case ConfiguratorStates::CONFIG_RECEIVED:
      nextState = AgentsConfiguratorManagerStates::CONFIG_RECEIVED;
      break;
    case ConfiguratorStates::REQUEST_UPDATE_OPT:
      updateAvailableOptions();
      break;
    case ConfiguratorStates::INIT:
      //Peer disconnected, restore all stopped agents
      for(std::list<ConfiguratorAgent *>::iterator agent=_agentsList.begin(); agent != _agentsList.end(); ++agent){
        if (*agent != _selectedAgent){
          (*agent)->begin();
        }
      }
      _selectedAgent = nullptr;
      nextState = AgentsConfiguratorManagerStates::INIT; 
      break;
  }


  return nextState;
}
