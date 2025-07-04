/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "NetworkOptionsDefinitions.h"
#include <connectionHandlerModels/settings.h>

#define MAX_UHWID_SIZE 32
#define MAX_JWT_SIZE  269

/* Status codes */
enum class StatusMessage {
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
  INTERNET_NOT_AVAILABLE     = -8,
  SCAN_DISABLED_CONNECTING   = -100,
  HW_ERROR_CONN_MODULE       = -101,
  HW_CONN_MODULE_STOPPED     = -102,
  HW_ERROR_SE_BEGIN          = -150,
  HW_ERROR_SE_CONFIG         = -151,
  HW_ERROR_SE_LOCK           = -152,
  ERROR_GENERATING_UHWID     = -160,
  ERROR_STORAGE_BEGIN        = -200,
  FAIL_TO_PARTITION_STORAGE  = -201,
  ERROR                      = -255
};

/* Commands codes */
enum class RemoteCommands { CONNECT                         = 1,
                            GET_ID                          = 2,
                            GET_BLE_MAC_ADDRESS             = 3,
                            RESET                           = 4,
                            SCAN                            = 100,
                            GET_WIFI_FW_VERSION             = 101,
                            GET_PROVISIONING_SKETCH_VERSION = 200,
                            GET_NETCONFIG_LIB_VERSION       = 201,
};

/* Types of outgoing messages */
enum class MessageOutputType { STATUS,
                               NETWORK_OPTIONS,
                               UHWID,
                               JWT,
                               BLE_MAC_ADDRESS,
                               WIFI_FW_VERSION,
                               PROV_SKETCH_VERSION,
                               NETCONFIG_LIB_VERSION,
                               PROV_PUBLIC_KEY
};

/* Types of ingoing messages */
enum class MessageInputType {
  COMMANDS,
  NETWORK_SETTINGS,
  TIMESTAMP
};

/* Message structure for outgoing messages
 * The payload is a union of different types
 * pointers, no copy of object is required no
 * memory is allocated
 */
struct ProvisioningOutputMessage {
  MessageOutputType type;
  union {
    StatusMessage status;
    const NetworkOptions *netOptions;
    const byte *uhwid; // Must be a pointer to a byte array of MAX_UHWID_SIZE
    const char *jwt;
    const uint8_t *BLEMacAddress;
    const char *wifiFwVersion;
    const char *provSketchVersion;
    const char *netConfigLibVersion;
    const char *provPublicKey;
  } m;
};

/*
 * Message structure for ingoing messages
 */
struct ProvisioningInputMessage {
  MessageInputType type;
  union {
    RemoteCommands cmd;
    models::NetworkSetting netSetting;
    uint64_t timestamp;
  } m;
};
