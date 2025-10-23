# Network Configurator

[![Check Arduino status](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/actions/workflows/check-arduino.yml/badge.svg)](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/actions/workflows/check-arduino.yml)
[![Compile Examples](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/workflows/Compile%20Examples/badge.svg)](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/actions?workflow=Compile+Examples)
[![Spell Check](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/workflows/Spell%20Check/badge.svg)](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/actions?workflow=Spell+Check)
[![Unit Tests](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/workflows/Unit%20Tests/badge.svg)](https://github.com/arduino-libraries/Arduino_NetworkConfigurator/actions?workflow=Unit+Tests)

This library enables the *remote* configuration of an [`Arduino_ConnectionHandler`](https://github.com/arduino-libraries/Arduino_ConnectionHandler) instance through the board's physical interfaces like "Bluetooth Low Energy (BLE)", Serial, etc..
The provided network configuration is stored on persistent storage and loaded at the start up.

The library offers mechanisms for wiping out the saved network configuration on the persistent storage.

## Supported boards and configurable connectivity types matrix
|Boards | WiFi | Ethernet | GSM | NB-IoT | CAT-M1 | Cellular | LoRa |
|-------|:----:|:--------:|:---:|:------:|:------:|:--------:|:----:|
|[`UNO R4 WiFi`](https://store.arduino.cc/products/uno-r4-wifi)  | X |   |   |   |   |   |   |
|[`OPTA WiFi`](https://store.arduino.cc/products/opta-wifi)      | X | X |   |   |   |   |   |
|[`Portenta H7`](https://store.arduino.cc/portenta-h7)           | X | X |   |   | X | X |   |
|[`GIGA R1 WiFi`](https://store.arduino.cc/products/giga-r1-wifi)| X |   |   |   |   |   |   |
|[`Nicla Vision`](https://store.arduino.cc/products/nicla-vision)| X |   |   |   |   |   |   |
|[`Portenta C33`](https://store.arduino.cc/products/portenta-c33)| X | X |   |   |   |   |   |
|[`Nano RP2040 Connect`](https://store.arduino.cc/products/arduino-nano-rp2040-connect)| X |   |   |   |   |   |   |
|[`Nano 33 IoT`](https://store.arduino.cc/arduino-nano-33-iot)   | X |   |   |   |   |   |   |
|[`MKR WiFi 1010`](https://store.arduino.cc/arduino-mkr-wifi-1010)| X |   |   |   |   |   |   |

## How to clean up the stored configuration

The `NetworkConfigurator` library provides a way for wiping out the stored network settings and forcing
the restart of the BLE interface if turned off.

The procedure:

* `Arduino Opta`: press and hold the user button (`BTN_USER`) until the led (`LED_USER`) turns off
* `Arduino MKR WiFi 1010`: short the pin 7 to GND until the led turns off
* `Arduino GIGA R1 WiFi`: short the pin 7 to GND until the led turns off
* `Arduino Portenta H7`: short the pin 0 to GND until the led turns off
* `Arduino Portenta C33`: short the pin 0 to GND until the led turns off
* `Nicla Vision`: short the pin PA_13 to GND until the led turns off
* Other boards: short the pin 2 to GND until the led turns off
* `Portenta Machine Control`: currently the reset procedure is not available

### More on the reconfiguration pin
Internally, the pin indicated in the procedure is set as `INPUT_PULLUP` (except for `Arduino Opta` ) and it's attached to an ISR fired on every change of the pin's status.

In order to be notified when the ISR is fired, it's possible to register a callback function using the function `NetworkConfigurator.addReconfigurePinCallback(callback)`. Please take the example as reference.

### Change the reconfiguration pin
In order to change the default pin for resetting the board, it's possible to use the function `NetworkConfigurator.setReconfigurePin(your_pin)` specifying the new pin.
The pin must be in the list of digital pins usable for interrupts. Please refer to the Arduino documentation for more details: https://docs.arduino.cc/language-reference/en/functions/external-interrupts/attachInterrupt/

### Disable the reconfiguration feature
In order to disable the reconfiguration procedure, use this function in the sketch `NetworkConfigurator.setReconfigurePin(DISABLE_PIN)`


## Configurator Agents
The library provides a set of *Configurator Agents* that added as plug-in to the sketch handle the communication between the Arduino Network Configurator and an external client ([*Arduino IoT App*](https://cloud.arduino.cc/iot-remote-app/), Arduino IoT Cloud, [*Arduino Cloud CLI from v0.3.4*](https://github.com/arduino/arduino-cloud-cli)) for configuring the board.

Out-of-the box there are 2 Configurator Agents
* `BLEAgent`: manage the BLE interface
* `SerialAgent`: manage the Serial interface

### Configurator Agents and supported board matrix
|Boards | BLEAgent | SerialAgent |
|-------|:----:|:--------:|
|[`UNO R4 WiFi`](https://store.arduino.cc/products/uno-r4-wifi)  | X | X |
|[`OPTA WiFi`](https://store.arduino.cc/products/opta-wifi)      | X | X |
|[`Portenta H7`](https://store.arduino.cc/portenta-h7)           | X | X |
|[`GIGA R1 WiFi`](https://store.arduino.cc/products/giga-r1-wifi)| X | X |
|[`Nicla Vision`](https://store.arduino.cc/products/nicla-vision)| X | X |
|[`Portenta C33`](https://store.arduino.cc/products/portenta-c33)| X | X |
|[`Nano RP2040 Connect`](https://store.arduino.cc/products/arduino-nano-rp2040-connect)| X | X |
|[`Nano 33 IoT`](https://store.arduino.cc/arduino-nano-33-iot)   | X | X |
|[`MKR WiFi 1010`](https://store.arduino.cc/arduino-mkr-wifi-1010)| X | X |

### How to enable a Configurator Agent
1. Include the *Configurator Agent* header file in the sketch ex.: `#include <configuratorAgents/agents/BLEAgent.h>`
2. Declare a global variable of the *agent* in the sketch ex.: `BLEAgentClass BLEAgent;`
3. Register the *agent* to the `NetworkConfigurator` ex.:`NetworkConfigurator.addAgent(BLEAgent);`

## Storage

The `NetworkConfigurator` needs a *persistent storage* for saving the provided network configuration.

The user must specify a *storage library* in the sketch and pass it to the `NetworkConfigurator` as parameter.

The user can use the [`Arduino_KVStore`](https://github.com/arduino-libraries/Arduino_KVStore) library as storage, otherwise he can implement his own custom storage library.

The custom storage library must be based on a Key-Value storage system and must implement the interface `KVStoreInterface` of the [`Arduino_KVStore`](https://github.com/arduino-libraries/Arduino_KVStore) library

### How to set the storage library
1. Include the header file of the *storage library* ex.: `#include <Arduino_KVStore.h>`
2. Declare the instance of the *storage manager* ex.: `KVStore kvstore;`
3. Register the storage manager instance to the `NetworkConfigurator` ex.: `NetworkConfigurator.setStorage(kvstore);`

## How-to-use
1. Choose and include a *storage library* like [`Arduino_KVStore`](https://github.com/arduino-libraries/Arduino_KVStore)
2. Choose and include the *Configurator Agents*
3. Register the *storage library* and the *Configurator Agents* to the `NetworkConfigurator`

```
#include <GenericConnectionHandler.h>
#include <Arduino_KVStore.h>
#include <Arduino_NetworkConfigurator.h>
#include <configuratorAgents/agents/BLEAgent.h>
#include <configuratorAgents/agents/SerialAgent.h>

KVStore kvstore;
BLEAgentClass BLEAgent;
SerialAgentClass SerialAgent;
GenericConnectionHandler conMan;
NetworkConfiguratorClass NetworkConfigurator(conMan);

void setup() {
  /* Initialize serial debug port and wait up to 5 seconds for port to open */
  Serial.begin(9600);
  for(unsigned long const serialBeginTime = millis(); !Serial && (millis() - serialBeginTime <= 5000); ) { }

  /* Add callbacks to the ConnectionHandler object to get notified of network
  * connection events. */
  conMan.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  conMan.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  conMan.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);

  /* Set the KVStore */
  NetworkConfigurator.setStorage(kvstore);
  /* Add the interfaces that are enabled for configuring the network*/
  NetworkConfigurator.addAgent(BLEAgent);
  NetworkConfigurator.addAgent(SerialAgent);

  /* Start the network configurator */
  NetworkConfigurator.begin();

}

void loop() {
  /*
   * Poll the networkConfigurator and check if connectionHandler is configured.
   * If the ConnectionHandler has been configured, the following code keeps on
   * running connection workflows on our allowing reconnection in case of failure
   * and notification of connect/disconnect event if enabled.
   *
   * NOTE: any use of delay() within the loop or methods called from it will delay
   * the execution of .update() and .check() methods of the NetworkConfigurator and
   * ConnectionHandler objects which might not guarantee the correct functioning
   * of the code.
   */
  if(NetworkConfigurator.update() == NetworkConfiguratorStates::CONFIGURED) {
    conMan.check();
  }
}

void onNetworkConnect() {
  Serial.println(">>>> CONNECTED to network");
}

void onNetworkDisconnect() {
  Serial.println(">>>> DISCONNECTED from network");
}

void onNetworkError() {
  Serial.println(">>>> ERROR");
}
```

View the full example in the `example` folder.
