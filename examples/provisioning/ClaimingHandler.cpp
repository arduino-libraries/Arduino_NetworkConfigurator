/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ClaimingHandler.h"
#include "SecretsHelper.h"
#include "Arduino_DebugUtils.h"
#define PROVISIONING_SERVICEID_FOR_AGENTMANAGER 0xB1

ClaimingHandlerClass::ClaimingHandlerClass(AgentsManagerClass &agc) {
  _agentManager = &agc;
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
  _agentManager->poll();

  switch (_receivedEvent) {
    case ClaimingReqEvents::GET_ID: getIdReqHandler          (); break;
    case ClaimingReqEvents::RESET:  resetStoredCredReqHandler(); break;
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
      return;
    }

    String token = CreateJWTToken(*_uhwid, _ts, _secureElement);
    Serial.print("Token: ");//TODO REMOVE
    Serial.println(token);
    if (token == "") {
      DEBUG_ERROR("ClaimingHandlerClass::%s Error: token not created", __FUNCTION__);
      sendStatus(StatusMessage::ERROR);
      return;
    }
    byte _uhwidBytes[33];
    hexStringToBytes(*_uhwid, _uhwidBytes, _uhwid->length());
    _uhwidBytes[32] = '\0';
    //Send UHWID
    ProvisioningOutputMessage idMsg = {MessageOutputType::UHWID};
    idMsg.m.uhwid = (char *)_uhwidBytes;
    _agentManager->sendMsg(idMsg);
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
  } else {
    sendStatus(StatusMessage::RESET_COMPLETED);
  }

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

bool ClaimingHandlerClass::sendStatus(StatusMessage msg) {
  ProvisioningOutputMessage statusMsg = { MessageOutputType::STATUS, { msg } };
  return _agentManager->sendMsg(statusMsg);
}