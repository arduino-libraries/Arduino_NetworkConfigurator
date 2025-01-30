/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <Arduino_DebugUtils.h>
#include <algorithm>
#include <settings/settings.h>
#include "AgentsConfiguratorManager.h"
#include "NetworkOptionsDefinitions.h"
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
#define BOARD_HAS_BLE
#endif

#ifdef BOARD_HAS_BLE
#include <ArduinoBLE.h>
#include "utility/HCI.h"
#endif

bool AgentsConfiguratorManager::addAgent(ConfiguratorAgent &agent) {
  _agentsList.push_back(&agent);
}

bool AgentsConfiguratorManager::begin(uint8_t id) {
  if (_state == AgentsConfiguratorManagerStates::END) {
    pinMode(LED_BUILTIN, OUTPUT);

    for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
      if ((*agent)->begin() == ConfiguratorAgent::AgentConfiguratorStates::ERROR) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s agent type %d fails", __FUNCTION__, (int)(*agent)->getAgentType());
        return false;
      }
    }

    DEBUG_DEBUG("AgentsConfiguratorManager begin completed");
    _state = AgentsConfiguratorManagerStates::INIT;
  }

  _servicesList.push_back(id);

  _instances++;

  return true;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::poll() {
  switch (_state) {
    case AgentsConfiguratorManagerStates::INIT:                 _state = handleInit              (); break;
    case AgentsConfiguratorManagerStates::SEND_INITIAL_STATUS:  _state = handleSendInitialStatus (); break;
    case AgentsConfiguratorManagerStates::SEND_NETWORK_OPTIONS: _state = handleSendNetworkOptions(); break;
    case AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS:   _state = handleConfInProgress    (); break;
    case AgentsConfiguratorManagerStates::END:                                                       break;
  }

  return _state;
}

void AgentsConfiguratorManager::enableBLEAgent(bool enable) {
  if (_bleAgentEnabled == enable) {
    return;
  }
  _bleAgentEnabled = enable;

  if (enable) {
    startBLEAgent();
  } else {
    stopBLEAgent();
  }
}

bool AgentsConfiguratorManager::end(uint8_t id) {
  std::list<uint8_t>::iterator it = std::find(_servicesList.begin(), _servicesList.end(), id);
  if (it != _servicesList.end()) {
    _servicesList.erase(it);
  } else {
    return false;
  }

  _instances--;
  if (_instances == 0) {
    std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
      agent->end();
    });

#if defined(ARDUINO_PORTENTA_H7_M7)
    digitalWrite(LED_BUILTIN, HIGH);
#else
    digitalWrite(LED_BUILTIN, LOW);
#endif
    _selectedAgent = nullptr;
    _statusRequest.reset();
    _initStatusMsg = StatusMessage::NONE;
    _state = AgentsConfiguratorManagerStates::END;
  }

  return true;
}

void AgentsConfiguratorManager::disconnect() {
  if (_selectedAgent) {
    _selectedAgent->disconnectPeer();
    _state = handlePeerDisconnected();
  }
}

bool AgentsConfiguratorManager::sendMsg(ProvisioningOutputMessage &msg) {

  switch (msg.type) {
    case MessageOutputType::STATUS:
      {
        if ((int)msg.m.status < 0) {
          if (_statusRequest.pending) {
            _statusRequest.reset();
          }
          _initStatusMsg = msg.m.status;
        } else if (msg.m.status == StatusMessage::CONNECTED) {
          if (_statusRequest.pending && _statusRequest.key == RequestType::CONNECT) {
            _statusRequest.reset();
          }
          _initStatusMsg = msg.m.status;
        } else if (msg.m.status == StatusMessage::RESET_COMPLETED) {
          if (_statusRequest.pending && _statusRequest.key == RequestType::RESET) {
            _statusRequest.reset();
          }
        }
      }
      break;
    case MessageOutputType::NETWORK_OPTIONS:
      {
        memcpy(&_netOptions, msg.m.netOptions, sizeof(NetworkOptions));
        if (_statusRequest.pending && _statusRequest.key == RequestType::SCAN) {
          _statusRequest.reset();
        }
      }
      break;
    case MessageOutputType::UHWID:
    case MessageOutputType::JWT:
      {
        _statusRequest.completion++;
        if (_statusRequest.pending && _statusRequest.key == RequestType::GET_ID && _statusRequest.completion == 2) {
          _statusRequest.reset();
        }
      }
      break;
    default:
      break;
  }

  if(_state == AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS) {
    return _selectedAgent->sendMsg(msg);
  }
  return true;
}

