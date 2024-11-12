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
#include "CBORAdapter.h"
#include "cbor/CBOR.h"

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
    _initStatusMsg = MessageTypeCodes::NONE;
    _state = AgentsConfiguratorManagerStates::END;
  }

  return true;
}

bool AgentsConfiguratorManager::setStatusMessage(StatusMessage msg) {
  if ((int)msg < 0) {
    if (_statusRequest.pending) {
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
      startBLEAgent();
#endif
      _statusRequest.reset();
    }
  } else if (msg == MessageTypeCodes::CONNECTING) {
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
    stopBLEAgent();
#endif
    if (!_statusRequest.pending && _statusRequest.key != RequestType::CONNECT) {
      _statusRequest.pending = true;
      _statusRequest.key = RequestType::CONNECT;
    }
  } else if (msg == MessageTypeCodes::SCANNING) {
#ifdef BOARD_HAS_WIFI
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
    stopBLEAgent();
#endif
#endif
  } else if (msg == MessageTypeCodes::CONNECTED) {
    if (_statusRequest.pending && _statusRequest.key == RequestType::CONNECT) {
      _statusRequest.reset();
    }
  }

  if (msg == MessageTypeCodes::SCANNING && !_selectedAgent) {
    return true;
  }

  if (_selectedAgent) {
    if (!sendStatus(msg)) {
      DEBUG_WARNING("AgentsConfiguratorManager::%s failed to send status to the peer", __FUNCTION__);
    }
  } else {
    _initStatusMsg = msg;
  }

  return true;
}

bool AgentsConfiguratorManager::setNetworkOptions(NetworkOptions netOptions) {
  memcpy(&_netOptions, &netOptions, sizeof(NetworkOptions));
  if (_statusRequest.pending && _statusRequest.key == RequestType::SCAN) {
    _statusRequest.reset();
  }

#ifdef BOARD_HAS_WIFI
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
  if (netOptions.type == NetworkOptionsClass::WIFI) {
    startBLEAgent();
  }
#endif
#endif

  if (_selectedAgent) {
    //Send immediatly if agent is connected with a peer
    sendNetworkOptions();
  }

  return true;
}

bool AgentsConfiguratorManager::setUID(String uid, String signature) {
  bool res = false;
  if (_statusRequest.pending && _statusRequest.key == RequestType::GET_ID) {
    if (_selectedAgent) {
      if (uid.length() > MAX_UID_SIZE) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s UID too long", __FUNCTION__);
        return res;
      }
      if (signature.length() > MAX_SIGNATURE_SIZE) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s Signature too long", __FUNCTION__);
        return res;
      }

      size_t len = CBOR_DATA_UID_LEN;
      uint8_t data[len];

      res = CBORAdapter::uidToCBOR(uid, data, &len);

      if (!res) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s failed to convert uid to CBOR", __FUNCTION__);
        return res;
      }

      res = _selectedAgent->sendData(data, len);
      if (!res) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s failed to send uid", __FUNCTION__);
        return res;
      }

      len = CBOR_DATA_SIGNATURE_LEN;
      uint8_t signatureData[len];

      res = CBORAdapter::signatureToCBOR(signature, signatureData, &len);
      if (!res) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s failed to convert signature to CBOR", __FUNCTION__);
        return res;
      }

      res = _selectedAgent->sendData(signatureData, len);

      if (!res) {
        DEBUG_ERROR("AgentsConfiguratorManager::%s failed to send signature", __FUNCTION__);
        return res;
      }
      res = true;
      _statusRequest.reset();
    }
  }
  return res;
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
  if (_initStatusMsg != MessageTypeCodes::NONE) {
    if (!sendStatus(_initStatusMsg)) {
      DEBUG_WARNING("AgentsConfiguratorManager::%s failed to send initial status", __FUNCTION__);
      return nextState;
    }
    _initStatusMsg = MessageTypeCodes::NONE;
  }
  nextState = AgentsConfiguratorManagerStates::SEND_NETWORK_OPTIONS;
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleSendNetworkOptions() {
  AgentsConfiguratorManagerStates nextState = _state;
  if (sendNetworkOptions()) {
    nextState = AgentsConfiguratorManagerStates::CONFIG_IN_PROGRESS;
  }
  return nextState;
}

AgentsConfiguratorManagerStates AgentsConfiguratorManager::handleConfInProgress() {
  AgentsConfiguratorManagerStates nextState = _state;

  ConfiguratorAgent::AgentConfiguratorStates agentConfState = _selectedAgent->poll();
  switch (agentConfState) {
    case ConfiguratorAgent::AgentConfiguratorStates::RECEIVED_DATA:
      handleReceivedData();
      break;
    case ConfiguratorAgent::AgentConfiguratorStates::INIT:
      handlePeerDisconnected();
      nextState = AgentsConfiguratorManagerStates::INIT;
      break;
  }

  return nextState;
}

void AgentsConfiguratorManager::handleReceivedCommands(RemoteCommands cmd) {
  switch (cmd) {
    case RemoteCommands::CONNECT: handleConnectCommand(); break;
    case RemoteCommands::SCAN: handleUpdateOptCommand(); break;
    case RemoteCommands::GET_ID: handleGetUIDCommand(); break;
  }
}

