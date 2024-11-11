/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "MessagesDefinitions.h"
#include "NetworkOptionsDefinitions.h"
#include "settings/settings.h"
#include "cbor/CBOR.h"

#define CBOR_DATA_HEADER_LEN 6
#define CBOR_DATA_UID_LEN 34 + CBOR_DATA_HEADER_LEN
#define CBOR_DATA_SIGNATURE_LEN 34 + CBOR_DATA_HEADER_LEN
#define CBOR_DATA_STATUS_LEN 4 + CBOR_DATA_HEADER_LEN
#define MAX_UID_SIZE 32
#define MAX_SIGNATURE_SIZE 32

class CBORAdapter {
public:
  static bool uidToCBOR(String uid, uint8_t *data, size_t *len);
  static bool signatureToCBOR(String signature, uint8_t *data, size_t *len);
  static bool statusToCBOR(StatusMessage msg, uint8_t *data, size_t *len);
  static bool networkOptionsToCBOR(NetworkOptions &netOptions, uint8_t *data, size_t *len);
  static bool getMsgFromCBOR(uint8_t *data, size_t len, ProvisioningCommandDown *msg);
  static bool extractTimestamp(ProvisioningCommandDown *msg, uint64_t *ts);
  static bool extractNetworkSetting(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
  static bool extractCommand(ProvisioningCommandDown *msg, RemoteCommands *cmd);

private:
  CBORAdapter();
  static bool adaptStatus(StatusMessage msg, uint8_t *data, size_t *len);
  static bool adaptWiFiOptions(WiFiOption *wifiOptions, uint8_t *data, size_t *len);
#if defined(BOARD_HAS_WIFI)
  static bool extractWiFiSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_ETHERNET)
  static bool extractEthernetSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
  static bool extractIpSettings(ProvisioningIPStruct *ipMsg, models::ip_addr *ipSetting);
#endif
#if defined(BOARD_HAS_GSM)
  static bool extractGSMSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_LORA)
  static bool extractLoRaSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_CELLULAR)
  static bool extractCellularSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_CATM1_NBIOT)
  static bool extractCATM1Settings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_NB)
  static bool extractNBIoTSettings(ProvisioningCommandDown *msg, models::NetworkSetting *netSetting);
#endif
#if defined(BOARD_HAS_CELLULAR) || defined(BOARD_HAS_GSM) || defined(BOARD_HAS_NB)
  static bool extractCellularFields(ProvisioningCellularConfigMessage *msg, models::CellularSetting *cellSetting);
#endif
};