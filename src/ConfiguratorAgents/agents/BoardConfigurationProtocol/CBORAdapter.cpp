/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CBORAdapter.h"
#include "cbor/MessageEncoder.h"
#include "cbor/MessageDecoder.h"
#include <settings/settings_default.h>

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

  Encoder::Status status = encoder.encode((Message *)&uhwidMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}

bool CBORAdapter::jwtToCBOR(const char *jwt, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_JWT_LEN || strlen(jwt) > MAX_JWT_SIZE) {
    Serial.println("CBORAdapter jwtToCBOR: Invalid jwt size");
    return false;
  }

  memset(data, 0x00, *len);

  JWTProvisioningMessage provisioningMsg;
  provisioningMsg.c.id = ProvisioningMessageId::JWTProvisioningMessageId;
  memset(provisioningMsg.jwt, 0x00, MAX_JWT_SIZE);
  memcpy(provisioningMsg.jwt, jwt, strlen(jwt));

  Encoder::Status status = encoder.encode((Message *)&provisioningMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
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

  Encoder::Status status = encoder.encode((Message *)&bleMacMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
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
  Decoder::Status status = decoder.decode((Message *)msg, data, len);
  return status == Decoder::Status::Complete ? true : false;
}

bool CBORAdapter::adaptStatus(StatusMessage msg, uint8_t *data, size_t *len) {
  CBORMessageEncoder encoder;

  if (*len < CBOR_DATA_STATUS_LEN) {
    return false;
  }

  StatusProvisioningMessage statusMsg;
  statusMsg.c.id = ProvisioningMessageId::StatusProvisioningMessageId;
  statusMsg.status = (int)msg;

  Encoder::Status status = encoder.encode((Message *)&statusMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
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

  Encoder::Status status = encoder.encode((Message *)&wifiMsg, data, *len);

  return status == Encoder::Status::Complete ? true : false;
}