void AgentsConfiguratorManager::handleReceivedData() {
  if (!_selectedAgent->receivedDataAvailable()) {
    return;
  }

  size_t len = _selectedAgent->getReceivedDataLength();
  uint8_t data[len];
  if (!_selectedAgent->getReceivedData(data, &len)) {
    DEBUG_WARNING("AgentsConfiguratorManager::%s failed to get received data", __FUNCTION__);
    return;
  }

  ProvisioningCommandDown msg;
  if (!CBORAdapter::getMsgFromCBOR(data, len, &msg)) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s Invalid message", __FUNCTION__);
    sendStatus(MessageTypeCodes::INVALID_PARAMS);
    return;
  }

  if (msg.c.id == CommandId::ProvisioningTimestamp) {
    uint64_t ts;
    if (!CBORAdapter::extractTimestamp(&msg, &ts)) {
      DEBUG_DEBUG("AgentsConfiguratorManager::%s Invalid timestamp", __FUNCTION__);
      sendStatus(MessageTypeCodes::INVALID_PARAMS);
      return;
    }

    if (_returnTimestampCb != nullptr) {
      _returnTimestampCb(ts);
    }

  } else if (msg.c.id == CommandId::ProvisioningCommands) {
    RemoteCommands cmd;
    if (CBORAdapter::extractCommand(&msg, &cmd)) {
      handleReceivedCommands(cmd);
    }
  } else {
    models::NetworkSetting netSetting;
    if (!CBORAdapter::extractNetworkSetting(&msg, &netSetting)) {
      DEBUG_DEBUG("AgentsConfiguratorManager::%s Invalid network Setting", __FUNCTION__);
      sendStatus(MessageTypeCodes::INVALID_PARAMS);
      return;
    }
    if (_returnNetworkSettingsCb != nullptr) {
      _returnNetworkSettingsCb(&netSetting);
    }
  }
}

void AgentsConfiguratorManager::handleConnectCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a Connect request while executing another request", __FUNCTION__);
    sendStatus(MessageTypeCodes::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::CONNECT;
  callHandler(RequestType::CONNECT);
}

void AgentsConfiguratorManager::handleUpdateOptCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a UpdateConnectivityOptions request while executing another request", __FUNCTION__);
    sendStatus(MessageTypeCodes::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::SCAN;
  callHandler(RequestType::SCAN);
}

void AgentsConfiguratorManager::handleGetUIDCommand() {
  if (_statusRequest.pending) {
    DEBUG_DEBUG("AgentsConfiguratorManager::%s received a GetUnique request while executing another request", __FUNCTION__);
    sendStatus(MessageTypeCodes::OTHER_REQUEST_IN_EXECUTION);
    return;
  }

  _statusRequest.pending = true;
  _statusRequest.key = RequestType::GET_ID;
  callHandler(RequestType::GET_ID);
}

size_t AgentsConfiguratorManager::computeOptionsToSendLength() {
  size_t length = CBOR_DATA_HEADER_LEN;

  if (_netOptions.type == NetworkOptionsClass::WIFI) {
    for (uint8_t i = 0; i < _netOptions.option.wifi.numDiscoveredWiFiNetworks; i++) {
      length += 4;  //for RSSI and text identifier
      length += _netOptions.option.wifi.discoveredWifiNetworks[i].SSIDsize;
    }
  }

  return length;
}

bool AgentsConfiguratorManager::sendNetworkOptions() {
  size_t len = computeOptionsToSendLength();
  uint8_t data[len];
  if (!CBORAdapter::networkOptionsToCBOR(_netOptions, data, &len)) {
    return false;
  }

  bool res = _selectedAgent->sendData(data, len);
  if (!res) {
    DEBUG_WARNING("AgentsConfiguratorManager::%s failed to send network options", __FUNCTION__);
  }

  return res;
}

bool AgentsConfiguratorManager::sendStatus(StatusMessage msg) {
  bool res = false;
  size_t len = CBOR_DATA_STATUS_LEN;
  uint8_t data[len];
  res = CBORAdapter::statusToCBOR(msg, data, &len);
  if (!res) {
    DEBUG_ERROR("AgentsConfiguratorManager::%s failed encode status: %d ", __FUNCTION__, (int)msg);
    return res;
  }

  res = _selectedAgent->sendData(data, len);
  if (!res) {
    DEBUG_WARNING("AgentsConfiguratorManager::%s failed to send status: %d ", __FUNCTION__, (int)msg);
  }
  return res;
}

void AgentsConfiguratorManager::handlePeerDisconnected() {
  //Peer disconnected, restore all stopped agents
  for (std::list<ConfiguratorAgent *>::iterator agent = _agentsList.begin(); agent != _agentsList.end(); ++agent) {
    if (*agent != _selectedAgent) {
      (*agent)->begin();
    }
  }
#if defined(ARDUINO_PORTENTA_H7_M7)
  digitalWrite(LED_BUILTIN, HIGH);
#else
  digitalWrite(LED_BUILTIN, LOW);
#endif
  _selectedAgent = nullptr;
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
      err = "Get UID ";
    } else if (type == RequestType::CONNECT) {
      err = "Connect ";
    }

    DEBUG_WARNING("AgentsConfiguratorManager::%s %s request received, but handler function is not provided", __FUNCTION__, err.c_str());
    _statusRequest.reset();
    sendStatus(MessageTypeCodes::INVALID_REQUEST);
  }
}

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
void AgentsConfiguratorManager::stopBLEAgent() {
  std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
    if (agent->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsConfiguratorManager::%s End ble agent for wifi", __FUNCTION__);
      agent->end();
    }
  });
}

void AgentsConfiguratorManager::startBLEAgent() {
  std::for_each(_agentsList.begin(), _agentsList.end(), [](ConfiguratorAgent *agent) {
    if (agent->getAgentType() == ConfiguratorAgent::AgentTypes::BLE) {
      DEBUG_VERBOSE("AgentsConfiguratorManager::%s Restart ble agent after wifi use", __FUNCTION__);
      agent->begin();
    }
  });
}
#endif

AgentsConfiguratorManager ConfiguratorManager;
