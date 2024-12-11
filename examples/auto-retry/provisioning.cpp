#include "provisioning.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
#define PROVISIONING_SERVICEID_FOR_AGENTMANAGER 0xB1
Provisioning::Provisioning(AgentsConfiguratorManager &agc)
  : _agentManager{ &agc } {
}

void Provisioning::begin() {
  if (!_agentManager->addRequestHandler(RequestType::GET_ID, getIdRequestHandler)) {
    Serial.println("error adding get id callback");
  }
  if (!_agentManager->addReturnTimestampCallback(setTimestamp)) {
    Serial.println("error adding returntimestamp callback");
  }
  _agentManager->begin(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = true;
  _reqCompleted = false;
}

void Provisioning::end() {
  _agentManager->removeReturnTimestampCallback();
  _agentManager->removeRequestHandler(RequestType::GET_ID);
  _agentManager->end(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = false;
}

bool Provisioning::poll() {
  _agentManager->poll();
  if (_reqReceived) {
    _reqReceived = false;

    if (_ts != 0) {

      _agentManager->setID("identifier1212123210111213141516", "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJjODcxODE3ZTRkN2VhY2QxOGI5NGYwYWE4YjAwNDZkZDE5Zjg0MGEwMDgzMzZiNTA5YWJmOTkwYjQ3ZTNiMTg3IiwiaWF0IjoxNzI4NDc1MTA4LCJleHAiOjE3Mjg0Nzg3MDh9.-f_UodDftALaC27UXj3lmp8SCWXCpHLBYl70Pl6DbHzb4lc-GqLYt2I_g_pquiNPym2YApUPL_7yqO-NfWuQHQ");
      _reqCompleted = true;
    } else {
      Serial.println("Error: timestamp not provided");
      _agentManager->setStatusMessage(MessageTypeCodes::PARAMS_NOT_FOUND);
    }
  }
  return _reqCompleted;
}

void Provisioning::getIdRequestHandler() {
  Serial.println("Request getId received");
  _reqReceived = true;
}
void Provisioning::setTimestamp(uint64_t ts) {
  _ts = ts;
  Serial.print("received TS: ");
  Serial.println(_ts);
}