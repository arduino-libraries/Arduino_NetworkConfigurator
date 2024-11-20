/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CBORAdapter.h"
#include "cbor/MessageEncoder.h"
#include "cbor/MessageDecoder.h"

bool CBORAdapter::uidToCBOR(String uid, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_UID_LEN || uid.length() > MAX_UID_SIZE) {
    return false;
  }

  memset(data, 0x00, *len);

  ProvisioningUniqueIdMessage uidMsg;
  uidMsg.c.id = CommandId::ProvisioningUniqueId;
  memcpy(uidMsg.params.uniqueId, uid.c_str(), uid.length());

  Encoder::Status status = encoder.encode((Message *)&uidMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}

bool CBORAdapter::signatureToCBOR(String signature, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_SIGNATURE_LEN || signature.length() > MAX_SIGNATURE_SIZE) {
    return false;
  }

  memset(data, 0x00, *len);

  ProvisioningSignatureMessage provisioningMsg;
  provisioningMsg.c.id = CommandId::ProvisioningSignature;
  memcpy(provisioningMsg.params.signature, signature.c_str(), signature.length());

  Encoder::Status status = encoder.encode((Message *)&provisioningMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}

bool CBORAdapter::statusToCBOR(StatusMessage msg, uint8_t *data, size_t *len) {
  bool result = false;

  switch (msg) {
    case MessageTypeCodes::NONE:
      break;
    default:
      result = adaptStatus(msg, data, len);
      break;
  }

  return result;
}

bool CBORAdapter::networkOptionsToCBOR(NetworkOptions &netOptions, uint8_t *data, size_t *len) {
  bool result = false;
  switch (netOptions.type) {
    case NetworkOptionsClass::WIFI:
      result = adaptWiFiOptions(&netOptions.option.wifi, data, len);
      break;
    default:
      WiFiOption wifiOptions;
      wifiOptions.numDiscoveredWiFiNetworks = 0;
      result = adaptWiFiOptions(&wifiOptions, data, len);//In case of WiFi scan is not available send an empty list of wifi options
      break;
  }
  return result;
}

bool CBORAdapter::getMsgFromCBOR(uint8_t *data, size_t len, ProvisioningCommandDown *msg) {
  CBORMessageDecoder decoder;
  Decoder::Status status = decoder.decode((Message *)msg, data, len);
  return status == Decoder::Status::Complete ? true : false;
}

bool CBORAdapter::extractTimestamp(ProvisioningCommandDown *msg, uint64_t *ts) {
  if (msg->c.id == CommandId::ProvisioningTimestamp) {
    *ts = msg->provisioningTimestamp.params.timestamp;
    return true;
  }
  return false;
}

bool CBORAdapter::extractNetworkSetting(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  bool result = false;
  netSetting->type = NetworkAdapter::NONE;
  switch (msg->c.id) {
#if defined(BOARD_HAS_WIFI)
    case CommandId::ProvisioningWifiConfig:
      result = extractWiFiSettings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_ETHERNET)
    case CommandId::ProvisioningEthernetConfig:
      result = extractEthernetSettings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_GSM)
    case CommandId::ProvisioningGSMConfig:
      result = extractGSMSettings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_LORA)
    case CommandId::ProvisioningLoRaConfig:
      result = extractLoRaSettings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_CELLULAR)
    case CommandId::ProvisioningCellularConfig:
      result = extractCellularSettings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
    case CommandId::ProvisioningCATM1Config:
      result = extractCATM1Settings(msg, netSetting);
      break;
#endif

#if defined(BOARD_HAS_NB)
    case CommandId::ProvisioningNBIOTConfig:
      result = extractNBIoTSettings(msg, netSetting);
      break;
#endif
    default:
      break;
  }

  return result;
}

bool CBORAdapter::extractCommand(ProvisioningCommandDown *msg, RemoteCommands *cmd) {
  if (msg->c.id == CommandId::ProvisioningCommands) {
    *cmd = (RemoteCommands)msg->provisioningCommands.params.cmd;
    return true;
  }
  return false;
}

bool CBORAdapter::adaptStatus(StatusMessage msg, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_STATUS_LEN) {
    return false;
  }

  ProvisioningStatusMessage statusMsg;
  statusMsg.c.id = CommandId::ProvisioningStatus;
  statusMsg.params.status = (int)msg;

  Encoder::Status status = encoder.encode((Message *)&statusMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}

bool CBORAdapter::adaptWiFiOptions(WiFiOption *wifiOptions, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  ProvisioningListWifiNetworksMessage wifiMsg;
  wifiMsg.c.id = CommandId::ProvisioningListWifiNetworks;
  wifiMsg.params.numDiscoveredWiFiNetworks = wifiOptions->numDiscoveredWiFiNetworks;
  for (uint8_t i = 0; i < wifiOptions->numDiscoveredWiFiNetworks; i++) {
    wifiMsg.params.discoveredWifiNetworks[i].SSID = wifiOptions->discoveredWifiNetworks[i].SSID;
    wifiMsg.params.discoveredWifiNetworks[i].RSSI = &wifiOptions->discoveredWifiNetworks[i].RSSI;
  }

  Encoder::Status status = encoder.encode((Message *)&wifiMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}

#if defined(BOARD_HAS_WIFI)
bool CBORAdapter::extractWiFiSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::WIFI;
  memcpy(netSetting->wifi.ssid, msg->provisioningWifiConfig.params.ssid, sizeof(msg->provisioningWifiConfig.params.ssid));
  memcpy(netSetting->wifi.pwd, msg->provisioningWifiConfig.params.pwd, sizeof(msg->provisioningWifiConfig.params.pwd));
  return true;
}
#endif

