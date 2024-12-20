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

enum class AgentsConfiguratorManagerStates { INIT,
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
                         GET_ID };

class AgentsConfiguratorManager {
public:
  AgentsConfiguratorManager()
    : _netOptions{ .type = NetworkOptionsClass::NONE } {};
  bool begin(uint8_t id);
  bool end(uint8_t id);
  void disconnect();
  AgentsConfiguratorManagerStates poll();
  void enableBLEAgent(bool enable);
  bool isBLEAgentEnabled() {
    return _bleAgentEnabled;
  };
  bool setStatusMessage(StatusMessage msg);
  bool setNetworkOptions(NetworkOptions netOptions);
  bool setID(String uhwid, String jwt);
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
    return _state != AgentsConfiguratorManagerStates::INIT && _state != AgentsConfiguratorManagerStates::END;
  };
private:
  AgentsConfiguratorManagerStates _state = AgentsConfiguratorManagerStates::END;
  std::list<ConfiguratorAgent *> _agentsList;
  std::list<uint8_t> _servicesList;
  ConfiguratorRequestHandler _reqHandlers[3];
  ReturnTimestamp _returnTimestampCb = nullptr;
  ReturnNetworkSettings _returnNetworkSettingsCb = nullptr;
  ConfiguratorAgent *_selectedAgent = nullptr;
  uint8_t _instances = 0;
  bool _bleAgentEnabled = true;
  StatusMessage _initStatusMsg = MessageTypeCodes::NONE;
  NetworkOptions _netOptions;
  typedef struct {
    void reset() {
      pending = false;
      key = RequestType::NONE;
    };
    bool pending;
    RequestType key;
  } StatusRequest;

  StatusRequest _statusRequest = { .pending = false, .key = RequestType::NONE };

  AgentsConfiguratorManagerStates handleInit();
  AgentsConfiguratorManagerStates handleSendInitialStatus();
  AgentsConfiguratorManagerStates handleSendNetworkOptions();
  AgentsConfiguratorManagerStates handleConfInProgress();
  void handleReceivedCommands(RemoteCommands cmd);
  void handleReceivedData();
  void handleConnectCommand();
  void handleUpdateOptCommand();
  void handleGetIDCommand();
  void handleGetBleMacAddressCommand();
  bool sendNetworkOptions();
  bool sendStatus(StatusMessage msg);

  AgentsConfiguratorManagerStates handlePeerDisconnected();
  void callHandler(RequestType key);
  void stopBLEAgent();
  void startBLEAgent();
};

extern AgentsConfiguratorManager ConfiguratorManager;
