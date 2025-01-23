// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <GenericConnectionHandler.h>
#include "NetworkConfigurator.h"
#include "ConfiguratorAgents/AgentsConfiguratorManager.h"
#include "ConfiguratorAgents/agents/BLE/BLEConfiguratorAgent.h"
#include "ConfiguratorAgents/agents/Serial/SerialAgent.h"
#include "ClaimingHandler.h"

void onCounterChange();

int counter;

void initProperties() {

  ArduinoCloud.addProperty(counter, READWRITE, ON_CHANGE, onCounterChange);
  ConfiguratorManager.addAgent(BLEAgent);
  ConfiguratorManager.addAgent(SerialAgent);
}

GenericConnectionHandler ArduinoIoTPreferredConnection;
NetworkConfigurator NetworkConf(ConfiguratorManager, ArduinoIoTPreferredConnection);
ClaimingHandlerClass ClaimingHandler(ConfiguratorManager);
