/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ClaimingHandler.h"
#include "SecretsHelper.h"
#include "Arduino_DebugUtils.h"
#include "Utility/LEDFeedback/LEDFeedback.h"
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
#define BOARD_HAS_BLE
#endif
#ifdef BOARD_HAS_BLE
#include <ArduinoBLE.h>
#include "utility/HCI.h"
#endif

#define PROVISIONING_SERVICEID_FOR_AGENTMANAGER 0xB1

ClaimingHandlerClass::ClaimingHandlerClass() {
  _agentManager = &AgentsManagerClass::getInstance();
}

bool ClaimingHandlerClass::begin(SecureElement *secureElement, String *uhwid, ClearStoredCredentialsHandler clearStoredCredentials) {
  if(_state != ClaimingHandlerStates::END) {
    return false;
  }

  if (!_agentManager->addRequestHandler(RequestType::GET_ID, getIdRequestCb)) {
    return false;
  }

  if (!_agentManager->addRequestHandler(RequestType::RESET, resetStoredCredRequestCb)) {
    return false;
  }

  if(!_agentManager->addRequestHandler(RequestType::GET_BLE_MAC_ADDRESS, getBLEMacAddressRequestCb)) {
    return false;
  }

  if (!_agentManager->addReturnTimestampCallback(setTimestamp)) {
    return false;
  }
  _agentManager->begin(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _uhwid = uhwid;
  _secureElement = secureElement;
  _clearStoredCredentials = clearStoredCredentials;
  _state = ClaimingHandlerStates::INIT;
}

void ClaimingHandlerClass::end() {
  _agentManager->removeReturnTimestampCallback();
  _agentManager->removeRequestHandler(RequestType::GET_ID);
  _agentManager->removeRequestHandler(RequestType::RESET);
  _agentManager->end(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _state = ClaimingHandlerStates::END;
}

void ClaimingHandlerClass::poll() {
  if(_state == ClaimingHandlerStates::END) {
    return;
  }
  LEDFeedbackClass::getInstance().poll();
  _agentManager->poll();

  switch (_receivedEvent) {
    case ClaimingReqEvents::GET_ID:              getIdReqHandler           (); break;
    case ClaimingReqEvents::RESET:               resetStoredCredReqHandler (); break;
    case ClaimingReqEvents::GET_BLE_MAC_ADDRESS: getBLEMacAddressReqHandler(); break;
  }
  _receivedEvent = ClaimingReqEvents::NONE;
  return;
}

void ClaimingHandlerClass::getIdReqHandler() {
  if (_ts != 0) {
    Serial.print("UHWID: "); //TODO REMOVE
    Serial.println(*_uhwid);
    if (*_uhwid == "") {
      DEBUG_ERROR("ClaimingHandlerClass::%s Error: UHWID not found", __FUNCTION__);
      sendStatus(StatusMessage::ERROR);
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
      return;
    }

    byte _uhwidBytes[32];
    hexStringToBytes(*_uhwid, _uhwidBytes, _uhwid->length());
    //Send UHWID
    ProvisioningOutputMessage idMsg = {MessageOutputType::UHWID};
    idMsg.m.uhwid = _uhwidBytes;
    _agentManager->sendMsg(idMsg);

    String token = CreateJWTToken(*_uhwid, _ts, _secureElement);
    Serial.print("Token: ");//TODO REMOVE
    Serial.println(token);
    if (token == "") {
      DEBUG_ERROR("ClaimingHandlerClass::%s Error: token not created", __FUNCTION__);
      sendStatus(StatusMessage::ERROR);
      LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
      return;
    }

    //Send JWT
    ProvisioningOutputMessage jwtMsg = {MessageOutputType::JWT};
    jwtMsg.m.jwt = token.c_str();
    _agentManager->sendMsg(jwtMsg);
    _ts = 0;
  } else {
    DEBUG_DEBUG("ClaimingHandlerClass::%s Error: timestamp not provided" , __FUNCTION__);
    sendStatus(StatusMessage::PARAMS_NOT_FOUND);
  }
}

void ClaimingHandlerClass::resetStoredCredReqHandler() {
  if( _clearStoredCredentials == nullptr) {
    sendStatus(StatusMessage::ERROR);
    return;
  }

  if( !_clearStoredCredentials()){
    DEBUG_ERROR("ClaimingHandlerClass::%s Error: reset stored credentials failed", __FUNCTION__);
    sendStatus(StatusMessage::ERROR);
    LEDFeedbackClass::getInstance().setMode(LEDFeedbackClass::LEDFeedbackMode::ERROR);
  } else {
    sendStatus(StatusMessage::RESET_COMPLETED);
  }

}

void ClaimingHandlerClass::getBLEMacAddressReqHandler() {
  uint8_t mac[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#ifdef BOARD_HAS_BLE
  bool activated = false;
  ConfiguratorAgent * connectedAgent = _agentManager->getConnectedAgent();
  if(!_agentManager->isBLEAgentEnabled() || (connectedAgent != nullptr &&
    connectedAgent->getAgentType() != ConfiguratorAgent::AgentTypes::BLE)) {
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
  _agentManager->sendMsg(outputMsg);
}

void ClaimingHandlerClass::getIdRequestCb() {
  _receivedEvent = ClaimingReqEvents::GET_ID;
}
void ClaimingHandlerClass::setTimestamp(uint64_t ts) {
  _ts = ts;
}

void ClaimingHandlerClass::resetStoredCredRequestCb() {
  DEBUG_DEBUG("ClaimingHandlerClass::%s Reset stored credentials request received", __FUNCTION__);
  _receivedEvent = ClaimingReqEvents::RESET;
}

void ClaimingHandlerClass::getBLEMacAddressRequestCb() {
  _receivedEvent = ClaimingReqEvents::GET_BLE_MAC_ADDRESS;
}

bool ClaimingHandlerClass::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage statusMsg = { MessageOutputType::STATUS, { msg } };
  return _agentManager->sendMsg(statusMsg);
}
