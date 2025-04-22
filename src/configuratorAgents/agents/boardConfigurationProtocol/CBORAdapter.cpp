/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "CBORAdapter.h"
#include "cbor/MessageEncoder.h"
#include "cbor/MessageDecoder.h"

bool CBORAdapter::uhwidToCBOR(const byte *uhwid, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_UHWID_LEN) {
    return false;
  }

  memset(data, 0x00, *len);

  UniqueHardwareIdProvisioningMessage uhwidMsg;
  uhwidMsg.c.id = ProvisioningMessageId::UniqueHardwareIdProvisioningMessageId;
  memset(uhwidMsg.uniqueHardwareId, 0x00, MAX_UHWID_SIZE);
  //Since some bytes of UHWID could be 00 is not possible use strlen to copy the UHWID
  memcpy(uhwidMsg.uniqueHardwareId, uhwid, MAX_UHWID_SIZE);

  MessageEncoder::Status status = encoder.encode((Message *)&uhwidMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::jwtToCBOR(const char *jwt, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_JWT_LEN || strlen(jwt) > MAX_JWT_SIZE) {
    return false;
  }

  memset(data, 0x00, *len);

  JWTProvisioningMessage provisioningMsg;
  provisioningMsg.c.id = ProvisioningMessageId::JWTProvisioningMessageId;
  memset(provisioningMsg.jwt, 0x00, MAX_JWT_SIZE);
  memcpy(provisioningMsg.jwt, jwt, strlen(jwt));

  MessageEncoder::Status status = encoder.encode((Message *)&provisioningMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::BLEMacAddressToCBOR(const uint8_t *mac, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;
  if (*len < CBOR_DATA_BLE_MAC_LEN) {
    return false;
  }

  memset(data, 0x00, *len);

  BLEMacAddressProvisioningMessage bleMacMsg;
  bleMacMsg.c.id = ProvisioningMessageId::BLEMacAddressProvisioningMessageId;
  memcpy(bleMacMsg.macAddress, mac, BLE_MAC_ADDRESS_SIZE);

  MessageEncoder::Status status = encoder.encode((Message *)&bleMacMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::provPublicKeyToCBOR(const char *provPublicKey, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;
  if(*len < CBOR_MIN_PROV_PUBIC_KEY_LEN + strlen(provPublicKey)) {
    return false;
  }
  ProvPublicKeyProvisioningMessage provPublicKeyMsg;
  provPublicKeyMsg.c.id = ProvisioningMessageId::ProvPublicKeyProvisioningMessageId;
  provPublicKeyMsg.provPublicKey = provPublicKey;

  MessageEncoder::Status status = encoder.encode((Message *)&provPublicKeyMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::statusToCBOR(StatusMessage msg, uint8_t *data, size_t *len) {
  bool result = false;

  switch (msg) {
    case StatusMessage::NONE:
      break;
    default:
      result = adaptStatus(msg, data, len);
      break;
  }

  return result;
}

bool CBORAdapter::wifiFWVersionToCBOR(const char *wifiFWVersion, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;
  if(*len < CBOR_MIN_WIFI_FW_VERSION_LEN + strlen(wifiFWVersion)) {
    return false;
  }
  VersionMessage wifiFWVersionMsg;
  wifiFWVersionMsg.c.id = StandardMessageId::WiFiFWVersionMessageId;
  wifiFWVersionMsg.params.version = wifiFWVersion;

  MessageEncoder::Status status = encoder.encode((Message *)&wifiFWVersionMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::provSketchVersionToCBOR(const char *provSketchVersion, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;
  if(*len < CBOR_MIN_PROV_SKETCH_VERSION_LEN + strlen(provSketchVersion)) {
    return false;
  }
  ProvSketchVersionProvisioningMessage provSketchVersionMsg;
  provSketchVersionMsg.c.id = ProvisioningMessageId::ProvSketchVersionProvisioningMessageId;
  provSketchVersionMsg.provSketchVersion = provSketchVersion;

  MessageEncoder::Status status = encoder.encode((Message *)&provSketchVersionMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::netConfigLibVersionToCBOR(const char *netConfigLibVersion, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;
  if(*len < CBOR_MIN_NETCONFIG_LIB_VERSION_LEN + strlen(netConfigLibVersion)) {
    return false;
  }
  NetConfigLibVersionProvisioningMessage netConfigLibVersionMsg;
  netConfigLibVersionMsg.c.id = ProvisioningMessageId::NetConfigLibVersProvisioningMessageId;
  netConfigLibVersionMsg.netConfigLibVersion = netConfigLibVersion;

  MessageEncoder::Status status = encoder.encode((Message *)&netConfigLibVersionMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::networkOptionsToCBOR(const NetworkOptions *netOptions, uint8_t *data, size_t *len) {
  bool result = false;
  switch (netOptions->type) {
    case NetworkOptionsClass::WIFI:
      result = adaptWiFiOptions(&(netOptions->option.wifi), data, len);
      break;
    default:
      WiFiOption wifiOptions;
      wifiOptions.numDiscoveredWiFiNetworks = 0;
      result = adaptWiFiOptions(&wifiOptions, data, len);  //In case of WiFi scan is not available send an empty list of wifi options
      break;
  }
  return result;
}

bool CBORAdapter::getMsgFromCBOR(const uint8_t *data, size_t len, ProvisioningMessageDown *msg) {
  CBORMessageDecoder decoder;
  MessageDecoder::Status status = decoder.decode((Message *)msg, data, len);
  return status == MessageDecoder::Status::Complete ? true : false;
}

bool CBORAdapter::adaptStatus(StatusMessage msg, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_STATUS_LEN) {
    return false;
  }

  StatusProvisioningMessage statusMsg;
  statusMsg.c.id = ProvisioningMessageId::StatusProvisioningMessageId;
  statusMsg.status = (int)msg;

  MessageEncoder::Status status = encoder.encode((Message *)&statusMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

bool CBORAdapter::adaptWiFiOptions(const WiFiOption *wifiOptions, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  ListWifiNetworksProvisioningMessage wifiMsg;
  wifiMsg.c.id = ProvisioningMessageId::ListWifiNetworksProvisioningMessageId;
  wifiMsg.numDiscoveredWiFiNetworks = wifiOptions->numDiscoveredWiFiNetworks;
  for (uint8_t i = 0; i < wifiOptions->numDiscoveredWiFiNetworks; i++) {
    wifiMsg.discoveredWifiNetworks[i].SSID = wifiOptions->discoveredWifiNetworks[i].SSID;
    wifiMsg.discoveredWifiNetworks[i].RSSI = const_cast<int *>(&wifiOptions->discoveredWifiNetworks[i].RSSI);
  }

  MessageEncoder::Status status = encoder.encode((Message *)&wifiMsg, data, *len);

  return status == MessageEncoder::Status::Complete ? true : false;
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
