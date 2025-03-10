/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <Arduino_DebugUtils.h>
#include <algorithm>
#include <settings/settings.h>
#include "AgentsManager.h"
#include "NetworkOptionsDefinitions.h"
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
#define BOARD_HAS_BLE
#endif

#ifdef BOARD_HAS_BLE
#include <ArduinoBLE.h>
#include "utility/HCI.h"
#endif

bool AgentsManagerClass::addAgent(ConfiguratorAgent &agent) {
  _agentsList.push_back(&agent);
}

bool AgentsManagerClass::begin(uint8_t id) {
  if (_state == AgentsManagerStates::END) {
    pinMode(LED_BUILTIN, OUTPUT);

    for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
      if((*agent)->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
        if (!_bleAgentEnabled) {
          continue;
        }
      }
      if ((*agent)->begin() == ConfiguratorAgent::AgentConfiguratorStates::ERROR) {
        DEBUG_ERROR("AgentsManagerClass::%s agent type %d fails", __FUNCTION__, (int)(*agent)->getAgentType());
        return false;
      }
    }

    DEBUG_DEBUG("AgentsManagerClass begin completed");
    _state = AgentsManagerStates::INIT;
  }

  _servicesList.push_back(id);

  _instances++;

  return true;
}

AgentsManagerStates AgentsManagerClass::poll() {
  switch (_state) {
    case AgentsManagerStates::INIT:                 _state = handleInit              (); break;
    case AgentsManagerStates::SEND_INITIAL_STATUS:  _state = handleSendInitialStatus (); break;
    case AgentsManagerStates::SEND_NETWORK_OPTIONS: _state = handleSendNetworkOptions(); break;
    case AgentsManagerStates::CONFIG_IN_PROGRESS:   _state = handleConfInProgress    (); break;
    case AgentsManagerStates::END:                                                       break;
  }

  return _state;
}

void AgentsManagerClass::enableBLEAgent(bool enable) {
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

bool AgentsManagerClass::end(uint8_t id) {
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
    _state = AgentsManagerStates::END;
  }

  return true;
}

void AgentsManagerClass::disconnect() {
  if (_selectedAgent) {
    _selectedAgent->disconnectPeer();
    _state = handlePeerDisconnected();
  }
}

bool AgentsManagerClass::sendMsg(ProvisioningOutputMessage &msg) {

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
    case MessageOutputType::WIFI_FW_VERSION:
      {
        if (_statusRequest.pending && _statusRequest.key == RequestType::GET_WIFI_FW_VERSION) {
          _statusRequest.reset();
        }
      }
      break;
    default:
      break;
  }

  if(_state == AgentsManagerStates::CONFIG_IN_PROGRESS) {
    return _selectedAgent->sendMsg(msg);
  }
  return true;
}

bool AgentsManagerClass::addRequestHandler(RequestType type, ConfiguratorRequestHandler callback) {
  bool success = false;
  if (_reqHandlers[(int)type - 1] == nullptr) {
    _reqHandlers[(int)type - 1] = callback;
    success = true;
  }

  return success;
}

bool AgentsManagerClass::addReturnTimestampCallback(ReturnTimestamp callback) {
  if (_returnTimestampCb == nullptr) {
    _returnTimestampCb = callback;
    return true;
  }

  return true;
}

bool AgentsManagerClass::addReturnNetworkSettingsCallback(ReturnNetworkSettings callback) {
  if (_returnNetworkSettingsCb == nullptr) {
    _returnNetworkSettingsCb = callback;
    return true;
  }
  return false;
}