bool AgentsConfiguratorManager::addRequestHandler(RequestType type, ConfiguratorRequestHandler callback) {
  bool success = false;
  if (_reqHandlers[(int)type - 1] == nullptr) {
    _reqHandlers[(int)type - 1] = callback;
    success = true;
  }

  return success;
}

bool AgentsConfiguratorManager::addReturnTimestampCallback(ReturnTimestamp callback) {
  if (_returnTimestampCb == nullptr) {
    _returnTimestampCb = callback;
    return true;
  }

  return true;
}

bool AgentsConfiguratorManager::addReturnNetworkSettingsCallback(ReturnNetworkSettings callback) {
  if (_returnNetworkSettingsCb == nullptr) {
    _returnNetworkSettingsCb = callback;
    return true;
  }
  return false;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleInit() {
  AgentsConfiguratorManagerStates nextState = _state;

  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->poll() == ConfiguratorAgent::AgentConfiguratorStates::PEER_CONNECTED) {
      _selectedAgent = *agent;
#if defined(ARDUINO_PORTENTA_H7_M7)
      digitalWrite(LED_BUILTIN, LOW);
#else
      digitalWrite(LED_BUILTIN, HIGH);
#endif
      nextState = AgentsConfiguratorManagerStates::SEND_INITIAL_STATUS;
      break;
    }
  }

  //stop all other agents
  if (_selectedAgent != nullptr) {
    for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
      if (*agent != _selectedAgent) {
        (*agent)->end();
      }
    }
  }
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleSendInitialStatus() {
  AgentsConfiguratorManagerStates nextState = _state;
  if (_initStatusMsg != StatusMessage::NONE) {
    if (!sendStatus(_initStatusMsg)) {
      DEBUG_WARNING("AgentsConfiguratorManager::%s failed to send initial status", __FUNCTION__);
      return nextState;
    }
    _initStatusMsg = StatusMessage::NONE;
  }
  nextState = AgentsConfiguratorManagerStates::SEND_NETWORK_OPTIONS;
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleSendNetworkOptions() {
  AgentsConfiguratorManagerStates nextState = _state;
  ProvisioningOutputMessage networkOptionMsg = { MessageOutputType::NETWORK_OPTIONS };
  networkOptionMsg.m.netOptions = &_netOptions;
  if (_selectedAgent->sendMsg(networkOptionMsg)) {
    nextState = AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;
  }
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleConfInProgress() {
  AgentsConfiguratorManagerStates nextState = _state;

  if (_selectedAgent == nullptr) {
    DEBUG_WARNING("AgentsConfiguratorManager::%s selected agent is null", __FUNCTION__);
    return AgentsConfiguratorManagerStates::INIT;
  }

  ConfiguratorAgent::AgentConfiguratorStates agentConfState = _selectedAgent->poll();
  switch (agentConfState) {
    case ConfiguratorAgent::AgentConfiguratorStates::RECEIVED_DATA:
      handleReceivedData();
      break;
    case ConfiguratorAgent::AgentConfiguratorStates::INIT:
      nextState = handlePeerDisconnected();
      break;
  }

  return nextState;
}

void AgentsConfiguratorManager::handleReceivedCommands(RemoteCommands cmd) {
  switch (cmd) {
    case RemoteCommands::CONNECT:             handleConnectCommand         (); break;
    case RemoteCommands::SCAN:                handleUpdateOptCommand       (); break;
    case RemoteCommands::GET_ID:              handleGetIDCommand           (); break;
    case RemoteCommands::GET_BLE_MAC_ADDRESS: handleGetBleMacAddressCommand(); break;
    case RemoteCommands::RESET:               handleResetCommand           (); break;
  }
}

void AgentsConfiguratorManager::handleReceivedData() {
  if (!_selectedAgent->receivedMsgAvailable()) {
    return;
  }
  ProvisioningInputMessage msg;
  if (!_selectedAgent->getReceivedMsg(msg)) {
    DEBUG_WARNING("AgentsConfiguratorManager::%s failed to get received data", __FUNCTION__);
    return;
  }

  switch (msg.type) {
    case MessageInputType::TIMESTAMP:
      if (_returnTimestampCb != nullptr) {
        _returnTimestampCb(msg.m.timestamp);
      }
      break;
    case MessageInputType::NETWORK_SETTINGS:
      if (_returnNetworkSettingsCb != nullptr) {
        _returnNetworkSettingsCb(&msg.m.netSetting);
      }
      break;
    case MessageInputType::COMMANDS:
      handleReceivedCommands(msg.m.cmd);
      break;
    default:
      break;
  }
}

void AgentsConfiguratorManager::handleConnectCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a Connect request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::CONNECT;
  callHandler(RequestType::CONNECT);
}

void AgentsConfiguratorManager::handleUpdateOptCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a UpdateConnectivityOptions request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::SCAN;
  callHandler(RequestType::SCAN);
}

