/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Decoder.h"
#include <settings/settings_default.h>
#if defined(BOARD_HAS_ETHERNET)
#include <IPAddress.h>
#endif

// FIXME move this utility functions
static bool copyCBORStringToArray(CborValue * param, char * dest, size_t dest_size) {
  if (cbor_value_is_text_string(param)) {
    // NOTE: keep in mind that _cbor_value_copy_string tries to put a \0 at the end of the string
    if(_cbor_value_copy_string(param, dest, &dest_size, NULL) == CborNoError) {
      return true;
    }
  }

  return false;
}

// FIXME dest_size should be also returned, the copied byte array can have a different size from the starting one
// for the time being we need this on SHA256 only
static bool copyCBORByteToArray(CborValue * param, uint8_t * dest, size_t dest_size) {
  if (cbor_value_is_byte_string(param)) {
    // NOTE: keep in mind that _cbor_value_copy_string tries to put a \0 at the end of the string
    if(_cbor_value_copy_string(param, dest, &dest_size, NULL) == CborNoError) {
      return true;
    }
  }

  return false;
}

Decoder::Status TimestampProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  TimestampProvisioningMessage* ts = (TimestampProvisioningMessage*) message;

  // Message is composed of a single parameter: a 64-bit unsigned integer
  if (cbor_value_is_integer(param)) {
    uint64_t val = 0;
    if (cbor_value_get_uint64(param, &val) == CborNoError) {
      ts->timestamp = val;
    }
  }

  return Decoder::Status::Complete;
}
#if defined(BOARD_HAS_WIFI)
Decoder::Status WifiConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  // Message is composed of 2 parameters: ssid and password
  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.wifi.ssid, sizeof(provisioningNetworkConfig->networkSetting.wifi.ssid))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.wifi.pwd, sizeof(provisioningNetworkConfig->networkSetting.wifi.pwd))) {
    return Decoder::Status::Error;
  }
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::WIFI;
  return Decoder::Status::Complete;
}
#endif

Decoder::Status CommandsProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  CommandsProvisioningMessage* provisioningCommands = (CommandsProvisioningMessage*) message;

  // Message is composed of a single parameter: a 32-bit signed integer
  if (cbor_value_is_integer(param)) {
    int val = 0;
    if (cbor_value_get_int(param, &val) == CborNoError) {
      provisioningCommands->cmd = val;
    }
  }

  return Decoder::Status::Complete;
}

#if defined(BOARD_HAS_LORA)
Decoder::Status LoRaConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  // Message is composed of 5 parameters: app_eui, app_key, band, channel_mask, device_class
  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.appeui, sizeof(provisioningNetworkConfig->networkSetting.lora.appeui))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.appkey, sizeof(provisioningNetworkConfig->networkSetting.lora.appkey))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  provisioningNetworkConfig->networkSetting.lora.band = models::settingsDefault(NetworkAdapter::LORA).lora.band;
  if (cbor_value_is_integer(param)) {
    int val = 0;
    if (cbor_value_get_int(param, &val) == CborNoError) {
      if(val >= 0){
        //if the peer sends -1 we keep the default value. "-1" is used to keep the default value
        provisioningNetworkConfig->networkSetting.lora.band = val;
      }
    }
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if(!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.channelMask, sizeof(provisioningNetworkConfig->networkSetting.lora.channelMask))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  char deviceClass[LORA_DEVICE_CLASS_SIZE];
  memset(deviceClass, 0x00, sizeof(deviceClass));
  if (!copyCBORStringToArray(param, deviceClass, sizeof(deviceClass))) {
    return Decoder::Status::Error;
  }

  if (deviceClass[0] == '\0') {
    provisioningNetworkConfig->networkSetting.lora.deviceClass = models::settingsDefault(NetworkAdapter::LORA).lora.deviceClass;
  }else {
    provisioningNetworkConfig->networkSetting.lora.deviceClass = (uint8_t)deviceClass[0];
  }
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::LORA;
  return Decoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
Decoder::Status CATM1ConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  CborValue array_iter;
  size_t arrayLength = 0;

  // Message is composed of 5 parameters: pin, band, apn, login and password
  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.pin, sizeof(provisioningNetworkConfig->networkSetting.catm1.pin))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (cbor_value_get_type(param) != CborArrayType) {
    return Decoder::Status::Error;
  }

  cbor_value_get_array_length(param, &arrayLength);

  if(arrayLength > BAND_SIZE) {
    return Decoder::Status::Error;
  }
  provisioningNetworkConfig->networkSetting.catm1.band = 0;
  if (arrayLength == 0) {
    provisioningNetworkConfig->networkSetting.catm1.band = models::settingsDefault(NetworkAdapter::CATM1).catm1.band;
  }

  if (cbor_value_enter_container(param, &array_iter) != CborNoError) {
    return Decoder::Status::Error;
  }

  for(size_t i = 0; i < arrayLength; i++) {
    if (!cbor_value_is_unsigned_integer(&array_iter)) {
      return Decoder::Status::Error;
    }

    int val = 0;
    if (cbor_value_get_int(&array_iter, &val) != CborNoError) {
      return Decoder::Status::Error;
    }

    provisioningNetworkConfig->networkSetting.catm1.band |= val;

    if (cbor_value_advance(&array_iter) != CborNoError) {
      return Decoder::Status::Error;
    }
  }

  if (cbor_value_leave_container(param, &array_iter) != CborNoError){
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.apn, sizeof(provisioningNetworkConfig->networkSetting.catm1.apn))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.login, sizeof(provisioningNetworkConfig->networkSetting.catm1.login))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.pass, sizeof(provisioningNetworkConfig->networkSetting.catm1.pass))) {
    return Decoder::Status::Error;
  }

  provisioningNetworkConfig->networkSetting.catm1.rat = models::settingsDefault(NetworkAdapter::CATM1).catm1.rat;
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::CATM1;

  return Decoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_ETHERNET)
