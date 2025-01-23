/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "ConfiguratorAgents/AgentsConfiguratorManager.h"
#include <Arduino_SecureElement.h>

typedef bool (*ClearStoredCredentialsHandler)(); 
class ClaimingHandlerClass {
public:
  ClaimingHandlerClass(AgentsConfiguratorManager &agc);
  bool begin(SecureElement *secureElement, String *uhwid, ClearStoredCredentialsHandler clearStoredCredentials);
  void end();
  void poll();
private:
  String *_uhwid;
  enum class ClaimingHandlerStates {
    INIT,
    END
  };
  enum class ClaimingReqEvents { NONE,
                                 GET_ID,
                                 RESET };
  static inline ClaimingReqEvents _receivedEvent = ClaimingReqEvents::NONE;
  ClaimingHandlerStates _state = ClaimingHandlerStates::END;
  AgentsConfiguratorManager *_agentManager = nullptr;
  static inline uint64_t _ts = 0;
  SecureElement *_secureElement;
  ClearStoredCredentialsHandler _clearStoredCredentials = nullptr;
  void getIdReqHandler();
  void resetStoredCredReqHandler();
  static void getIdRequestCb();
  static void setTimestamp(uint64_t ts);
  static void resetStoredCredRequestCb();
};