void AgentsConfiguratorManager::handleGetIDCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a GetUnique request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::GET_ID;
  callHandler(RequestType::GET_ID);
}

void AgentsConfiguratorManager::handleGetBleMacAddressCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a GetBleMacAddress request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  uint8_t mac[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#ifdef BOARD_HAS_BLE
  if(!isBLEAgentEnabled()){
    BLE.begin();
  }

  HCI.readBdAddr(mac);

  for(int i = 0; i < 3; i++){
    uint8_t byte = mac[i];
    mac[i] = mac[5-i];
    mac[5-i] = byte;
  }
#endif

  ProvisioningOutputMessage outputMsg;
  outputMsg.type = MessageOutputType::BLE_MAC_ADDRESS;
  outputMsg.m.BLEMacAddress = mac;
  _selectedAgent->sendMsg(outputMsg);

}

void AgentsConfiguratorManager::handleResetCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a GetUnique request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::RESET;
  callHandler(RequestType::RESET);
}

bool AgentsConfiguratorManager::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage outputMsg = { MessageOutputType::STATUS, { msg } };
  return _selectedAgent->sendMsg(outputMsg);
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handlePeerDisconnected() {
  //Peer disconnected, restore all stopped agents
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if (*agent != _selectedAgent) {
      if ((*agent)->getAgentType() == ConfiguratorAgent::AgentTypes::BLE && !_bleAgentEnabled) {
        continue;
      }
      (*agent)->begin();
    }
  }
#if defined(ARDUINO_PORTENTA_H7_M7)
  digitalWrite(LED_BUILTIN, HIGH);
#else
  digitalWrite(LED_BUILTIN, LOW);
#endif
  _selectedAgent = nullptr;
  return AgentsConfiguratorManagerStates::INIT;
}

void AgentsConfiguratorManager::callHandler(RequestType type) {
  int key = (int)type - 1;
  if (_reqHandlers[key] != nullptr) {
    _reqHandlers[key]();
  } else {
    String err;
    if (type == RequestType::SCAN) {
      err = "Scan ";
    } else if (type == RequestType::GET_ID) {
      err = "Get ID ";
    } else if (type == RequestType::CONNECT) {
      err = "Connect ";
    }

    DEBUG_WARNING("AgentsConfiguratorManager::%s %s request received, but handler function is not provided", __FUNCTION__, err.c_str());
    _statusRequest.reset();
    sendStatus(StatusMessage::INVALID_REQUEST);
  }
}

void AgentsConfiguratorManager::stopBLEAgent() {
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsConfiguratorManager::%s End ble agent", __FUNCTION__);
      (*agent)->end();
      if (*agent == _selectedAgent) {
        _state = handlePeerDisconnected();
      }
    }
  }
}

void AgentsConfiguratorManager::startBLEAgent() {
  if (_state == AgentsConfiguratorManagerStates::END || !_bleAgentEnabled) {
    return;
  }
  std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
    if (agent->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsConfiguratorManager::%s Restart ble agent", __FUNCTION__);
      agent->begin();
    }
  });
}


AgentsConfiguratorManager ConfiguratorManager;
