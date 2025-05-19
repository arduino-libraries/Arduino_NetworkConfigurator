/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "Decoder.h"
#include <connectionHandlerModels/settings_default.h>
#include <cbor/utils/decoder.h>
#if defined(BOARD_HAS_ETHERNET)
#include <IPAddress.h>
#endif

MessageDecoder::Status TimestampProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  TimestampProvisioningMessage* ts = (TimestampProvisioningMessage*) message;

  // Message is composed of a single parameter: a 64-bit unsigned integer
  if (cbor_value_is_integer(param)) {
    uint64_t val = 0;
    if (cbor_value_get_uint64(param, &val) == CborNoError) {
      ts->timestamp = val;
    }
  }

  return MessageDecoder::Status::Complete;
}
#if defined(BOARD_HAS_WIFI)
MessageDecoder::Status WifiConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  size_t ssidSize = sizeof(provisioningNetworkConfig->networkSetting.wifi.ssid);
  // Message is composed of 2 parameters: ssid and password
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.wifi.ssid, ssidSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t pwdSize = sizeof(provisioningNetworkConfig->networkSetting.wifi.pwd);
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.wifi.pwd, pwdSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::WIFI;
  return MessageDecoder::Status::Complete;
}
#endif

MessageDecoder::Status CommandsProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  CommandsProvisioningMessage* provisioningCommands = (CommandsProvisioningMessage*) message;

  // Message is composed of a single parameter: a 32-bit signed integer
  if (cbor_value_is_integer(param)) {
    int val = 0;
    if (cbor_value_get_int(param, &val) == CborNoError) {
      provisioningCommands->cmd = val;
    }
  }

  return MessageDecoder::Status::Complete;
}

#if defined(BOARD_HAS_LORA)
MessageDecoder::Status LoRaConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  size_t appeuiSize = sizeof(provisioningNetworkConfig->networkSetting.lora.appeui);
  // Message is composed of 5 parameters: app_eui, app_key, band, channel_mask, device_class
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.appeui, appeuiSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t appkeySize = sizeof(provisioningNetworkConfig->networkSetting.lora.appkey);
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.appkey, appkeySize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
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
    return MessageDecoder::Status::Error;
  }

  size_t channelMaskSize = sizeof(provisioningNetworkConfig->networkSetting.lora.channelMask);
  if(cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.lora.channelMask, channelMaskSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  char deviceClass[LORA_DEVICE_CLASS_SIZE];
  memset(deviceClass, 0x00, sizeof(deviceClass));

  size_t deviceClassSize = sizeof(deviceClass);
  if (cbor::utils::copyCBORStringToArray(param, deviceClass, deviceClassSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  if (deviceClass[0] == '\0') {
    provisioningNetworkConfig->networkSetting.lora.deviceClass = models::settingsDefault(NetworkAdapter::LORA).lora.deviceClass;
  }else {
    provisioningNetworkConfig->networkSetting.lora.deviceClass = (uint8_t)deviceClass[0];
  }
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::LORA;
  return MessageDecoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
MessageDecoder::Status CATM1ConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  CborValue array_iter;
  size_t arrayLength = 0;

  size_t pinSize = sizeof(provisioningNetworkConfig->networkSetting.catm1.pin);
  // Message is composed of 5 parameters: pin, band, apn, login and password
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.pin, pinSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  if (cbor_value_get_type(param) != CborArrayType) {
    return MessageDecoder::Status::Error;
  }

  cbor_value_get_array_length(param, &arrayLength);

  if(arrayLength > BAND_SIZE) {
    return MessageDecoder::Status::Error;
  }
  provisioningNetworkConfig->networkSetting.catm1.band = 0;
  if (arrayLength == 0) {
    provisioningNetworkConfig->networkSetting.catm1.band = models::settingsDefault(NetworkAdapter::CATM1).catm1.band;
  }

  if (cbor_value_enter_container(param, &array_iter) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  for(size_t i = 0; i < arrayLength; i++) {
    if (!cbor_value_is_unsigned_integer(&array_iter)) {
      return MessageDecoder::Status::Error;
    }

    int val = 0;
    if (cbor_value_get_int(&array_iter, &val) != CborNoError) {
      return MessageDecoder::Status::Error;
    }

    provisioningNetworkConfig->networkSetting.catm1.band |= val;

    if (cbor_value_advance(&array_iter) != CborNoError) {
      return MessageDecoder::Status::Error;
    }
  }

  if (cbor_value_leave_container(param, &array_iter) != CborNoError){
    return MessageDecoder::Status::Error;
  }

  size_t apnSize = sizeof(provisioningNetworkConfig->networkSetting.catm1.apn);
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.apn, apnSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t loginSize = sizeof(provisioningNetworkConfig->networkSetting.catm1.login);
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.login, loginSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t passSize = sizeof(provisioningNetworkConfig->networkSetting.catm1.pass);
  if (cbor::utils::copyCBORStringToArray(param, provisioningNetworkConfig->networkSetting.catm1.pass, passSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  provisioningNetworkConfig->networkSetting.catm1.rat = models::settingsDefault(NetworkAdapter::CATM1).catm1.rat;
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::CATM1;

  return MessageDecoder::Status::Complete;
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

MessageDecoder::Status EthernetConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message){
  NetworkConfigProvisioningMessage* provisioningNetworkConfig  = (NetworkConfigProvisioningMessage*) message;

  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  // Message is composed of 2 parameters: static ip, dns, default gateway, netmask, timeout and response timeout
  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.ip)) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.dns)) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.gateway)) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  if (!getProvisioningIPStructFromMessage(param, &provisioningNetworkConfig->networkSetting.eth.netmask)) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
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
    return MessageDecoder::Status::Error;
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
  return MessageDecoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_NB) || defined(BOARD_HAS_GSM) ||defined(BOARD_HAS_CELLULAR)
