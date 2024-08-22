#include "BLEStringCharacteristic.h"
#include "BLECharacteristic.h"
#include "BLEConfiguratorAgent.h"


BLEConfiguratorAgent::BLEConfiguratorAgent():
_confService{"5e5be887-c816-4d4f-b431-9eb34b02f4d9"},
_ssidCharacteristic{"b0f3f174-f834-440b-be8b-e699047e33d1", BLERead | BLEWrite, 64},
_pswCharacteristic{"ea1c6589-3817-4782-9d8b-c4a03708bb52", BLERead | BLEWrite, 64},
_wifiListCharacteristic{"b03033b6-0ee3-489b-b6b5-04efa515cbdf", BLERead|BLENotify|BLEIndicate, 64},
_statusCharacteristic{"34776508-336f-4546-9f65-fbd9c2bc42d5", BLERead | BLENotify | BLEIndicate, 256},
_errorCode{""}
{
}

ConfiguratorStates BLEConfiguratorAgent::begin(){
  _optionsChar = (BLECharacteristic*)&_wifiListCharacteristic;
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    return ConfiguratorStates::ERROR;
  }

  BLE.setLocalName("Arduino-provisioning");
  BLE.setAdvertisedService(_confService);

  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler );
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler );

  // assign event handlers for characteristic
  _ssidCharacteristic.setEventHandler(BLEWritten, this->ssidCharacteristicWritten);
  _pswCharacteristic.setEventHandler(BLEWritten, this->pswCharacteristicWritten);
  _wifiListCharacteristic.setEventHandler(BLESubscribed, this->wifiListCharacteristicSubscribed);
  // add the characteristic to the service
  _confService.addCharacteristic(_ssidCharacteristic);
  _confService.addCharacteristic(_pswCharacteristic);
  _confService.addCharacteristic(_statusCharacteristic);
  _confService.addCharacteristic(_wifiListCharacteristic);
  // add service
  BLE.addService(_confService);

  // start advertising
  BLE.advertise();
  _state = ConfiguratorStates::INIT;
  _paramsCompleted[0] = 0;
  _paramsCompleted[1] = 0;
  return _state;
}

ConfiguratorStates BLEConfiguratorAgent::end(){
  if(_deviceConnected){
    _deviceConnected = false;
    BLE.disconnect();
  }
  if (_state != ConfiguratorStates::END){
    BLE.stopAdvertise();
    BLE.end();
    _state = ConfiguratorStates::END;
  }

  return _state;
}

ConfiguratorStates BLEConfiguratorAgent::poll(){
  BLE.poll();
  if (_paramsCompleted[0] == 1 && _paramsCompleted[1] == 1){
    _state = ConfiguratorStates::CONFIG_RECEIVED;
  }


  return _state;

}


bool BLEConfiguratorAgent::isPeerConnected(){
  return _deviceConnected;
}

bool BLEConfiguratorAgent::getNetworkConfigurations(models::NetworkSetting *netSetting){
  if(_state == ConfiguratorStates::CONFIG_RECEIVED){
    if (netSetting->type == NetworkAdapter::WIFI){
      if(_ssidCharacteristic.value().length() >= 33 || _pswCharacteristic.value().length() >= 64){
        Serial.print("invalid cred length SSID: ");
        Serial.print(_ssidCharacteristic.value().length());
        Serial.print("invalid cred length PSW: ");
        Serial.print(_pswCharacteristic.value().length());

        return false;
      }
      memset(netSetting->values.wifi.ssid, 0x00, 33);
      memset(netSetting->values.wifi.pwd, 0x00, 64);
      memcpy(netSetting->values.wifi.ssid, _ssidCharacteristic.value().c_str(), _ssidCharacteristic.value().length());
      memcpy(netSetting->values.wifi.pwd, _pswCharacteristic.value().c_str(), _pswCharacteristic.value().length());
      return true; 
    }else{
      Serial.println("NetSettings type not supported");
      return false;
    }
    
  }

  Serial.println("No cred received");
  return false;
}

