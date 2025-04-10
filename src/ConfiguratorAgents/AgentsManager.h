/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "Arduino.h"
#include "agents/ConfiguratorAgent.h"
#include "MessagesDefinitions.h"

/**
 * @enum AgentsManagerStates
 * @brief Represents the various states of the AgentsManager.
 *
 * States:
 * - INIT:
 *   The initial state where the AgentsManager is polling all the handled
 *   agents waiting for a connected user client.
 *
 * - SEND_INITIAL_STATUS:
 *   In this state, the AgentsManager sends an initial status message
 *   to inform about errors or current state.
 *
 * - SEND_NETWORK_OPTIONS:
 *   This state is responsible for sending network configuration options
 *   to the connected agent. These options may include the list of available WiFi Networks
 *
 * - CONFIG_IN_PROGRESS:
 *   Indicates that a configuration process is currently in progress.
 *   The AgentsManager is actively handling commands or data related to
 *   network configuration orprovisioning.
 *
 * - END:
 *   The final state where the AgentsManager concludes its operations.
 *   This may involve cleaning up resources and stopping agents
 */
enum class AgentsManagerStates { INIT,
                                SEND_INITIAL_STATUS,
                                SEND_NETWORK_OPTIONS,
                                CONFIG_IN_PROGRESS,
                                END };

typedef void (*ConfiguratorRequestHandler)();
typedef void (*ReturnTimestamp)(uint64_t ts);
typedef void (*ReturnNetworkSettings)(models::NetworkSetting *netSetting);

enum class RequestType: int { NONE = -1,
                              CONNECT = 0,
                              SCAN = 1,
                              GET_ID  = 2,
                              RESET = 3,
                              GET_WIFI_FW_VERSION = 4,
                              GET_BLE_MAC_ADDRESS = 5,
                              GET_PROVISIONING_SKETCH_VERSION = 6,
                              GET_NETCONFIG_LIB_VERSION = 7};

/**
 * @class AgentsManagerClass
 * @brief Manages the lifecycle, communication, and state of multiple configurator agents.
 *
 * The AgentsManagerClass is a singleton class responsible for coordinating multiple
 * configurator agents. It provides methods to initialize, terminate, enable, disable,
 * and interact with agents. The agents handle a communication interface for configuring
 * network credentials and handle the claiming cloud process. The interfaces could be: BLE,
 * Serial and other.
 * The class also handles communication with agents, manages
 * their states, and provides callback mechanisms for handling requests and returning
 * data such as timestamps and network settings.
 *
 * Key functionalities include:
 * - Managing the state of the agents.
 * - Sending and receiving messages to/from agents.
 * - Adding and removing agents dynamically.
 * - Handling requests and callbacks for specific operations.
 * - Monitoring the progress of the execution of received commands.
 *
 */
class AgentsManagerClass {
public:
  /**
   * @brief Get the singleton instance of the AgentsManagerClass.
   * @return Reference to the singleton instance.
   */
  static AgentsManagerClass &getInstance();

  /**
   * @brief Initialize the AgentsManager, and starts the agents.
   * @return True if initialization is successful, false otherwise.
   */
  bool begin();

  /**
   * @brief Terminate the AgentsManager.
   * @return True if termination is successful, false otherwise.
   */
  bool end();

  /**
   * @brief Disconnect the currently connected agent.
   */
  void disconnect();

  /**
   * @brief Update the state of the AgentsManager.
   * @return The current state of the AgentsManager.
   */
  AgentsManagerStates update();

  /**
   * @brief Enable or disable a specific agent.
   * The agent will be automatically started or stopped
   * based on the enable parameter.
   * @param type The type of agent to enable or disable.
   * @param enable True to enable, false to disable.
   */
  void enableAgent(ConfiguratorAgent::AgentTypes type, bool enable);

