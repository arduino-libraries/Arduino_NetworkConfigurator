/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "configuratorAgents/NetworkOptionsDefinitions.h"
#include "configuratorAgents/MessagesDefinitions.h"
#include "Arduino.h"

/**
 * @brief Abstract base class for Configurator Agents.
 *
 * This class defines the interface for agents that handle configuration
 * tasks over different communication channels (e.g., BLE, USB Serial...).
 */
class ConfiguratorAgent {
public:
  /**
   * @brief States of the Configurator Agent.
   */
  enum class AgentConfiguratorStates {
    INIT,           /**< Initial state. Wait for a connection.*/
    PEER_CONNECTED, /**< Peer device is connected. */
    RECEIVED_DATA,  /**< Data has been received. */
    END,            /**< Configurator Agents is ended. */
    ERROR           /**< An error occurred. */
  };

  /**
   * @brief Types of Configurator Agents.
   */
  enum class AgentTypes {
    BLE,        /**< Bluetooth Low Energy agent. */
    USB_SERIAL, /**< USB Serial agent. */
  };

  virtual ~ConfiguratorAgent() {}

  /**
   * @brief Initialize and start the Configurator Agent.
   * @return The current state of the agent after initialization.
   */
  virtual AgentConfiguratorStates begin() = 0;

  /**
   * @brief End the Configurator Agent.
   * @return The current state of the agent after finalization.
   */
  virtual AgentConfiguratorStates end() = 0;

  /**
   * @brief Update the state of the Configurator Agent.
   *
   * This method should be called periodically to process events.
   *
   * @return The current state of the agent after the update.
   */
  virtual AgentConfiguratorStates update() = 0;

  /**
   * @brief Disconnect the connected peer device.
   */
  virtual void disconnectPeer() = 0;

  /**
   * @brief Check if a received message is available.
   * @return True if a message is available, false otherwise.
   */
  virtual bool receivedMsgAvailable() = 0;

  /**
   * @brief Retrieve the received message.
   * @param msg Reference to a ProvisioningInputMessage object to store the message.
   * @return True if the message was successfully retrieved, false otherwise.
   */
  virtual bool getReceivedMsg(ProvisioningInputMessage &msg) = 0;

  /**
   * @brief Send a message to the peer device.
   * @param msg Reference to a ProvisioningOutputMessage object containing the message to send.
   * @return True if the message was successfully sent, false otherwise.
   */
  virtual bool sendMsg(ProvisioningOutputMessage &msg) = 0;

  /**
   * @brief Check if a peer device is connected.
   * @return True if a peer device is connected, false otherwise.
   */
  virtual bool isPeerConnected() = 0;

  /**
   * @brief Get the type of the Configurator Agent.
   * @return The type of the agent (e.g., BLE, USB Serial).
   */
  virtual AgentTypes getAgentType() = 0;
};