static inline bool getProvisioningIPStructFromMessage(CborValue*param, models::ip_addr* ipStruct) {
  uint8_t bytesString[17];
  size_t len = sizeof(bytesString);

  memset(bytesString, 0x00, sizeof(bytesString));

  if (!cbor_value_is_byte_string(param)) {
    return false;
  }

  if(_cbor_value_copy_string(param, bytesString, &len, NULL) != CborNoError) {
      return false;
  }

  if(len == 4){
    ipStruct->type = IPType::IPv4;
    memcpy(ipStruct->bytes, bytesString, len);
  } else if(len == 16){
    ipStruct->type = IPType::IPv6;
    memcpy(ipStruct->bytes, bytesString, len);
  } else if(len == 0) {
    //DHCP
    return true;
  }else {
    return false;
  }

  return true;
}

Decoder::Status EthernetConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message){
  NetworkConfigProvisioningMessage* provisioningNetworkConfig  = (NetworkConfigProvisioningMessage*) message;

  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  // Message is composed of 2 parameters: static ip, dns, default gateway, netmask, timeout and response timeout
  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.ip)) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.dns)) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.gateway)) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.netmask)) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  provisioningNetworkConfig->networkSetting.eth.timeout = models::settingsDefault(NetworkAdapter::ETHERNET).eth.timeout;
  if (cbor_value_is_integer(param)) {
    int val = 0;
    if (cbor_value_get_int(param, &val) == CborNoError) {
      //if the peer sends -1 we keep the default value. "-1" is used to keep the default value
      if(val >= 0) {
        provisioningNetworkConfig->networkSetting.eth.timeout = val;
      }
    }
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  provisioningNetworkConfig->networkSetting.eth.response_timeout = models::settingsDefault(NetworkAdapter::ETHERNET).eth.response_timeout;
  if (cbor_value_is_integer(param)) {
    int val = 0;
    if (cbor_value_get_int(param, &val) == CborNoError) {
      //if the peer sends -1 we keep the default value. "-1" is used to keep the default value
      if(val >= 0) {
        provisioningNetworkConfig->networkSetting.eth.response_timeout = val;
      }
    }
  }
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::ETHERNET;
  return Decoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_NB) || defined(BOARD_HAS_GSM) ||defined(BOARD_HAS_CELLULAR)
static inline Decoder::Status extractCellularFields(CborValue* param, models::CellularSetting* cellSetting) {

  // Message is composed of 4 parameters: pin, apn, login and password
  if (!copyCBORStringToArray(param, cellSetting->pin, sizeof(cellSetting->pin))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, cellSetting->apn, sizeof(cellSetting->apn))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, cellSetting->login, sizeof(cellSetting->login))) {
    return Decoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return Decoder::Status::Error;
  }

  if (!copyCBORStringToArray(param, cellSetting->pass, sizeof(cellSetting->pass))) {
    return Decoder::Status::Error;
  }

  return Decoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_CELLULAR)
Decoder::Status CellularConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message){
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::CELL;
  return extractCellularFields(param, &provisioningNetworkConfig->networkSetting.cell);
}
#endif

#if defined(BOARD_HAS_NB)
Decoder::Status NBIOTConfigProvisioningMessageDecoder::decode(CborValue* iter, Message* msg) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) msg;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::NB;
  return extractCellularFields(iter, &provisioningNetworkConfig->networkSetting.nb);
}
#endif

#if defined(BOARD_HAS_GSM)
Decoder::Status GSMConfigProvisioningMessageDecoder::decode(CborValue* iter, Message* msg) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) msg;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::GSM;
  return extractCellularFields(iter, &provisioningNetworkConfig->networkSetting.gsm);
}
#endif

static TimestampProvisioningMessageDecoder      timestampProvisioningMessageDecoder;
static CommandsProvisioningMessageDecoder       commandsProvisioningMessageDecoder;
#if defined(BOARD_HAS_WIFI)
static WifiConfigProvisioningMessageDecoder     wifiConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_LORA)
static LoRaConfigProvisioningMessageDecoder     loRaConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_CATM1_NBIOT)
static CATM1ConfigProvisioningMessageDecoder    cATM1ConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_ETHERNET)
static EthernetConfigProvisioningMessageDecoder ethernetConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_CELLULAR)
static CellularConfigProvisioningMessageDecoder cellularConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_NB)
static NBIOTConfigProvisioningMessageDecoder    nbiotConfigProvisioningMessageDecoder;
#endif
#if defined(BOARD_HAS_GSM)
static GSMConfigProvisioningMessageDecoder      gsmConfigProvisioningMessageDecoder;
#endif