bool BLEConfiguratorAgent::setAvailableOptions(NetworkOptions netOptions){
  Serial.println("Setting options");
  _netOptions = netOptions;
  if(netOptions.type == NetworkOptionsClass::WIFI){
    Serial.println("copy wifi option");
    memset(_netOptions.option.wifi.discoveredWifiNetworks, 0x00, sizeof(DiscoveredWiFiNetwork)*MAX_WIFI_NETWORKS );
    memcpy(_netOptions.option.wifi.discoveredWifiNetworks, netOptions.option.wifi.discoveredWifiNetworks, sizeof(DiscoveredWiFiNetwork)*MAX_WIFI_NETWORKS  );
  }
  
  if(_deviceConnected && _optionsChar->subscribed()){
    sendOptions(*_optionsChar);
  }
  if(_state == ConfiguratorStates::REQUEST_UPDATE_OPT){
    _state = ConfiguratorStates::WAITING_FOR_CONFIG;
  }
  

  return true;
}

bool BLEConfiguratorAgent::setErrorCode(String error){
  _errorCode = error;
  
  _statusCharacteristic.setValue(_errorCode);

  if(_state == ConfiguratorStates::CONFIG_RECEIVED){
    _paramsCompleted[0] = 0;
    _paramsCompleted[1] = 0;
    _state = ConfiguratorStates::WAITING_FOR_CONFIG;
  }

  return true;
}

void BLEConfiguratorAgent::blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  _deviceConnected = true;
  _state = ConfiguratorStates::WAITING_FOR_CONFIG;
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void BLEConfiguratorAgent::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  _state = ConfiguratorStates::INIT;
  _deviceConnected = false;
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void BLEConfiguratorAgent::ssidCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.println("Characteristic event, written ssid");

  
  _paramsCompleted[0] = 1;


}


void BLEConfiguratorAgent::pswCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.println("Characteristic event, written psw");

  _paramsCompleted[1] = 1;

}

int BLEConfiguratorAgent::computeOptionsToSendLength(){

  int length = 0;

  if(_netOptions.type == NetworkOptionsClass::WIFI){
    for(uint8_t i =0; i <_netOptions.option.wifi.numDiscoveredWiFiNetworks; i++){
      length += 6;
      length += _netOptions.option.wifi.discoveredWifiNetworks[i].SSIDsize;
    }

  }
  return length;


}

void BLEConfiguratorAgent::wifiListCharacteristicSubscribed(BLEDevice central, BLECharacteristic characteristic){

  sendOptions(characteristic);
  
}

void BLEConfiguratorAgent::sendOptions(BLECharacteristic characteristic){
  Serial.println("Sending networks");
  int dataLengthToSend = computeOptionsToSendLength();
  char bufferToSend[dataLengthToSend];
  int handle = 0;
  
  if(_netOptions.type == NetworkOptionsClass::WIFI){
    Serial.print("networks to send: ");
    Serial.println(_netOptions.option.wifi.numDiscoveredWiFiNetworks);

    for(uint8_t i =0; i <_netOptions.option.wifi.numDiscoveredWiFiNetworks; i++){

      Serial.print("Sending: ");
      Serial.println(i);

        
      memcpy(&bufferToSend[handle], _netOptions.option.wifi.discoveredWifiNetworks[i].SSID, strlen(_netOptions.option.wifi.discoveredWifiNetworks[i].SSID));
      handle += _netOptions.option.wifi.discoveredWifiNetworks[i].SSIDsize;
      bufferToSend[handle++] = ';';
      char rssi[5];
      itoa(_netOptions.option.wifi.discoveredWifiNetworks[i].RSSI,rssi, 10);
      memcpy(&bufferToSend[handle], rssi, strlen(rssi));
      handle += strlen(rssi);
      bufferToSend[handle++] = '|';


      if(handle > dataLengthToSend){
        Serial.print("Error more space required than provided: ");
        Serial.print(handle);
        Serial.print("/");
        Serial.println(dataLengthToSend);
        break;
      }

    }
  }
  
  bufferToSend[handle] = '\0';

  dataLengthToSend = handle;
  
  int transferredBytes = 0;

  while(transferredBytes < dataLengthToSend){
    transferredBytes += characteristic.write(&bufferToSend[transferredBytes]);
    Serial.print("transferred: ");
    Serial.print(transferredBytes);
    Serial.print(" of ");
    Serial.println(dataLengthToSend);
    delay(1000);
  }
}