  /**
   * @brief Check if a specific agent is enabled.
   * @param type The type of agent to check.
   * @return True if the agent is enabled, false otherwise.
   */
  bool isAgentEnabled(ConfiguratorAgent::AgentTypes type);

  /**
   * @brief Force start an agent even if it is disabled.
   * @param type The type of agent to start.
   * @return True if the agent is found in the list, false otherwise.
   */
  bool startAgent(ConfiguratorAgent::AgentTypes type);

  /**
   * @brief Stop a specific agent.
   * @param type The type of agent to stop.
   * @return True if the agent is found in the list, false otherwise.
   */
  bool stopAgent(ConfiguratorAgent::AgentTypes type);

  /**
   * @brief Get the currently connected agent.
   * @return Pointer to the connected agent, or nullptr if no agent is connected.
   */
  ConfiguratorAgent *getConnectedAgent();

  /**
   * @brief Send a message to the connected agent or queue a message
   * if no agent is connected.
   * @param msg The message to send.
   * @return True if the message is successfully sent, false otherwise.
   */
  bool sendMsg(ProvisioningOutputMessage &msg);

  /**
   * @brief Add an agent to be managed to the list.
   * @param agent The agent to add.
   * @return True if the agent is successfully added, false otherwise.
   */
  bool addAgent(ConfiguratorAgent &agent);

  /**
   * @brief Add an handler callback for a specific request type.
   * The callback is fired when the properly request is received
   * @param type The type of request to handle.
   * @param callback The callback function to handle the request.
   * @return True if the handler is successfully added, false otherwise.
   */
  bool addRequestHandler(RequestType type, ConfiguratorRequestHandler callback);

  /**
   * @brief Remove the handler callback for a specific request type.
   * @param type The type of request to remove the handler for.
   */
  void removeRequestHandler(RequestType type);

  /**
   * @brief Add a callback to be fired when the timestamp is received from the agent.
   * @param callback The callback function to return the timestamp.
   * @return True if the callback is successfully added, false otherwise.
   */
  bool addReturnTimestampCallback(ReturnTimestamp callback);

  /**
   * @brief Remove the callback for returning a timestamp.
   */
  void removeReturnTimestampCallback();

  /**
   * @brief Add a callback to return network settings received from the agent.
   * @param callback The callback function to return the network settings.
   * @return True if the callback is successfully added, false otherwise.
   */
  bool addReturnNetworkSettingsCallback(ReturnNetworkSettings callback);

  /**
   * @brief Remove the callback for returning network settings.
   */
  void removeReturnNetworkSettingsCallback();

  /**
   * @brief Check if a configuration process is in progress.
   * @return True if a configuration process is in progress, false otherwise.
   */
  bool isConfigInProgress();

private:
  AgentsManagerClass();
  AgentsManagerStates _state;
  std::list<ConfiguratorAgent *> _agentsList;
  bool _enabledAgents[2];
  ConfiguratorRequestHandler _reqHandlers[8];
  ReturnTimestamp _returnTimestampCb;
  ReturnNetworkSettings _returnNetworkSettingsCb;
  ConfiguratorAgent *_selectedAgent;
  uint8_t _instances;
  StatusMessage _initStatusMsg;
  NetworkOptions _netOptions;
  typedef struct {
    void reset() {
      pending = false;
      key = RequestType::NONE;
      completion = 0;
    };
    uint8_t completion;
    bool pending;
    RequestType key;
  } StatusRequest;

  StatusRequest _statusRequest;

  AgentsManagerStates handleInit();
  AgentsManagerStates handleSendInitialStatus();
  AgentsManagerStates handleSendNetworkOptions();
  AgentsManagerStates handleConfInProgress();
  void updateProgressRequest(StatusMessage type);
  void updateProgressRequest(MessageOutputType type);
  void handleReceivedCommands(RemoteCommands cmd);
  void handleReceivedData();

  bool sendStatus(StatusMessage msg);

  void handlePeerDisconnected();
};
