/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include "Arduino.h"
#include "GenericConnectionHandler.h"
#include "ConfiguratorAgents/AgentsManager.h"
#include <settings/settings.h>
#include <Arduino_TimedAttempt.h>
#include <Arduino_KVStore.h>
#include "Utility/ResetInput/ResetInput.h"

enum class NetworkConfiguratorStates { CHECK_ETH, //TODO rename 
                                       READ_STORED_CONFIG,
                                       WAITING_FOR_CONFIG,
                                       CONNECTING,
                                       CONFIGURED,
                                       UPDATING_CONFIG,
                                       ERROR,
                                       END };

/**
 * @brief Class responsible for managing the network configuration of an Arduino_ConnectionHandler instance.
 * The library provides a way to configure and update
 * the network settings using different interfaces like BLE, Serial, etc.
 * The NetworkConfigurator library stores the provided network settings in a persistent
 * key-value storage (KVStore) and uses the ConnectionHandler library to connect to the network.
 * At startup the NetworkConfigurator library reads the stored network settings from the storage
 * and loads them into the ConnectionHandler object.
 *
 * The NetworkConfigurator library provides a way for wiping out the stored network settings and forcing
 * the restart of the BLE interface if turned off.
 * Reconfiguration procedure:
 * - Arduino Opta: press and hold the user button (BTN_USER) until the led (LED_USER) turns off
 * - Arduino Nano 33 IOT: short the pin 2 to GND until the led turns off
 * - Arduino Uno R4 WiFi: short the pin 2 to GND until the led turns off
 * - Arduino Nano RP2040 Connect: short the pin 2 to 3.3V until the led turns off
 * - Other boards: short the pin 7 to GND until the led turns off
 *
 */
class NetworkConfiguratorClass {
public:
  /**
   * @brief Constructor for the NetworkConfiguratorClass.
   * @param connectionHandler Reference to a ConnectionHandler object.
   */
  NetworkConfiguratorClass(ConnectionHandler &connectionHandler);

  /**
   * @brief Initializes the network configurator and starts the interfaces
   * for configuration.
   * @return True if initialization is successful, false otherwise.
   */
  bool begin();

  /**
   * @brief Polls the current state of the network configurator.
   * @return The current state as a NetworkConfiguratorStates enum.
   */
  NetworkConfiguratorStates poll();//TODO rename "update"

  /**
   * @brief Resets the stored network configuration.
   * This method is alternative to the reconfiguration procedure
   * without forcing the restart of the BLE interface if turned off.
   * @return True if the reset is successful, false otherwise.
   */
  bool resetStoredConfiguration();

  /**
   * @brief Ends the network configurator session and stops the
   * interfaces for configuration.
   * @return True if the session ends successfully, false otherwise.
   */
  bool end();

  /**
   * @brief Scans for available network options.
   * @return True if the scan is successful, false otherwise.
   */
  bool scanNetworkOptions();

  /**
   * @brief Sets the storage backend for network configuration.
   * @param kvstore Reference to a KVStore object.
   */
  void setStorage(KVStore &kvstore);

  /**
   * @brief Sets the pin used for the reconfiguration procedure.
   * It must be set before calling the begin() method.
   * @param pin The pin number to be used for reconfiguration,
   * internally it's mapped to an interrupt with INPUT_PULLUP mode.
   */
  void setReconfigurePin(uint32_t pin);

  /**
   * @brief Adds a callback function to be triggered every time the
   * interrupt on the reconfiguration pin is fired.
   * @param callback Pointer to the callback function.
   */
  //TODO add to example
  void addReconfigurePinCallback(void (*callback)());//TODO add version with std:function https://github.com/arduino-libraries/ArduinoMqttClient/blob/0a0706262ad56043954b008a7f89adf3b783ec88/src/MqttClient.h#L38

  /**
   * @brief Checks if BLE (Bluetooth Low Energy) is enabled.
   * @return True if BLE is enabled, false otherwise.
   */
  bool isBLEenabled();

  /**
   * @brief Enables or disables BLE (Bluetooth Low Energy).
   * @param enable True to enable BLE, false to disable it.
   */
  void enableBLE(bool enable);

  /**
   * @brief Disconnects the current agent from the peer.
   */
  void disconnectAgent();//TODO think about a method for returning the agent and call its disconnect

  /**
   * @brief Adds a new configurator agent.
   * Configurator agents handle the configuration interfaces BLE, Serial, etc.
   * This method should be called before the begin() method.
   * @param agent Reference to a ConfiguratorAgent object.
   * @return True if the agent is added successfully, false otherwise.
   */
  bool addAgent(ConfiguratorAgent &agent);

private:
  NetworkConfiguratorStates _state;
  ConnectionHandler *_connectionHandler;
  static inline models::NetworkSetting _networkSetting;
  bool _connectionHandlerIstantiated;
  ResetInput *_resetInput;
  bool _bleEnabled;
  TimedAttempt _connectionTimeout;
  TimedAttempt _connectionRetryTimer;
  TimedAttempt _optionUpdateTimer;

  enum class NetworkConfiguratorEvents { NONE,
                                         SCAN_REQ,
                                         CONNECT_REQ,
                                         NEW_NETWORK_SETTINGS,
                                         GET_WIFI_FW_VERSION };
  static inline NetworkConfiguratorEvents _receivedEvent;

  enum class ConnectionResult { SUCCESS,
                                FAILED,
                                IN_PROGRESS };

  KVStore *_kvstore;
  AgentsManagerClass *_agentsManager;

#ifdef BOARD_HAS_ETHERNET
  NetworkConfiguratorStates handleCheckEth(); //TODO rename
#endif
  NetworkConfiguratorStates handleReadStorage();
  NetworkConfiguratorStates handleWaitingForConf();
  NetworkConfiguratorStates handleConnecting();
  NetworkConfiguratorStates handleConfigured();
  NetworkConfiguratorStates handleUpdatingConfig();
  NetworkConfiguratorStates handleErrorState();
  bool handleConnectRequest();
  void handleGetWiFiFWVersion();

  void startReconfigureProcedure();

  String decodeConnectionErrorMessage(NetworkConnectionState err, StatusMessage *errorCode);
  ConnectionResult connectToNetwork(StatusMessage *err);
  ConnectionResult disconnectFromNetwork();
  bool sendStatus(StatusMessage msg);
  static void printNetworkSettings();
#ifdef BOARD_HAS_WIFI
  bool scanWiFiNetworks(WiFiOption &wifiOptObj);
  bool insertWiFiAP(WiFiOption &wifiOptObj, char *ssid, int rssi);
#endif
  static void scanReqHandler();
  static void connectReqHandler();
  static void setNetworkSettingsHandler(models::NetworkSetting *netSetting);
  static void getWiFiFWVersionHandler();
};

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
