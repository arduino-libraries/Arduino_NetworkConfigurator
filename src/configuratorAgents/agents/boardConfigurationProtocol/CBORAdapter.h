/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "configuratorAgents/MessagesDefinitions.h"
#include <connectionHandlerModels/settings.h>
#include "cbor/CBOR.h"
#include "./cbor/CBOR.h"

#define CBOR_DATA_HEADER_LEN 6
#define CBOR_DATA_UHWID_LEN MAX_UHWID_SIZE + 2 + CBOR_DATA_HEADER_LEN  //UHWID size + 2 bytes for CBOR array of bytes identifier + CBOR header size
#define CBOR_DATA_JWT_LEN MAX_JWT_SIZE + 3 + CBOR_DATA_HEADER_LEN      //Signature size + 2 bytes for CBOR array of bytes identifier + CBOR header size
#define CBOR_DATA_STATUS_LEN 4 + CBOR_DATA_HEADER_LEN
#define CBOR_DATA_BLE_MAC_LEN BLE_MAC_ADDRESS_SIZE + 2 + CBOR_DATA_HEADER_LEN
#define CBOR_MIN_WIFI_FW_VERSION_LEN CBOR_DATA_HEADER_LEN + 1 // CBOR_DATA_HEADER_LEN + 1 byte for the length of the string
#define CBOR_MIN_PROV_SKETCH_VERSION_LEN CBOR_DATA_HEADER_LEN + 1 // CBOR_DATA_HEADER_LEN + 1 byte for the length of the string
#define CBOR_MIN_NETCONFIG_LIB_VERSION_LEN CBOR_DATA_HEADER_LEN + 1 // CBOR_DATA_HEADER_LEN + 1 byte for the length of the string
#define CBOR_MIN_PROV_PUBIC_KEY_LEN CBOR_DATA_HEADER_LEN + 3 // CBOR_DATA_HEADER_LEN + 2 bytes for the length of the string + 1 byte for the type of the string

class CBORAdapter {
public:
  static bool uhwidToCBOR(const byte *uhwid, uint8_t *data, size_t *len);
  static bool jwtToCBOR(const char *jwt, uint8_t *data, size_t *len);
  static bool provPublicKeyToCBOR(const char *provPublicKey, uint8_t *data, size_t *len);
  static bool BLEMacAddressToCBOR(const uint8_t *mac, uint8_t *data, size_t *len);
  static bool wifiFWVersionToCBOR(const char *wifiFWVersion, uint8_t *data, size_t *len);
  static bool provSketchVersionToCBOR(const char *provSketchVersion, uint8_t *data, size_t *len);
  static bool netConfigLibVersionToCBOR(const char *netConfigLibVersion, uint8_t *data, size_t *len);
  static bool statusToCBOR(StatusMessage msg, uint8_t *data, size_t *len);
  static bool networkOptionsToCBOR(const NetworkOptions *netOptions, uint8_t *data, size_t *len);
  static bool getMsgFromCBOR(const uint8_t *data, size_t len, ProvisioningMessageDown *msg);
private:
  CBORAdapter();
  static bool adaptStatus(StatusMessage msg, uint8_t *data, size_t *len);
  static bool adaptWiFiOptions(const WiFiOption *wifiOptions, uint8_t *data, size_t *len);
};
