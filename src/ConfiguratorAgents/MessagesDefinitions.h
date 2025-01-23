/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "NetworkOptionsDefinitions.h"
#include <settings/settings.h>

#define MAX_UHWID_SIZE 32
#define MAX_JWT_SIZE  268

enum class MessageTypeCodes {
  NONE                       = 0,
  CONNECTING                 = 1,
  CONNECTED                  = 2,
  RESET_COMPLETED            = 4,
  SCANNING                   = 100,
  FAILED_TO_CONNECT          = -1,
  CONNECTION_LOST            = -2,
  DISCONNECTED               = -3,
  PARAMS_NOT_FOUND           = -4,
  INVALID_PARAMS             = -5,
  OTHER_REQUEST_IN_EXECUTION = -6,
  INVALID_REQUEST            = -7,
  SCAN_DISABLED_CONNECTING   = -100,
  HW_ERROR_CONN_MODULE       = -101,
  WIFI_IDLE                  = -102,
  WIFI_STOPPED               = -103,
  ERROR_STORAGE_BEGIN        = -200, 
  ERROR                      = -255
};

typedef MessageTypeCodes StatusMessage;

enum class RemoteCommands { CONNECT             = 1,
                            GET_ID              = 2,
                            GET_BLE_MAC_ADDRESS = 3,
                            RESET               = 4,
                            SCAN                = 100 
};

enum class MessageOutputType { STATUS,
                               NETWORK_OPTIONS,
                               UHWID,
                               JWT,
                               BLE_MAC_ADDRESS
};

enum class MessageInputType {
  COMMANDS,
  NETWORK_SETTINGS,
  TIMESTAMP
};

struct ProvisioningOutputMessage {
  MessageOutputType type;
  union {
    StatusMessage status;
    const NetworkOptions *netOptions;
    const char *uhwid;
    const char *jwt;
    const uint8_t *BLEMacAddress;
  } m;
};

struct ProvisioningInputMessage {
  MessageInputType type;
  union {
    RemoteCommands cmd;
    models::NetworkSetting netSetting;
    uint64_t timestamp;
  } m;
};