#if defined(BOARD_HAS_ETHERNET)
bool CBORAdapter::extractEthernetSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::ETHERNET;
  extractIpSettings(&msg->provisioningEthernetConfig.params.ip, &netSetting->eth.ip);
  extractIpSettings(&msg->provisioningEthernetConfig.params.dns, &netSetting->eth.dns);
  extractIpSettings(&msg->provisioningEthernetConfig.params.gateway, &netSetting->eth.gateway);
  extractIpSettings(&msg->provisioningEthernetConfig.params.netmask, &netSetting->eth.netmask);
  netSetting->eth.timeout = msg->provisioningEthernetConfig.params.timeout;
  netSetting->eth.response_timeout = msg->provisioningEthernetConfig.params.response_timeout;
  return true;
}

bool CBORAdapter::extractIpSettings(ProvisioningIPStruct *ipMsg, models::ip_addr *ipSetting) {
  uint8_t len = 0;
  if (ipMsg->type == ProvisioningIPStruct::IPType::IPV4) {
    len = 4;
    ipSetting->type = IPType::IPv4;
  } else if (ipMsg->type == ProvisioningIPStruct::IPType::IPV6) {
    len = 16;
    ipSetting->type = IPType::IPv6;
  }
  memcpy(ipSetting->bytes, ipMsg->ip, len);
  return true;
}
#endif

#if defined(BOARD_HAS_GSM)
bool CBORAdapter::extractGSMSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::GSM;

  return extractCellularFields(&msg->provisioningCellularConfig, &netSetting->gsm);
}
#endif

#if defined(BOARD_HAS_LORA)
bool CBORAdapter::extractLoRaSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::LORA;

  memcpy(netSetting->lora.appeui, msg->provisioningLoRaConfig.params.appeui, sizeof(msg->provisioningLoRaConfig.params.appeui));
  memcpy(netSetting->lora.appkey, msg->provisioningLoRaConfig.params.appkey, sizeof(msg->provisioningLoRaConfig.params.appkey));
  netSetting->lora.band = msg->provisioningLoRaConfig.params.band;
  memcpy(netSetting->lora.channelMask, msg->provisioningLoRaConfig.params.channelMask, sizeof(msg->provisioningLoRaConfig.params.channelMask));
  netSetting->lora.deviceClass = (uint8_t)msg->provisioningLoRaConfig.params.deviceClass[0];

  return true;
}
#endif

#if defined(BOARD_HAS_CELLULAR)
bool CBORAdapter::extractCellularSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::CELL;

  return extractCellularFields(&msg->provisioningCellularConfig, &netSetting->cell);
}
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
bool CBORAdapter::extractCATM1Settings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::CATM1;

  memcpy(netSetting->catm1.pin, msg->provisioningCATM1Config.params.pin, sizeof(msg->provisioningCATM1Config.params.pin));
  memcpy(netSetting->catm1.apn, msg->provisioningCATM1Config.params.apn, sizeof(msg->provisioningCATM1Config.params.apn));
  memcpy(netSetting->catm1.login, msg->provisioningCATM1Config.params.login, sizeof(msg->provisioningCATM1Config.params.login));
  memcpy(netSetting->catm1.pass, msg->provisioningCATM1Config.params.pass, sizeof(msg->provisioningCATM1Config.params.pass));

  netSetting->catm1.band = 0;

  for (uint8_t i = 0; i < 4; i++) {
    if (msg->provisioningCATM1Config.params.band[i] != 0) {
      netSetting->catm1.band |= msg->provisioningCATM1Config.params.band[i];
    }
  }

  return true;
}
#endif

#if defined(BOARD_HAS_NB)
bool CBORAdapter::extractNBIoTSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting) {
  netSetting->type = NetworkAdapter::NB;

  return extractCellularFields(&msg->provisioningCellularConfig, &netSetting->nb);
}
#endif

#if defined(BOARD_HAS_CELLULAR) || defined(BOARD_HAS_GSM) || defined(BOARD_HAS_NB)
bool CBORAdapter::extractCellularFields(ProvisioningCellularConfigMessage *msg, models::CellularSetting *cellSetting) {
  memcpy(cellSetting->pin, msg->params.pin, sizeof(msg->params.pin));
  memcpy(cellSetting->apn, msg->params.apn, sizeof(msg->params.apn));
  memcpy(cellSetting->login, msg->params.login, sizeof(msg->params.login));
  memcpy(cellSetting->pass, msg->params.pass, sizeof(msg->params.pass));
  return true;
}
#endif
