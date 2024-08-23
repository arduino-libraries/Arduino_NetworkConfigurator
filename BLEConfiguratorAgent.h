#pragma once
#include "Arduino.h"
#include <ArduinoBLE.h>
#include "ConfiguratorAgent.h"

class BLEConfiguratorAgent: public ConfiguratorAgent  {
  public:
    BLEConfiguratorAgent();
    ConfiguratorStates begin();
    ConfiguratorStates end();
    ConfiguratorStates poll();
    bool getNetworkConfigurations(models::NetworkSetting *netSetting);
    bool isPeerConnected();
    bool setAvailableOptions(NetworkOptions netOptions);
    bool setErrorCode(String error);
    bool setInfoCode(String info);
    inline AgentTypes getAgentType() { return AgentTypes::BLE;};
  private:
    static inline ConfiguratorStates  _state = ConfiguratorStates::END ;
    static inline volatile bool  _deviceConnected = false;
    static inline NetworkOptions _netOptions={};
    static inline volatile uint8_t _paramsCompleted[2] = {0,0};
    String _statusCode;
    BLEService _confService; // Bluetooth® Low Energy LED Service
    #ifdef BOARD_HAS_WIFI
    // Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
    BLEStringCharacteristic _ssidCharacteristic;

    BLEStringCharacteristic _pswCharacteristic;
    BLEStringCharacteristic _wifiListCharacteristic;
    static inline BLECharacteristic *_optionsChar;
    #endif
    BLEStringCharacteristic _statusCharacteristic;

    

    static void blePeripheralConnectHandler(BLEDevice central);
    static void blePeripheralDisconnectHandler(BLEDevice central);
    static void ssidCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic);
    static void pswCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic);
    static void wifiListCharacteristicSubscribed(BLEDevice central, BLECharacteristic characteristic);
    static int  computeOptionsToSendLength();
    static void sendOptions(BLECharacteristic characteristic);

};

