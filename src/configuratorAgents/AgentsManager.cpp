/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include <Arduino_DebugUtils.h>
#include <algorithm>
#include "AgentsManager.h"
#include "NetworkOptionsDefinitions.h"
#include "utility/LEDFeedback.h"

/******************************************************************************
 * PUBLIC MEMBER FUNCTIONS
 ******************************************************************************/

bool AgentsManagerClass::addAgent(ConfiguratorAgent &agent) {
#ifdef ARDUINO_OPTA
  if(agent.getAgentType() == ConfiguratorAgent::AgentTypes::BLE && _getPid_() != OPTA_WIFI_PID) {
    return false;
  }
#endif
  _agentsList.push_back(&agent);
  return true;
}

AgentsManagerClass &AgentsManagerClass::getInstance() {
  static AgentsManagerClass instance;
  return instance;
};

bool AgentsManagerClass::begin() {
  _instances++;

  if (_state != AgentsManagerStates::END) {
    return true;
  }

  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if( _enabledAgents[(int)(*agent)->getAgentType()] == false) {
      continue;
    }
    if ((*agent)->begin() == ConfiguratorAgent::AgentConfiguratorStates::ERROR) {
      DEBUG_ERROR("AgentsManagerClass::%s agent type %d fails", __FUNCTION__, (int)(*agent)->getAgentType());
      return false;
    }
  }

  _state = AgentsManagerStates::INIT;

  return true;
}

AgentsManagerStates AgentsManagerClass::update() {
  switch (_state) {
    case AgentsManagerStates::INIT:                 _state = handleInit              (); break;
    case AgentsManagerStates::SEND_INITIAL_STATUS:  _state = handleSendInitialStatus (); break;
    case AgentsManagerStates::SEND_NETWORK_OPTIONS: _state = handleSendNetworkOptions(); break;
    case AgentsManagerStates::CONFIG_IN_PROGRESS:   _state = handleConfInProgress    (); break;
    case AgentsManagerStates::END:                                                       break;
  }

  return _state;
}

void AgentsManagerClass::enableAgent(ConfiguratorAgent::AgentTypes type, bool enable) {
  bool _agentState = _enabledAgents[(int)type];

  if (_agentState == enable) {
    return;
  }

  _enabledAgents[(int)type] = enable;
  if (enable) {
    startAgent(type);
  } else {
    stopAgent(type);
  }
}

bool AgentsManagerClass::isAgentEnabled(ConfiguratorAgent::AgentTypes type) {
  return _enabledAgents[(int)type];
}

bool AgentsManagerClass::startAgent(ConfiguratorAgent::AgentTypes type) {
  if (_state == AgentsManagerStates::END) {
    return false;
  }

  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->getAgentType() == type) {
      (*agent)->begin();
      return true;
    }
  }
  return false;
}

bool AgentsManagerClass::stopAgent(ConfiguratorAgent::AgentTypes type) {
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->getAgentType() == type) {
      (*agent)->end();
      if (*agent == _selectedAgent) {
        handlePeerDisconnected();
        _state = AgentsManagerStates::INIT;
      }
      return true;
    }
  }
  return false;
}

bool AgentsManagerClass::end() {
  _instances--;
  if (_instances == 0) {
    std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
      agent->end();
    });
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
    handlePeerDisconnected();
    _state = AgentsManagerStates::INIT;
  }
}

ConfiguratorAgent *AgentsManagerClass::getConnectedAgent() {
  return _selectedAgent;
}

bool AgentsManagerClass::sendMsg(ProvisioningOutputMessage &msg) {

  if(msg.type == MessageOutputType::STATUS) {
    updateProgressRequest(msg.m.status);
    if ((int)msg.m.status < 0 || msg.m.status == StatusMessage::CONNECTED) {
      _initStatusMsg = msg.m.status;
    }
  } else {
    updateProgressRequest(msg.type);
    if(msg.type == MessageOutputType::NETWORK_OPTIONS){
      memcpy(&_netOptions, msg.m.netOptions, sizeof(NetworkOptions));
    }
  }

  if(_state == AgentsManagerStates::CONFIG_IN_PROGRESS) {
    return _selectedAgent->sendMsg(msg);
  }
  return true;
}

