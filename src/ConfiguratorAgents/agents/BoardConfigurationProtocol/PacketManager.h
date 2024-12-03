/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "PacketBuffer.h"

class PacketManager {
public:
  enum class ReceivingState { WAITING_HEADER,
                              WAITING_PAYLOAD,
                              WAITING_END,
                              RECEIVED,
                              ERROR };
  enum class MessageType { DATA     = 2,
                           TRANSMISSION_CONTROL = 3 };
  typedef struct {
    MessageType type;
    InputPacketBuffer payload;
  } ReceivedData;
  PacketManager()
    : _tempInputMessageHeader{ 5 }, _tempInputMessageEnd{ 4 } {};
  bool createPacket(OutputPacketBuffer &msg, MessageType type, const uint8_t *data, size_t len);
  ReceivingState handleReceivedByte(ReceivedData &receivedData, uint8_t byte);
  void clear();
private:
  ReceivingState _state = ReceivingState::WAITING_HEADER;
  InputPacketBuffer _tempInputMessageHeader;
  InputPacketBuffer _tempInputMessagePayload;
  InputPacketBuffer _tempInputMessageEnd;

  ReceivingState handle_WaitingHeader(uint8_t byte);
  ReceivingState handle_WaitingPayload(uint8_t byte);
  ReceivingState handle_WaitingEnd(uint8_t byte);
  void clearInputBuffers();
  bool checkBeginPacket();
  bool checkEndPacket();
  bool checkCRC();
  uint8_t getPacketType();
  uint16_t getPacketLen();
};

extern PacketManager Packet;
