/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include "agents/ConfiguratorAgent.h"
#include "MessagesDefinitions.h"

enum class AgentsManagerStates { INIT,
                                             SEND_INITIAL_STATUS,
                                             SEND_NETWORK_OPTIONS,
                                             CONFIG_IN_PROGRESS,
                                             END };

typedef void (*ConfiguratorRequestHandler)();
typedef void (*ReturnTimestamp)(uint64_t ts);
typedef void (*ReturnNetworkSettings)(models::NetworkSetting *netSetting);

enum class RequestType { NONE,
                         CONNECT,
                         SCAN,
                         GET_ID,
                         RESET,
                         GET_WIFI_FW_VERSION}; //TODO fix when rebasing

class AgentsManagerClass {
public:
  AgentsManagerClass()
    : _netOptions{ .type = NetworkOptionsClass::NONE } {};
  bool begin(uint8_t id);
  bool end(uint8_t id);
  void disconnect();
  AgentsManagerStates poll();
  void enableBLEAgent(bool enable);
  bool isBLEAgentEnabled() {
    return _bleAgentEnabled;
  };

  bool sendMsg(ProvisioningOutputMessage &msg);
  bool addAgent(ConfiguratorAgent &agent);
  bool addRequestHandler(RequestType type, ConfiguratorRequestHandler callback);
  void removeRequestHandler(RequestType type) {
    _reqHandlers[(int)type - 1] = nullptr;
  };
  bool addReturnTimestampCallback(ReturnTimestamp callback);
  void removeReturnTimestampCallback() {
    _returnTimestampCb = nullptr;
  };
  bool addReturnNetworkSettingsCallback(ReturnNetworkSettings callback);
  void removeReturnNetworkSettingsCallback() {
    _returnNetworkSettingsCb = nullptr;
  };
  inline bool isConfigInProgress() {
    return _state != AgentsManagerStates::INIT && _state != AgentsManagerStates::END;
  };
private:
  AgentsManagerStates _state = AgentsManagerStates::END;
  std::list<ConfiguratorAgent *> _agentsList;
  std::list<uint8_t> _servicesList;
  ConfiguratorRequestHandler _reqHandlers[5]; //TODO fix when rebasing
  ReturnTimestamp _returnTimestampCb = nullptr;
  ReturnNetworkSettings _returnNetworkSettingsCb = nullptr;
  ConfiguratorAgent *_selectedAgent = nullptr;
  uint8_t _instances = 0;
  bool _bleAgentEnabled = true;
  StatusMessage _initStatusMsg = StatusMessage::NONE;
  NetworkOptions _netOptions;
  typedef struct {
    void reset() {
      pending = false;
      key = RequestType::NONE;
      completion = 0;
    };
    uint8_t completion;
    bool pending;
    RequestType key;
  } StatusRequest;

  StatusRequest _statusRequest = { .completion = 0 , .pending = false, .key = RequestType::NONE};

  AgentsManagerStates handleInit();
  AgentsManagerStates handleSendInitialStatus();
  AgentsManagerStates handleSendNetworkOptions();
  AgentsManagerStates handleConfInProgress();
  void handleReceivedCommands(RemoteCommands cmd);
  void handleReceivedData();
  void handleConnectCommand();
  void handleUpdateOptCommand();
  void handleGetIDCommand();
  void handleGetBleMacAddressCommand();
  void handleResetCommand();
  void handleGetWiFiFWVersionCommand();
  bool sendStatus(StatusMessage msg);

  AgentsManagerStates handlePeerDisconnected();
  void callHandler(RequestType key);
  void stopBLEAgent();
  void startBLEAgent();
};

extern AgentsManagerClass AgentsManager;