AgentsManagerStates AgentsManagerClass::handleInit() {
  AgentsManagerStates nextState = _state;

  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->poll() == ConfiguratorAgent::AgentConfiguratorStates::PEER_CONNECTED) {
      _selectedAgent = *agent;
#if defined(ARDUINO_PORTENTA_H7_M7)
      digitalWrite(LED_BUILTIN, LOW);
#else
      digitalWrite(LED_BUILTIN, HIGH);
#endif
      nextState = AgentsManagerStates::SEND_INITIAL_STATUS;
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

AgentsManagerStates AgentsManagerClass::handleSendInitialStatus() {
  AgentsManagerStates nextState = _state;
  if (_initStatusMsg != StatusMessage::NONE) {
    if (!sendStatus(_initStatusMsg)) {
      DEBUG_WARNING("AgentsManagerClass::%s failed to send initial status", __FUNCTION__);
      return nextState;
    }
    _initStatusMsg = StatusMessage::NONE;
  }
  nextState = AgentsManagerStates::SEND_NETWORK_OPTIONS;
  return nextState;
}

AgentsManagerStates AgentsManagerClass::handleSendNetworkOptions() {
  AgentsManagerStates nextState = _state;
  ProvisioningOutputMessage networkOptionMsg = { MessageOutputType::NETWORK_OPTIONS };
  networkOptionMsg.m.netOptions = &_netOptions;
  if (_selectedAgent->sendMsg(networkOptionMsg)) {
    nextState = AgentsManagerStates::CONFIG_IN_PROGRESS;
  }
  return nextState;
}

AgentsManagerStates AgentsManagerClass::handleConfInProgress() {
  AgentsManagerStates nextState = _state;

  if (_selectedAgent == nullptr) {
    DEBUG_WARNING("AgentsManagerClass::%s selected agent is null", __FUNCTION__);
    return AgentsManagerStates::INIT;
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

void AgentsManagerClass::handleReceivedCommands(RemoteCommands cmd) {
  switch (cmd) {
    case RemoteCommands::CONNECT:             handleConnectCommand         (); break;
    case RemoteCommands::SCAN:                handleUpdateOptCommand       (); break;
    case RemoteCommands::GET_ID:              handleGetIDCommand           (); break;
    case RemoteCommands::GET_BLE_MAC_ADDRESS: handleGetBleMacAddressCommand(); break;
    case RemoteCommands::RESET:               handleResetCommand           (); break;
    case RemoteCommands::GET_WIFI_FW_VERSION: handleGetWiFiFWVersionCommand(); break;
  }
}

void AgentsManagerClass::handleReceivedData() {
  if (!_selectedAgent->receivedMsgAvailable()) {
    return;
  }
  ProvisioningInputMessage msg;
  if (!_selectedAgent->getReceivedMsg(msg)) {
    DEBUG_WARNING("AgentsManagerClass::%s failed to get received data", __FUNCTION__);
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

void AgentsManagerClass::handleConnectCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a Connect request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::CONNECT;
  callHandler(RequestType::CONNECT);
}

void AgentsManagerClass::handleUpdateOptCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a UpdateConnectivityOptions request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::SCAN;
  callHandler(RequestType::SCAN);
}

void AgentsManagerClass::handleGetIDCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a GetUnique request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::GET_ID;
  callHandler(RequestType::GET_ID);
}

void AgentsManagerClass::handleGetBleMacAddressCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a GetBleMacAddress request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  uint8_t mac[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#ifdef BOARD_HAS_BLE
 bool activated = false;
  if(!isBLEAgentEnabled() || (_selectedAgent != nullptr &&
    _selectedAgent->getAgentType() != ConfiguratorAgent::AgentTypes::BLE)) {
    activated = true;
    BLE.begin();
  }

  HCI.readBdAddr(mac);

  for(int i = 0; i < 3; i++){
    uint8_t byte = mac[i];
    mac[i] = mac[5-i];
    mac[5-i] = byte;
  }
  if (activated) {
    BLE.end();
  }
#endif

  ProvisioningOutputMessage outputMsg;
  outputMsg.type = MessageOutputType::BLE_MAC_ADDRESS;
  outputMsg.m.BLEMacAddress = mac;
  _selectedAgent->sendMsg(outputMsg);

}

void AgentsManagerClass::handleResetCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a Reset request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::RESET;
  callHandler(RequestType::RESET);
}

void AgentsManagerClass::handleGetWiFiFWVersionCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsManagerClass::%s received a GetWiFiFWVersion request while executing another request", __FUNCTION__);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::GET_WIFI_FW_VERSION;
  callHandler(RequestType::GET_WIFI_FW_VERSION);
}

bool AgentsManagerClass::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage outputMsg = { MessageOutputType::STATUS, { msg } };
  return _selectedAgent->sendMsg(outputMsg);
}

AgentsManagerStates AgentsManagerClass::handlePeerDisconnected() {
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
  return AgentsManagerStates::INIT;
}

void AgentsManagerClass::callHandler(RequestType type) {
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

    DEBUG_WARNING("AgentsManagerClass::%s %s request received, but handler function is not provided", __FUNCTION__, err.c_str());
    _statusRequest.reset();
    sendStatus(StatusMessage::INVALID_REQUEST);
  }
}

void AgentsManagerClass::stopBLEAgent() {
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsManagerClass::%s End ble agent", __FUNCTION__);
      (*agent)->end();
      if (*agent == _selectedAgent) {
        _state = handlePeerDisconnected();
      }
    }
  }
}

void AgentsManagerClass::startBLEAgent() {
  if (_state == AgentsManagerStates::END || !_bleAgentEnabled) {
    return;
  }
  std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
    if (agent->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsManagerClass::%s Restart ble agent", __FUNCTION__);
      agent->begin();
    }
  });
}


AgentsManagerClass AgentsManager;
