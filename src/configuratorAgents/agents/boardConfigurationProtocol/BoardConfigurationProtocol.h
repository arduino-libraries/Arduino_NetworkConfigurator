/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "PacketManager.h"
#include "configuratorAgents/MessagesDefinitions.h"

/**
 * @class BoardConfigurationProtocol
 * @brief Abstract class that implements the ArduinoBoardConfiguration Protocol for board
 * network configuration and provisionig.
 * This class must be inherited by the concrete implementation of the virtual protected methods
 * that depends on the physical interface.
 */

class BoardConfigurationProtocol {
public:
  /**
   * @brief Retrieves the next available input message.
   * @param msg Reference to a ProvisioningInputMessage object to store the retrieved message.
   * @return True if a message was successfully retrieved, false otherwise.
   */
  bool getMsg(ProvisioningInputMessage &msg);

  /**
   * @brief Sends an output message.
   * @param msg Reference to a ProvisioningOutputMessage object containing the message to send.
   * @return True if the message was successfully sent, false otherwise.
   */
  bool sendMsg(ProvisioningOutputMessage &msg);

  /**
   * @brief Checks if there are any messages available to process.
   * @return True if messages are available, false otherwise.
   */
  bool msgAvailable();

protected:
  enum class TransmissionResult { INVALID_DATA = -2,
                                  PEER_NOT_AVAILABLE = -1,
                                  NOT_COMPLETED = 0,
                                  COMPLETED = 1,
                                  DATA_RECEIVED = 2 };
  TransmissionResult sendAndReceive();
  bool sendNak();
  bool sendData(PacketManager::MessageType type, const uint8_t *data, size_t len);
  void clear();
  void checkOutputPacketValidity();
  /*Pure virtual methods that depends on physical interface*/

  /**
   * @brief Checks if at least a byte has been received.
   * @return True if a byte has been received, false otherwise.
   */
  virtual bool received() = 0;
  /**
   * @brief Get the number of bytes received.
   * @return the number of bytes received.
   */
  virtual size_t available() = 0;
  /**
   * @brief Reads a byte from the physical interface input buffer.
   * @return The read byte.
   */
  virtual uint8_t read() = 0;
  /**
   * @brief Writes data to the physical interface output buffer.
   * @param data Pointer to the data to write.
   * @param len Length of the data to write.
   * @return The number of bytes written.
   */
  virtual int write(const uint8_t *data, size_t len) = 0;
  /**
   * @brief Handles the disconnection request from the peer device.
   */
  virtual void handleDisconnectRequest() = 0;
  /**
   * @brief Checks if the peer device is connected.
   * @return True if the peer device is connected, false otherwise.
   */
  virtual bool isPeerConnected() = 0;
  /**
   * @brief Clears the input buffer of the physical interface.
   */
  virtual void clearInputBuffer() = 0;

private:
  bool sendStatus(StatusMessage msg);
  size_t calculateTotalOptionsLength(const NetworkOptions *netOptions);
  bool sendNetworkOptions(const NetworkOptions *netOptions);
  bool sendUhwid(const byte *uhwid);
  bool sendJwt(const char *jwt, size_t len);
  bool sendProvPublicKey(const char *provPublicKey, size_t len);
  bool sendBleMacAddress(const uint8_t *mac, size_t len);
  bool sendVersion(const char *version, MessageOutputType type);
  TransmissionResult transmitStream();
  void printPacket(const char *label, const uint8_t *data, size_t len);
  std::list<OutputPacketBuffer> _outputMessagesList;
  std::list<InputPacketBuffer> _inputMessagesList;
  PacketManager::Packet_t _packet;
};
