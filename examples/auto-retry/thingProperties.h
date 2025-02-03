// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <GenericConnectionHandler.h>
#include "NetworkConfigurator.h"
#include "ConfiguratorAgents/AgentsManager.h"
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
#include "ConfiguratorAgents/agents/BLE/BLEAgent.h"
#endif
#include "ConfiguratorAgents/agents/Serial/SerialAgent.h"
#include "provisioning.h"

const char SSID[] = SECRET_SSID;           // Network SSID (name)
const char PASS[] = SECRET_OPTIONAL_PASS;  // Network password (use for WPA, or use as key for WEP)

void onCounterChange();

int counter;
GenericConnectionHandler ArduinoIoTPreferredConnection;
NetworkConfiguratorClass NetworkConfigurator(ArduinoIoTPreferredConnection);
Provisioning ProvisioningSystem(AgentsManager);
KVStore kvstore;
void initProperties() {

  ArduinoCloud.addProperty(counter, READWRITE, ON_CHANGE, onCounterChange);
  NetworkConfigurator.setStorage(kvstore);
#if !defined(ARDUINO_SAMD_MKRGSM1400) && !defined(ARDUINO_SAMD_MKRNB1500) && !defined(ARDUINO_SAMD_MKRWAN1300) && !defined(ARDUINO_SAMD_MKRWAN1310)
  AgentsManager.addAgent(BLEAgent);
#endif
  AgentsManager.addAgent(SerialAgent);
}
