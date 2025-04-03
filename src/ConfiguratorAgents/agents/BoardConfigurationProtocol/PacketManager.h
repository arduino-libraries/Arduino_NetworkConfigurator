/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "PacketBuffer.h"

namespace PacketManager {
  enum class ReceivingState { WAITING_HEADER,
                              WAITING_PAYLOAD,
                              WAITING_END,
                              RECEIVED,
                              ERROR };

  enum class MessageType { DATA     = 2,
                           TRANSMISSION_CONTROL = 3 };

  enum class TransmissionControlMessage : uint8_t{
    CONNECT    = 0x01,
    DISCONNECT = 0x02,
    NACK       = 0x03 };

  /*
   * The packet structure
   * 0x55 0xaa <type> <len> <payload> <crc> 0xaa 0x55
   *  ____________________________________________________________________________________________________________________________________
   * | Byte[0] | Byte[1] | Byte[2] | Byte[3] | Byte[4] | Byte[5].......Byte[len -3] | Byte[len-2] | Byte[len-1] | Byte[len] | Byte[len+1] |
   * |______________________HEADER_____________________|__________ PAYLOAD _________|_____________________ TRAILER _______________________|
   * | 0x55    | 0xaa    | <type>  | <len>             | <payload>                  | <crc>                     | 0xaa      | 0x55        |
   * |____________________________________________________________________________________________________________________________________|
   * <type> = MessageType: 2 = DATA, 3 = TRANSMISSION_CONTROL
   * <len> = length of the payload + 2 bytes for the CRC
   * <payload> = the data to be sent or received
   * <crc> = CRC16 of the payload
   */
  typedef struct {
    InputPacketBuffer Header = {5};
    InputPacketBuffer Payload;
    InputPacketBuffer Trailer = {4};
    uint32_t LastByteReceivedTs = 0;
    MessageType Type;
  } Packet_t;

  bool createPacket(OutputPacketBuffer &msg, MessageType type, const uint8_t *data, size_t len);

  class PacketReceiver {
    public:
      ReceivingState handleReceivedByte(Packet_t &packet, uint8_t byte);
      static PacketReceiver &getInstance() {
        static PacketReceiver instance;
        return instance;
      }
      void clear(Packet_t &packet);
    private:
      ReceivingState _state = ReceivingState::WAITING_HEADER;
      ReceivingState handle_WaitingHeader(Packet_t &packet, uint8_t byte);
      ReceivingState handle_WaitingPayload(Packet_t &packet, uint8_t byte);
      ReceivingState handle_WaitingEnd(Packet_t &packet, uint8_t byte);
      bool checkBeginPacket(Packet_t &packet);
      bool checkEndPacket(Packet_t &packet);
      bool checkCRC(Packet_t &packet);
      uint8_t getPacketType(Packet_t &packet);
      uint16_t getPacketLen(Packet_t &packet);
  };
}
