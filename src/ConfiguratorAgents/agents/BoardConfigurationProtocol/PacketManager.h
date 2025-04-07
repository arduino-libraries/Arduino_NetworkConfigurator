/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "Arduino.h"
#include "PacketBuffer.h"

/**
 * @namespace PacketManager
 * @brief Provides functionality for managing packets, including creating, receiving,
 * and validating packets with the ArduinoBoardConfiguration Protocol packet structure.
 */
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
   * The ArduinoBoardConfiguration Protocol packet structure
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

  /**
   * @brief Creates a packet with the specified type and payload data.
   * This function builds a packet by adding the appropriate header, payload,
   * and trailer, including the CRC16 checksum for the payload. The resulting packet
   * is stored in the provided output buffer.
   *
   * @param[out] msg The output buffer where the constructed packet will be stored.
   * @param[in] type The type of the message (e.g., DATA or TRANSMISSION_CONTROL).
   * @param[in] data Pointer to the payload data to be included in the packet.
   * @param[in] len The length of the payload data.
   * @return true if the packet was successfully created, false otherwise.
   */
  bool createPacket(OutputPacketBuffer &msg, MessageType type, const uint8_t *data, size_t len);


  /**
   * @class PacketReceiver
   * @brief Singleton class responsible for managing the reception of packets.
   */
  class PacketReceiver {
    public:
      /**
       * @brief Handles a received byte and updates the packet state.
       *
       * @param packet Reference to the packet being reconstructed.
       * @param byte The received byte to process.
       * @return The current state of the receiving process.
       */
      ReceivingState handleReceivedByte(Packet_t &packet, uint8_t byte);

      /**
       * @brief Retrieves the singleton instance of the PacketReceiver.
       * @return Reference to the singleton instance of PacketReceiver.
       */
      static PacketReceiver &getInstance() {
        static PacketReceiver instance;
        return instance;
      }
      /**
       * @brief Clears the contents of the given packet and resets its state.
       * @param packet Reference to the packet to be cleared.
       */
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
