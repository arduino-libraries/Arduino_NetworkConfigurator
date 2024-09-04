#include "BLEStringCharacteristic.h"
#include "BLECharacteristic.h"
#include "BLEConfiguratorAgent.h"
#define BASE_LOCAL_DEVICE_NAME "Arduino"

#if defined(ARDUINO_SAMD_MKRWIFI1010)
#define DEVICE_NAME " MKR 1010 "
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
#define DEVICE_NAME " NANO 33 IoT " 
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2)
#define DEVICE_NAME " UNO WiFi R2 " 
#elif defined (ARDUINO_NANO_RP2040_CONNECT)
#define DEVICE_NAME " NANO RP2040 " 
#elif defined(ARDUINO_PORTENTA_H7_M7)
#define DEVICE_NAME " Portenta H7 "
#elif defined(ARDUINO_PORTENTA_C33)
#define DEVICE_NAME " Portenta C33 "
#elif defined(ARDUINO_NICLA_VISION)
#define DEVICE_NAME " Nicla Vision "
#elif defined(ARDUINO_OPTA)
#define DEVICE_NAME " Opta "
#elif defined(ARDUINO_GIGA)
#define DEVICE_NAME " Giga "
#elif defined(ARDUINO_UNOR4_WIFI)
#define DEVICE_NAME " UNO R4 WiFi "
#endif

#define LOCAL_NAME BASE_LOCAL_DEVICE_NAME DEVICE_NAME

BLEConfiguratorAgent::BLEConfiguratorAgent():
_confService{"5e5be887-c816-4d4f-b431-9eb34b02f4d9"},
_ssidCharacteristic{"b0f3f174-f834-440b-be8b-e699047e33d1", BLERead | BLEWrite, 64},
_pswCharacteristic{"ea1c6589-3817-4782-9d8b-c4a03708bb52", BLERead | BLEWrite, 64},
_wifiListCharacteristic{"b03033b6-0ee3-489b-b6b5-04efa515cbdf", BLERead|BLENotify|BLEIndicate, 64},
_statusCharacteristic{"34776508-336f-4546-9f65-fbd9c2bc42d5", BLERead | BLENotify | BLEIndicate, 256},
_statusCode{""}
{
}

ConfiguratorStates BLEConfiguratorAgent::begin(){
  _optionsChar = (BLECharacteristic*)&_wifiListCharacteristic;
  if(_state != ConfiguratorStates::END){
    return _state;
  }
  //BLE.debug(Serial);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    return ConfiguratorStates::ERROR;
  }
  _localName = generateLocalDeviceName();
  Serial.print("Device Name: ");
  Serial.println(_localName);
  if(!BLE.setLocalName(_localName.c_str())){
    Serial.println("fail to set local name");
  }
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
  Serial.println("BLEConfiguratorAgent::begin starting adv");
  // start advertising
  BLE.advertise();
  _state = ConfiguratorStates::INIT;
  _paramsCompleted[0] = 0;
  _paramsCompleted[1] = 0;
  Serial.println("BLEConfiguratorAgent::begin started adv");
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
    if(_optionsBuf){
      Serial.println("clear optionsBuf");
      _optionsBuf.reset();
    }
  
    _state = ConfiguratorStates::END;
  }

  return _state;
}

ConfiguratorStates BLEConfiguratorAgent::poll(){
  BLE.poll();
  if (_paramsCompleted[0] == 1 && _paramsCompleted[1] == 1){
    _state = ConfiguratorStates::CONFIG_RECEIVED;
  }

  if(_hasOptionsTosend){

    if(_bytesOptionsSent < _bytesToSend){
      _bytesOptionsSent += _optionsChar->write(&_optionsBuf[_bytesOptionsSent]);
      Serial.print("transferred: ");
      Serial.print(_bytesOptionsSent);
      Serial.print(" of ");
      Serial.println(_bytesToSend);
      delay(500);
    }else{
      _bytesOptionsSent = 0;
      _bytesToSend = 0;
      _hasOptionsTosend = false;
      _optionsBuf.reset();
      Serial.println("transfer completed");
    }
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
    sendOptions();
  }
  if(_state == ConfiguratorStates::REQUEST_UPDATE_OPT){
    _state = ConfiguratorStates::WAITING_FOR_CONFIG;
  }
  

  return true;
}

bool BLEConfiguratorAgent::setInfoCode(String info){
  _statusCode = info;
  
  _statusCharacteristic.setValue(_statusCode);

  return true;
}

bool BLEConfiguratorAgent::setErrorCode(String error){
  _statusCode = error;
  _statusCharacteristic.setValue(_statusCode);

  if(_state == ConfiguratorStates::CONFIG_RECEIVED){ 
    _paramsCompleted[0] = 0;//TODO remove when implemented the "READY" command sent by the mobile app
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

  sendOptions();
  
}

void BLEConfiguratorAgent::sendOptions(){
  Serial.println("Sending networks");
  int dataLengthToSend = computeOptionsToSendLength();
  _optionsBuf = std::unique_ptr<char[]>(new char[dataLengthToSend]);
  int handle = 0;
  
  if(_netOptions.type == NetworkOptionsClass::WIFI){
    Serial.print("networks to send: ");
    Serial.println(_netOptions.option.wifi.numDiscoveredWiFiNetworks);

    for(uint8_t i =0; i <_netOptions.option.wifi.numDiscoveredWiFiNetworks; i++){

      Serial.print("Sending: ");
      Serial.println(i);

        
      memcpy(&_optionsBuf[handle], _netOptions.option.wifi.discoveredWifiNetworks[i].SSID, strlen(_netOptions.option.wifi.discoveredWifiNetworks[i].SSID));
      handle += _netOptions.option.wifi.discoveredWifiNetworks[i].SSIDsize;
      _optionsBuf[handle++] = ';';
      char rssi[5];
      itoa(_netOptions.option.wifi.discoveredWifiNetworks[i].RSSI,rssi, 10);
      memcpy(&_optionsBuf[handle], rssi, strlen(rssi));
      handle += strlen(rssi);
      _optionsBuf[handle++] = '|';


      if(handle > dataLengthToSend){
        Serial.print("Error more space required than provided: ");
        Serial.print(handle);
        Serial.print("/");
        Serial.println(dataLengthToSend);
        break;
      }

    }
  }
  
  _optionsBuf[handle] = '\0';

  dataLengthToSend = handle;
  
  _bytesOptionsSent = 0;
  _hasOptionsTosend = true;
  _bytesToSend = dataLengthToSend;
}

String BLEConfiguratorAgent::generateLocalDeviceName(){
  _Static_assert(sizeof(LOCAL_NAME) < 24, "Error BLE device Local Name too long. Reduce DEVICE_NAME length");//Check at compile time if the local name length is valid
  String macAddress = BLE.address();
  String last2Bytes = macAddress.substring(12); //Get the last two bytes of mac address
  String localName = LOCAL_NAME;
  localName.concat("- ");
  localName.concat(last2Bytes);
  return localName;
}