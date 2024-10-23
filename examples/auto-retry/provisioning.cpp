#include "provisioning.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"
#define PROVISIONING_SERVICEID_FOR_AGENTMANAGER 0xB1
Provisioning::Provisioning(AgentsConfiguratorManager &agc): _agentManager{&agc}{
 
}

void Provisioning::begin(){
  if(!_agentManager->addRequestHandler(RequestType::GET_ID, getIdRequestHandler)){
    Serial.println("error adding get id callback");
  }
  if(!_agentManager->addReturnTimestampCallback(setTimestamp)){
    Serial.println("error adding returntimestamp callback");
  }
  _agentManager->begin(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = true;
  _reqCompleted = false;
}

void Provisioning::end(){
  _agentManager->removeReturnTimestampCallback();
  _agentManager->removeRequestHandler(RequestType::GET_ID);
  _agentManager->end(PROVISIONING_SERVICEID_FOR_AGENTMANAGER);
  _agentInitialized = false;
}

bool Provisioning::poll(){
  _agentManager->poll();
  if(_reqReceived){
    _reqReceived = false;
    
    if(_ts != 0){

      _agentManager->setUID("identifier1212123210111213141516","01020304050607080910111213141516");
      _reqCompleted = true;
    }else{
      Serial.println("Error: timestamp not provided");
      _agentManager->setStatusMessage(MessageTypeCodes::PARAMS_NOT_FOUND);
    }
  }
  return _reqCompleted;
}

void Provisioning::getIdRequestHandler(){
  Serial.println("Request getId received");
  _reqReceived = true;
}
void Provisioning::setTimestamp(uint64_t ts){
  _ts = ts; 
  Serial.print("received TS: ");
  Serial.println(_ts);
}