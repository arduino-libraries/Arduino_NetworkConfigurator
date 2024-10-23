/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

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

#define MAX_VALIDITY_TIME 30000

BLEConfiguratorAgent::BLEConfiguratorAgent():
_confService{"5e5be887-c816-4d4f-b431-9eb34b02f4d9"},
_inputStreamCharacteristic {"0000ffe1-0000-1000-8000-00805f9b34fc", BLEWrite, 256},
_outputStreamCharacteristic{"0000ffe1-0000-1000-8000-00805f9b34fa", BLEIndicate, 64}
{
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::begin(){
  if(_state != AgentConfiguratorStates::END){
    return _state;
  }
  //BLE.debug(Serial);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    return AgentConfiguratorStates::ERROR;
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

  // add the characteristic to the service
  _confService.addCharacteristic(_outputStreamCharacteristic);
  _confService.addCharacteristic(_inputStreamCharacteristic);
  
  // add service
  BLE.addService(_confService);
  Serial.println("BLEConfiguratorAgent::begin starting adv");
  // start advertising
  BLE.advertise();
  _state = AgentConfiguratorStates::INIT;
  Serial.println("BLEConfiguratorAgent::begin started adv");
  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::end(){
  if(_deviceConnected){
    _deviceConnected = false;
    BLE.disconnect();
  }
  if (_state != AgentConfiguratorStates::END){
    BLE.stopAdvertise();
    BLE.end();
    Packet.clear();
    _outputMessages.clear();
    _inputMessages.clear();
    _state = AgentConfiguratorStates::END;
  }

  return _state;
}

ConfiguratorAgent::AgentConfiguratorStates BLEConfiguratorAgent::poll(){
  BLE.poll();
  if(_bleEvent.newEvent){
    _bleEvent.newEvent = false;
    switch (_bleEvent.type)
    {
    case BLEEventType::CONNECTED:
      _deviceConnected = true;
      _state = AgentConfiguratorStates::PEER_CONNECTED;
      break;
    case BLEEventType::DISCONNECTED:
      _deviceConnected = false;
      Packet.clear();
      _outputMessages.clear();
      _inputMessages.clear();
      _state = AgentConfiguratorStates::INIT;
      break;
    default:
      break;
    }
  }


  if(_deviceConnected){
    if (_inputStreamCharacteristic.written()) {
      Serial.println("inputStreamCharacteristic written"); 
      int receivedDataLen = _inputStreamCharacteristic.valueLength();
      const uint8_t* val = _inputStreamCharacteristic.value();
      PacketManager::ReceivingState res;
      PacketManager::ReceivedData receivedData;
      for (int i = 0; i < receivedDataLen; i++) {     
        res = Packet.handleReceivedByte(receivedData, val[i]);
        if(res == PacketManager::ReceivingState::ERROR){
          Serial.println("Error receiving packet");
          sendNak();
          transmitStream();
          _inputStreamCharacteristic.writeValue("");
          break;
        }else if(res == PacketManager::ReceivingState::RECEIVED){
          switch (receivedData.type)
          {
          case PacketManager::MessageType::DATA:
            {
              Serial.println("Received data packet");
              _inputMessages.addMessage(receivedData.payload);
              //Consider all sent data as received
              while(_outputMessages.numMessages() > 0){
                _outputMessages.popMessage();
              }
              _state = AgentConfiguratorStates::RECEIVED_DATA;
            }
            break;
          case PacketManager::MessageType::RESPONSE:
            {
              Serial.println("Received response packet");
              for(OutputPacketBuffer *outputMsg = _outputMessages.nextMessage(); outputMsg != nullptr; outputMsg = _outputMessages.nextMessage()){
                outputMsg->startProgress();
              }
            }
            break;
          default:
            break;
          }
        }
      }
    }
  
    if(_outputStreamCharacteristic.subscribed() && _outputMessages.numMessages() > 0){
      transmitStream();
    }
  }

  if(_outputMessages.numMessages()>0){
    checkOutputPacketValidity();
  }
  
  return _state;
}

bool BLEConfiguratorAgent::receivedDataAvailable()
{
  return _inputMessages.numMessages() > 0;
}

bool BLEConfiguratorAgent::getReceivedData(uint8_t *data, size_t *len)
{
  if(_inputMessages.numMessages() > 0){
    InputPacketBuffer *msg = _inputMessages.frontMessage();
    if(msg->len() <= *len){
      *len = msg->len();
      memset(data, 0x00, *len);
      memcpy(data, &(*msg)[0], *len);
      _inputMessages.popMessage();
      if(_inputMessages.numMessages() == 0){
        _state = AgentConfiguratorStates::PEER_CONNECTED;
      }
      return true;
    }
  }
  return false;
}

size_t BLEConfiguratorAgent::getReceivedDataLength()
{
  if(_inputMessages.numMessages() > 0){
    InputPacketBuffer * msg = _inputMessages.frontMessage();
    return msg->len();
  }
  return 0;
}

bool BLEConfiguratorAgent::sendData(const uint8_t *data, size_t len)
{
  return sendData(PacketManager::MessageType::DATA, data, len);
}

bool BLEConfiguratorAgent::isPeerConnected()
{
  return _deviceConnected;
}

void BLEConfiguratorAgent::blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  _bleEvent.type = BLEEventType::CONNECTED;
  _bleEvent.newEvent = true;

  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void BLEConfiguratorAgent::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  _bleEvent.type = BLEEventType::DISCONNECTED;
  _bleEvent.newEvent = true;
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
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

BLEConfiguratorAgent::TransmissionResult BLEConfiguratorAgent::transmitStream(){
  
  if(!_outputStreamCharacteristic.subscribed()){
    Serial.println("outputCharacteristic not subscribed");
    return TransmissionResult::PEER_NOT_AVAILABLE;
  }
  if(_outputMessages.numMessages() == 0){
    Serial.println("outputMessages empty");
    return TransmissionResult::COMPLETED;
  }

  TransmissionResult res = TransmissionResult::COMPLETED;

  for(OutputPacketBuffer *outputMsg = _outputMessages.nextMessage(0); outputMsg != nullptr; outputMsg = _outputMessages.nextMessage()){
    if(outputMsg->hasBytesToSend()){
      res = TransmissionResult::NOT_COMPLETED;
      outputMsg->incrementBytesSent(_outputStreamCharacteristic.write(&(*outputMsg)[outputMsg->bytesSent()], outputMsg->bytesToSend()));
      Serial.print("outputCharacteristic transferred: ");
      Serial.print(outputMsg->bytesSent());
      Serial.print(" of ");
      Serial.println(outputMsg->len());
      if(!outputMsg->hasBytesToSend()){
        Serial.println("outputCharacteristictransfer completed");
      }
      delay(500);
      break;
    }
  }

  return res;
}

bool BLEConfiguratorAgent::sendNak()
{
  uint8_t data = 0x03;
  return sendData(PacketManager::MessageType::RESPONSE, &data, sizeof(data));
}

void BLEConfiguratorAgent::checkOutputPacketValidity()
{
  for(OutputPacketBuffer *outputMsg = _outputMessages.nextMessage(0); outputMsg != nullptr; outputMsg = _outputMessages.nextMessage()){
    if(outputMsg->getValidityTs() != 0 && outputMsg->getValidityTs() < millis()){
      Serial.println("expired output message");
      outputMsg->reset();
    }
  }
}

bool BLEConfiguratorAgent::sendData(PacketManager::MessageType type, const uint8_t *data, size_t len)
{
  OutputPacketBuffer outputMsg;
  outputMsg.setValidityTs(millis() + MAX_VALIDITY_TIME);

  if(!Packet.createPacket(outputMsg, type, data, len)){
    Serial.println("Failed to create packet");
    return false;
  }
  if(!_outputMessages.addMessage(outputMsg)){
    Serial.println("Failed to add message to outputMessages");
    return false;
  }
  
  TransmissionResult res = TransmissionResult::NOT_COMPLETED;
  do{
    res = transmitStream();
    if(res == TransmissionResult::PEER_NOT_AVAILABLE){
      break;
    }
  }while(res == TransmissionResult::NOT_COMPLETED);

  return true;
}

BLEConfiguratorAgent BLEAgent;