bool AgentsManagerClass::addRequestHandler(RequestType type, ConfiguratorRequestHandler callback) {
  if(type == RequestType::NONE) {
    return false;
  }

  if (_reqHandlers[(int)type] == nullptr) {
    _reqHandlers[(int)type] = callback;
    return true;
  }

  return false;
}

void AgentsManagerClass::removeRequestHandler(RequestType type) {
  if(type == RequestType::NONE) {
    return;
  }
  _reqHandlers[(int)type] = nullptr;
}

bool AgentsManagerClass::addReturnTimestampCallback(ReturnTimestamp callback) {
  if (_returnTimestampCb == nullptr) {
    _returnTimestampCb = callback;
    return true;
  }

  return true;
}

void AgentsManagerClass::removeReturnTimestampCallback() {
  _returnTimestampCb = nullptr;
}

bool AgentsManagerClass::addReturnNetworkSettingsCallback(ReturnNetworkSettings callback) {
  if (_returnNetworkSettingsCb == nullptr) {
    _returnNetworkSettingsCb = callback;
    return true;
  }
  return false;
}

void AgentsManagerClass::removeReturnNetworkSettingsCallback() {
  _returnNetworkSettingsCb = nullptr;
}

bool AgentsManagerClass::isConfigInProgress() {
  return _state != AgentsManagerStates::INIT && _state != AgentsManagerStates::END;
}

/******************************************************************************
 * PRIVATE MEMBER FUNCTIONS
 ******************************************************************************/

AgentsManagerClass::AgentsManagerClass():
  _netOptions{ .type = NetworkOptionsClass::NONE },
  _returnTimestampCb{ nullptr },
  _returnNetworkSettingsCb{ nullptr },
  _selectedAgent{ nullptr },
  _instances{ 0 },
  _initStatusMsg{ StatusMessage::NONE },
  _statusRequest{ .completion = 0, .pending = false, .key = RequestType::NONE },
  _state{ AgentsManagerStates::END }
  {
    memset(_enabledAgents, 0x01, sizeof(_enabledAgents));
  }

AgentsManagerStates AgentsManagerClass::handleInit() {
  AgentsManagerStates nextState = _state;

  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if ((*agent)->update() == ConfiguratorAgent::AgentConfiguratorStates::PEER_CONNECTED) {
      _selectedAgent = *agent;
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

    if(_selectedAgent->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::PEER_CONNECTED_BLE);
    } else if(_selectedAgent->getAgentType() == ConfiguratorAgent::AgentTypes::USB_SERIAL) {
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::PEER_CONNECTED_SERIAL);
    } else {
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::PEER_CONNECTED);
    }

  }
  return nextState;
}

AgentsManagerStates AgentsManagerClass::handleSendInitialStatus() {
  if (_initStatusMsg != StatusMessage::NONE) {
    if (!sendStatus(_initStatusMsg)) {
      DEBUG_WARNING("AgentsManagerClass::%s failed to send initial status", __FUNCTION__);
      return AgentsManagerStates::SEND_INITIAL_STATUS;
    }
  }
  return AgentsManagerStates::SEND_NETWORK_OPTIONS;
}

AgentsManagerStates AgentsManagerClass::handleSendNetworkOptions() {
  ProvisioningOutputMessage networkOptionMsg = { MessageOutputType::NETWORK_OPTIONS };
  networkOptionMsg.m.netOptions = &_netOptions;
  if (_selectedAgent->sendMsg(networkOptionMsg)) {
    return AgentsManagerStates::CONFIG_IN_PROGRESS;
  }
  return AgentsManagerStates::SEND_NETWORK_OPTIONS;
}

AgentsManagerStates AgentsManagerClass::handleConfInProgress() {
  if (_selectedAgent == nullptr) {
    return AgentsManagerStates::INIT;
  }

  ConfiguratorAgent::AgentConfiguratorStates agentConfState = _selectedAgent->update();
  switch (agentConfState) {
    case ConfiguratorAgent::AgentConfiguratorStates::RECEIVED_DATA: handleReceivedData    (); break;
    case ConfiguratorAgent::AgentConfiguratorStates::INIT:          handlePeerDisconnected(); return AgentsManagerStates::INIT;
  }

  return AgentsManagerStates::CONFIG_IN_PROGRESS;
}

