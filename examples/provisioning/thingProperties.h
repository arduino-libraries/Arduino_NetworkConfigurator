// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <GenericConnectionHandler.h>
#include <Arduino_KVStore.h>
#include "NetworkConfigurator.h"
#include "ConfiguratorAgents/agents/BLE/BLEAgent.h"
#include "ConfiguratorAgents/agents/Serial/SerialAgent.h"
#include "ClaimingHandler.h"

void onCounterChange();

int counter;
KVStore kvstore;
GenericConnectionHandler ArduinoIoTPreferredConnection;
BLEAgentClass BLEAgent;
SerialAgentClass SerialAgent;
NetworkConfiguratorClass NetworkConfigurator(ArduinoIoTPreferredConnection);
ClaimingHandlerClass ClaimingHandler;

void initProperties() {

  ArduinoCloud.addProperty(counter, READWRITE, ON_CHANGE, onCounterChange);
  AgentsManagerClass::getInstance().addAgent(BLEAgent);
  AgentsManagerClass::getInstance().addAgent(SerialAgent);
  NetworkConfigurator.setStorage(kvstore);
}


