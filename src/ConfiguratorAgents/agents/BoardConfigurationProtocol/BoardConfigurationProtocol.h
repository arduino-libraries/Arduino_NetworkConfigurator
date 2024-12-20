/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include <list>
#include "PacketManager.h"
#include "ConfiguratorAgents/agents/ConfiguratorAgent.h"
#include "ConfiguratorAgents/MessagesDefinitions.h"

class BoardConfigurationProtocol : public ConfiguratorAgent {
public:
  virtual bool getReceivedMsg(ProvisioningInputMessage &msg);
  virtual bool sendMsg(ProvisioningOutputMessage &msg);
  virtual bool receivedMsgAvailable();

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
  virtual bool hasReceivedBytes() = 0;
  virtual size_t receivedBytes() = 0;
  virtual uint8_t readByte() = 0;
  virtual int writeBytes(const uint8_t *data, size_t len) = 0;
  virtual void handleDisconnectRequest() = 0;

private:
  bool sendStatus(StatusMessage msg);
  size_t calculateTotalOptionsLength(const NetworkOptions *netOptions);
  bool sendNetworkOptions(const NetworkOptions *netOptions);
  bool sendUhwid(const char *uhwid, size_t len);
  bool sendJwt(const char *jwt, size_t len);
  bool sendBleMacAddress(const uint8_t *mac, size_t len);
  TransmissionResult transmitStream();
  void printPacket(const char *label, const uint8_t *data, size_t len);
  std::list<OutputPacketBuffer> _outputMessagesList;
  std::list<InputPacketBuffer> _inputMessagesList;
};
