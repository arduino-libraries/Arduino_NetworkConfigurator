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

enum class RequestType: int { NONE = -1,
                              CONNECT = 0,
                              SCAN = 1,
                              GET_ID  = 2,
                              RESET = 3,
                              GET_WIFI_FW_VERSION = 4,
                              GET_BLE_MAC_ADDRESS = 5 };

class AgentsManagerClass {
public:
  static AgentsManagerClass &getInstance();
  bool begin();
  bool end();
  void disconnect();
  AgentsManagerStates poll();
  void enableBLEAgent(bool enable);
  bool isBLEAgentEnabled();
  ConfiguratorAgent *getConnectedAgent();
  bool sendMsg(ProvisioningOutputMessage &msg);
  bool addAgent(ConfiguratorAgent &agent);
  bool addRequestHandler(RequestType type, ConfiguratorRequestHandler callback);
  void removeRequestHandler(RequestType type);
  bool addReturnTimestampCallback(ReturnTimestamp callback);
  void removeReturnTimestampCallback();
  bool addReturnNetworkSettingsCallback(ReturnNetworkSettings callback);
  void removeReturnNetworkSettingsCallback();
  bool isConfigInProgress();

private:
  AgentsManagerClass();
  AgentsManagerStates _state;
  std::list<ConfiguratorAgent *> _agentsList;
  ConfiguratorRequestHandler _reqHandlers[6];
  ReturnTimestamp _returnTimestampCb;
  ReturnNetworkSettings _returnNetworkSettingsCb;
  ConfiguratorAgent *_selectedAgent;
  uint8_t _instances;
  bool _bleAgentEnabled;
  StatusMessage _initStatusMsg;
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

  StatusRequest _statusRequest;

  AgentsManagerStates handleInit();
  AgentsManagerStates handleSendInitialStatus();
  AgentsManagerStates handleSendNetworkOptions();
  AgentsManagerStates handleConfInProgress();
  void updateProgressRequest(StatusMessage type);
  void updateProgressRequest(MessageOutputType type);
  void handleReceivedCommands(RemoteCommands cmd);
  void handleReceivedData();

  bool sendStatus(StatusMessage msg);

  void handlePeerDisconnected();
  void stopBLEAgent();
  void startBLEAgent();
};
