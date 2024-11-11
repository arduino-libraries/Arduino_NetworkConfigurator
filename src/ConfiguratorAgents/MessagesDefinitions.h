/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"

enum class MessageTypeCodes {
  NONE                       = 0,
  CONNECTING                 = 1,
  CONNECTED                  = 2,
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
  ERROR                      = -255
};

typedef MessageTypeCodes StatusMessage;

enum class RemoteCommands { CONNECT = 1,
                            GET_ID  = 2,
                            SCAN    = 100 };