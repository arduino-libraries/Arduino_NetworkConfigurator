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

      _agentManager->setUID("identifier1212123210111213141516", "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiIxMjM0NTY3ODEyMzQ1Njc4MTIzNDU2NzgxMjM0NTY3ODEyMzQ1Njc4MTIzNDU2NzgxMjM0NTY3ODEyMzQ1Njc4IiwiaWF0IjoxNTE2MjM5MDIyfQ.ZrJ_zIb_4DYbV9b1rTeMbNnVPHnx7UBLZ4pPSnh8ggWm5QQzGXK6BuyL_zfJHbNWNHhLp4C3QjTCu0kx0CcVeg");
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