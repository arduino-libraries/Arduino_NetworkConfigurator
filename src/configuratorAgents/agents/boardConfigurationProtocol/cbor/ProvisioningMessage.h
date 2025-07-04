/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Arduino_CBOR.h>
#include <cbor/standards/StandardMessages.h>
#include <ConnectionHandlerDefinitions.h>
#include <connectionHandlerModels/settings.h>
#include <configuratorAgents/NetworkOptionsDefinitions.h>

#define UHWID_SIZE                  32
#define PROVISIONING_JWT_SIZE      269 // Max length of jwt is 268 + \0
#define BLE_MAC_ADDRESS_SIZE         6
#define WIFI_SSID_SIZE              33 // Max length of ssid is 32 + \0
#define WIFI_PWD_SIZE               64 // Max length of password is 63 + \0
#define LORA_APPEUI_SIZE            17 // appeui is 8 octets * 2 (hex format) + \0
#define LORA_APPKEY_SIZE            33 // appeui is 16 octets * 2 (hex format) + \0
#define LORA_CHANNEL_MASK_SIZE      13
#define LORA_DEVICE_CLASS_SIZE       2 // 1 char + \0
#define PIN_SIZE                     9 // 8 digits + \0
#define APN_SIZE                   101 // Max length of apn is 100 + \0
#define LOGIN_SIZE                  65 // Max length of login is 64 + \0
#define PASS_SIZE                   65 // Max length of password is 64 + \0
#define BAND_SIZE                    4
#define MAX_WIFI_NETWORKS           20
#define MAX_IP_SIZE                 16

enum CBORProvisioningMessageTag: CBORTag {
  CBORTimestampProvisioningMessage          = 0x012002,
  CBORCommandsProvisioningMessage           = 0x012003,
  CBORWifiConfigProvisioningMessage         = 0x012004,
  CBORLoRaConfigProvisioningMessage         = 0x012005,
  CBORGSMConfigProvisioningMessage          = 0x012006,
  CBORNBIOTConfigProvisioningMessage        = 0x012007,
  CBORCATM1ConfigProvisioningMessage        = 0x012008,
  CBOREthernetConfigProvisioningMessage     = 0x012009,
  CBORCellularConfigProvisioningMessage     = 0x012012,

  CBORStatusProvisioningMessage             = 0x012000,
  CBORListWifiNetworksProvisioningMessage   = 0x012001,
  CBORUniqueHardwareIdProvisioningMessage   = 0x012010,
  CBORJWTProvisioningMessage                = 0x012011,
  CBORBLEMacAddressProvisioningMessage      = 0x012013,
  CBORProvSketchVersionProvisioningMessage  = 0x012015,
  CBORNetConfigLibVersProvisioningMessage   = 0x012016,
  CBORProvPublicKeyProvisioningMessage      = 0x012017,
};

enum ProvisioningMessageId: MessageId {
  /* Provisioning commands*/
  StatusProvisioningMessageId = ArduinoProvisioningStartMessageId,
  ListWifiNetworksProvisioningMessageId,
  UniqueHardwareIdProvisioningMessageId,
  BLEMacAddressProvisioningMessageId,
  ProvSketchVersionProvisioningMessageId,
  NetConfigLibVersProvisioningMessageId,
  JWTProvisioningMessageId,
  ProvPublicKeyProvisioningMessageId,
  TimestampProvisioningMessageId,
  CommandsProvisioningMessageId,
  WifiConfigProvisioningMessageId,
  LoRaConfigProvisioningMessageId,
  GSMConfigProvisioningMessageId,
  NBIOTConfigProvisioningMessageId,
  CATM1ConfigProvisioningMessageId,
  EthernetConfigProvisioningMessageId,
  CellularConfigProvisioningMessageId,
};

typedef Message ProvisioningMessage;

struct StatusProvisioningMessage {
  ProvisioningMessage c;
  struct {
    int16_t status;
  };
};

struct WiFiNetwork {
  char *SSID;
  int *RSSI;
};

struct ListWifiNetworksProvisioningMessage {
  ProvisioningMessage c;
  struct {
    WiFiNetwork discoveredWifiNetworks[MAX_WIFI_NETWORKS];
    uint8_t numDiscoveredWiFiNetworks = 0;
  };
};

struct UniqueHardwareIdProvisioningMessage {
  ProvisioningMessage c;
  struct {
    char uniqueHardwareId[UHWID_SIZE]; //The payload is an array of char with a maximum length of 32, not null terminated. It's not a string.
  };
};

struct JWTProvisioningMessage {
  ProvisioningMessage c;
  struct {
    char jwt[PROVISIONING_JWT_SIZE]; //The payload is a string with maximum dimension of 268 characters + '\0'.
  };
};

struct ProvPublicKeyProvisioningMessage {
  ProvisioningMessage c;
  struct {
    const char *provPublicKey; //The payload is a string.
  };
};

struct BLEMacAddressProvisioningMessage {
  ProvisioningMessage c;
  struct {
    uint8_t macAddress[BLE_MAC_ADDRESS_SIZE];
  };
};

struct ProvSketchVersionProvisioningMessage {
  ProvisioningMessage c;
  struct {
    const char *provSketchVersion; //The payload is a string.
  };
};

struct NetConfigLibVersionProvisioningMessage {
  ProvisioningMessage c;
  struct {
    const char *netConfigLibVersion; //The payload is a string.
  };
};

struct TimestampProvisioningMessage {
  ProvisioningMessage c;
  struct {
    uint64_t timestamp;
  };
};

struct CommandsProvisioningMessage {
  ProvisioningMessage c;
  struct {
    uint8_t cmd;
  };
};

struct NetworkConfigProvisioningMessage {
  ProvisioningMessage c;
  models::NetworkSetting networkSetting;
};

union ProvisioningMessageDown {
  ProvisioningMessage                       c;
  struct TimestampProvisioningMessage       provisioningTimestamp;
  struct CommandsProvisioningMessage        provisioningCommands;
  struct NetworkConfigProvisioningMessage   provisioningNetworkConfig;
};
