/*
  Copyright (c) 2025 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
 * This sketch shows how to use the NetworkConfigurator library to configure the
 * network connectivity of the board. The library provides a way to configure and update
 * the network settings using different interfaces like BLE, Serial, etc.
 * The NetworkConfigurator library stores the provided network settings in a persistent
 * key-value storage (KVStore) and uses the ConnectionHandler library to connect to the network.
 * At startup the NetworkConfigurator library reads the stored network settings from the storage
 * and loads them into the ConnectionHandler object.
 *
 * The NetworkConfigurator library provides a way for wiping out the stored network settings.
 * Reconfiguration procedure:
 * - Arduino Opta: press and hold the user button (BTN_USER) until the led (LED_USER) turns off
 * - Arduino MKR WiFi 1010: short the pin 7 to GND until the led turns off
 * - Arduino GIGA R1 WiFi: short the pin 7 to GND until the led turns off
 * - Nicla Vision: short the pin PA_13 to GND until the led turns off
 * - Portenta H7: short the pin 0 to GND until the led turns off
 * - Portenta C33: short the pin 0 to GND until the led turns off
 * - Portenta Machine Control: the reset is not available
 * - Other boards: short the pin 2 to GND until the led turns off
 *
 * In this sketch the BLE and Serial interfaces are always enabled and ready for accepting
 * a new set of configuration.
 *
 * Sketch originally added 21 March 2025
 * by Fabio Centonze
 */

#include <Arduino_ConnectionHandler.h>
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

  /* Set the debug message level:
  * - DBG_ERROR: Only show error messages
  * - DBG_WARNING: Show warning and error messages
  * - DBG_INFO: Show info, warning, and error messages
  * - DBG_DEBUG: Show debug, info, warning, and error messages
  * - DBG_VERBOSE: Show all messages
  */
  setDebugMessageLevel(DBG_INFO);

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

  //Uncomment this for disabling the reconfiguration pin and the reconfiguration procedure
  //NetworkConfigurator.setReconfigurePin(DISABLE_PIN);

  /* Uncomment and specify your preferred pin for changing the default reconfiguration pin
  * The pin must be in the list of digital pins usable for interrupts.
  * Please refer to the Arduino documentation for more details:
  * https://docs.arduino.cc/language-reference/en/functions/external-interrupts/attachInterrupt/
  */
  //NetworkConfigurator.setReconfigurePin(your_pin);

  /* Add a custom callback function to be invoked every time the interrupt on reconfiguration pin is fired*/
  NetworkConfigurator.addReconfigurePinCallback(onResetPinInterrupt);
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

void onResetPinInterrupt() {
  //Add your custom code here
  //This function is called when the reconfiguration pin interrupt is fired
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