void AgentsManagerClass::updateProgressRequest(StatusMessage type) {
  if ((int)type < 0 && _statusRequest.pending) {
    _statusRequest.reset();
    return;
  }

  RequestType key = RequestType::NONE;
  switch (type) {
    case StatusMessage::CONNECTED:       key = RequestType::CONNECT; break;
    case StatusMessage::RESET_COMPLETED: key = RequestType::RESET;   break;
  }

  if (key == RequestType::NONE) {
    return;
  }

  if (_statusRequest.pending && _statusRequest.key == key) {
    _statusRequest.reset();
  }
}

void AgentsManagerClass::updateProgressRequest(MessageOutputType type) {
  RequestType key = RequestType::NONE;
  switch (type) {
    case MessageOutputType::NETWORK_OPTIONS:       key = RequestType::SCAN                           ; break;
    case MessageOutputType::UHWID:                 key = RequestType::GET_ID                         ; break;
    case MessageOutputType::JWT:                   key = RequestType::GET_ID                         ; break;
    case MessageOutputType::BLE_MAC_ADDRESS:       key = RequestType::GET_BLE_MAC_ADDRESS            ; break;
    case MessageOutputType::WIFI_FW_VERSION:       key = RequestType::GET_WIFI_FW_VERSION            ; break;
    case MessageOutputType::PROV_SKETCH_VERSION:   key = RequestType::GET_PROVISIONING_SKETCH_VERSION; break;
    case MessageOutputType::NETCONFIG_LIB_VERSION: key = RequestType::GET_NETCONFIG_LIB_VERSION      ; break;
    case MessageOutputType::PROV_PUBLIC_KEY:       key = RequestType::GET_ID                         ; break;
  }

  if (key == RequestType::NONE) {
    return;
  }

  if( key == RequestType::GET_ID && _statusRequest.key == key && _statusRequest.pending){
    _statusRequest.completion++;
    if(_statusRequest.completion == 3){
      _statusRequest.reset();
    }
    return;
  }

  if (_statusRequest.pending && _statusRequest.key == key) {
    _statusRequest.reset();
  }

}

void AgentsManagerClass::handleReceivedCommands(RemoteCommands cmd) {
  if (_statusRequest.pending) {
    DEBUG_WARNING("AgentsManagerClass::%s request received of type %d, but request %d is in progress", __FUNCTION__, (int)cmd, (int)_statusRequest.key);
    sendStatus(StatusMessage::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  RequestType type = RequestType::NONE;
  switch (cmd) {
    case RemoteCommands::CONNECT:                         type = RequestType::CONNECT                        ; break;
    case RemoteCommands::SCAN:                            type = RequestType::SCAN                           ; break;
    case RemoteCommands::GET_ID:                          type = RequestType::GET_ID                         ; break;
    case RemoteCommands::GET_BLE_MAC_ADDRESS:             type = RequestType::GET_BLE_MAC_ADDRESS            ; break;
    case RemoteCommands::RESET:                           type = RequestType::RESET                          ; break;
    case RemoteCommands::GET_WIFI_FW_VERSION:             type = RequestType::GET_WIFI_FW_VERSION            ; break;
    case RemoteCommands::GET_PROVISIONING_SKETCH_VERSION: type = RequestType::GET_PROVISIONING_SKETCH_VERSION; break;
    case RemoteCommands::GET_NETCONFIG_LIB_VERSION:       type = RequestType::GET_NETCONFIG_LIB_VERSION      ; break;
  }

  if(type == RequestType::NONE) {
    sendStatus(StatusMessage::INVALID_REQUEST);
    return;
  }

  ConfiguratorRequestHandler reqHandler = _reqHandlers[(int)type];
  if (reqHandler == nullptr) {
    DEBUG_WARNING("AgentsManagerClass::%s request received of type %d, but handler function is not provided", __FUNCTION__, (int)type);
    sendStatus(StatusMessage::INVALID_REQUEST);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = type;
  reqHandler();
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

bool AgentsManagerClass::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage outputMsg = { MessageOutputType::STATUS, { msg } };
  return _selectedAgent->sendMsg(outputMsg);
}

void AgentsManagerClass::handlePeerDisconnected() {
  //Peer disconnected, restore all stopped agents
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if (_enabledAgents[(int)(*agent)->getAgentType()] == false) {
      (*agent)->end();
      continue;
    }

    if (*agent != _selectedAgent) {
      (*agent)->begin();
    }
  }
  _selectedAgent = nullptr;
  return;
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