static inline MessageDecoder::Status extractCellularFields(CborValue* param, models::CellularSetting* cellSetting) {

   size_t pinSize = sizeof(cellSetting->pin);
  // Message is composed of 4 parameters: pin, apn, login and password
  if (cbor::utils::copyCBORStringToArray(param, cellSetting->pin, pinSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t apnSize = sizeof(cellSetting->apn);
  if (cbor::utils::copyCBORStringToArray(param, cellSetting->apn, apnSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t loginSize = sizeof(cellSetting->login);
  if (cbor::utils::copyCBORStringToArray(param, cellSetting->login, loginSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  // Next
  if (cbor_value_advance(param) != CborNoError) {
    return MessageDecoder::Status::Error;
  }

  size_t passSize = sizeof(cellSetting->pass);
  if (cbor::utils::copyCBORStringToArray(param, cellSetting->pass, passSize) == MessageDecoder::Status::Error) {
    return MessageDecoder::Status::Error;
  }

  return MessageDecoder::Status::Complete;
}
#endif

#if defined(BOARD_HAS_CELLULAR)
MessageDecoder::Status CellularConfigProvisioningMessageDecoder::decode(CborValue* param, Message* message){
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) message;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::CELL;
  return extractCellularFields(param, &provisioningNetworkConfig->networkSetting.cell);
}
#endif

#if defined(BOARD_HAS_NB)
MessageDecoder::Status NBIOTConfigProvisioningMessageDecoder::decode(CborValue* iter, Message* msg) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) msg;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::NB;
  return extractCellularFields(iter, &provisioningNetworkConfig->networkSetting.nb);
}
#endif

#if defined(BOARD_HAS_GSM)
MessageDecoder::Status GSMConfigProvisioningMessageDecoder::decode(CborValue* iter, Message* msg) {
  NetworkConfigProvisioningMessage* provisioningNetworkConfig = (NetworkConfigProvisioningMessage*) msg;
  memset(&provisioningNetworkConfig->networkSetting, 0x00, sizeof(models::NetworkSetting));
  provisioningNetworkConfig->networkSetting.type = NetworkAdapter::GSM;
  return extractCellularFields(iter, &provisioningNetworkConfig->networkSetting.gsm);
}
#endif

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
