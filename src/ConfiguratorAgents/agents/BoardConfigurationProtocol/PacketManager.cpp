/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ANetworkConfigurator_Config.h"
#if NETWORK_CONFIGURATOR_COMPATIBLE

#include <Arduino_DebugUtils.h>
#include "PacketManager.h"
#include "Arduino_CRC16.h"

namespace PacketManager {

  uint8_t PACKET_START[] = { 0x55, 0xaa };
  uint8_t PACKET_END[] = { 0xaa, 0x55 };
  #define PACKET_START_SIZE 2
  #define PACKET_END_SIZE 2
  #define PACKET_TYPE_SIZE 1
  #define PACKET_LENGTH_SIZE 2
  #define PACKET_CRC_SIZE 2
  #define PACKET_HEADERS_OVERHEAD PACKET_START_SIZE + PACKET_TYPE_SIZE + PACKET_LENGTH_SIZE + PACKET_CRC_SIZE + PACKET_END_SIZE
  #define PACKET_MINIMUM_SIZE PACKET_START_SIZE + PACKET_TYPE_SIZE + PACKET_LENGTH_SIZE

  #define CRC16_POLYNOMIAL 0x1021
  #define CRC16_INITIAL_VALUE 0xffff
  #define CRC16_FINAL_XOR_VALUE 0xffff
  #define CRC16_REFLECT_DATA true
  #define CRC16_REFLECT_RESULT true
  #define BYTES_VALIDITY_MS 10000

  bool createPacket(OutputPacketBuffer &outputMsg, MessageType type, const uint8_t *data, size_t len) {
    uint16_t packetLen = len + PACKET_HEADERS_OVERHEAD;
    uint16_t payloadLen = len + PACKET_CRC_SIZE;
    uint8_t payloadLenHigh = payloadLen >> 8;
    uint8_t payloadLenLow = payloadLen & 0xff;
    uint16_t payloadCRC = arduino::crc16::calculate((char *)data, len, CRC16_POLYNOMIAL, CRC16_INITIAL_VALUE, CRC16_FINAL_XOR_VALUE, CRC16_REFLECT_DATA, CRC16_REFLECT_RESULT);
    uint8_t crcHigh = payloadCRC >> 8;
    uint8_t crcLow = payloadCRC & 0xff;

    outputMsg.allocate(packetLen);

    outputMsg += PACKET_START[0];
    outputMsg += PACKET_START[1];
    outputMsg += (uint8_t)type;
    outputMsg += payloadLenHigh;
    outputMsg += payloadLenLow;

    if (!outputMsg.copyArray(data, len)) {
      return false;
    }

    outputMsg += crcHigh;
    outputMsg += crcLow;
    outputMsg += PACKET_END[0];
    outputMsg += PACKET_END[1];

    outputMsg.startProgress();
    return true;
  }

  PacketManager::ReceivingState PacketReceiver::handleReceivedByte(Packet_t &packet, uint8_t byte) {
    if (_state == ReceivingState::ERROR || _state == ReceivingState::RECEIVED) {
      _state = ReceivingState::WAITING_HEADER;
    }

    if (millis() - packet.LastByteReceivedTs > BYTES_VALIDITY_MS) {
      clear(packet);
    }

    packet.LastByteReceivedTs = millis();

    switch (_state) {
      case ReceivingState::WAITING_HEADER:  _state = handle_WaitingHeader (packet,byte); break;
      case ReceivingState::WAITING_PAYLOAD: _state = handle_WaitingPayload(packet,byte); break;
      case ReceivingState::WAITING_END:     _state = handle_WaitingEnd    (packet,byte); break;
      default:                                                                           break;
    }

    if (_state == ReceivingState::RECEIVED) {
      packet.Type = (MessageType)getPacketType(packet);
      if (packet.Type != MessageType::TRANSMISSION_CONTROL && packet.Type != MessageType::DATA) {
        //Packet type not recognized
        _state = ReceivingState::ERROR;
      }
    }

    if (_state == ReceivingState::ERROR) {
      clear(packet);
    }

    return _state;
  }

  void PacketReceiver::clear(Packet_t &packet) {
    packet.LastByteReceivedTs = 0;
    packet.Header.clear();
    packet.Payload.reset();
    packet.Trailer.clear();
  }

  bool PacketReceiver::checkBeginPacket(Packet_t &packet) {
    if (packet.Header.len() < sizeof(PACKET_START)) {
      return false;
    }

    if (packet.Header[0] == PACKET_START[0] && packet.Header[1] == PACKET_START[1]) {
      return true;
    }

    return false;
  }

  bool PacketReceiver::checkEndPacket(Packet_t &packet) {
    if (packet.Trailer.len() < sizeof(PACKET_END) + PACKET_CRC_SIZE) {
      return false;
    }
    if (packet.Trailer[2] == PACKET_END[0] && packet.Trailer[3] == PACKET_END[1]) {
      return true;
    }
    return false;
  }

  bool PacketReceiver::checkCRC(Packet_t &packet) {
    if (packet.Trailer.len() < sizeof(PACKET_END) + PACKET_CRC_SIZE) {
      return false;
    }
    uint16_t receivedCRC = ((uint16_t)packet.Trailer[0] << 8 | packet.Trailer[1]);
    uint16_t computedCRC = arduino::crc16::calculate((char *)&packet.Payload[0], packet.Payload.len(), CRC16_POLYNOMIAL, CRC16_INITIAL_VALUE, CRC16_FINAL_XOR_VALUE, CRC16_REFLECT_DATA, CRC16_REFLECT_RESULT);

    if (receivedCRC == computedCRC) {
      return true;
    }

    return false;
  }

  uint8_t PacketReceiver::getPacketType(Packet_t &packet) {
    return packet.Header[2];
  }

  uint16_t PacketReceiver::getPacketLen(Packet_t &packet) {
    if (packet.Header.len() < PACKET_MINIMUM_SIZE) {
      return 0;
    }

    return ((uint16_t)packet.Header[3] << 8 | packet.Header[4]);
  }

  PacketManager::ReceivingState PacketReceiver::handle_WaitingHeader(Packet_t &packet, uint8_t byte) {
    if (packet.Header.missingBytes() > 0) {
      packet.Header += byte;
    }

    if (packet.Header.receivedAll()) {
      if (!checkBeginPacket(packet)) {
        return ReceivingState::ERROR;
      }

      uint16_t packetLen = getPacketLen(packet);
      uint16_t payloadLen = packetLen - PACKET_CRC_SIZE;
      packet.Payload.allocate(payloadLen);
      packet.Payload.setPayloadLen(payloadLen);
      return ReceivingState::WAITING_PAYLOAD;
    }

    return ReceivingState::WAITING_HEADER;
  }

  PacketManager::ReceivingState PacketReceiver::handle_WaitingPayload(Packet_t &packet, uint8_t byte) {
    if (packet.Payload.missingBytes() > 0) {
      packet.Payload += byte;
    }

    if (packet.Payload.receivedAll()) {
    return ReceivingState::WAITING_END;
    }

    return ReceivingState::WAITING_PAYLOAD;
  }

  PacketManager::ReceivingState PacketReceiver::handle_WaitingEnd(Packet_t &packet ,uint8_t byte) {
    if (packet.Trailer.missingBytes() > 0) {
      packet.Trailer += byte;
    }

    if (packet.Trailer.receivedAll()) {
      if (checkCRC(packet) && checkEndPacket(packet)) {
        return ReceivingState::RECEIVED;
      } else {
        //Error
        return ReceivingState::ERROR;
      }
    }

    return ReceivingState::WAITING_END;
  }
}

#endif // NETWORK_CONFIGURATOR_COMPATIBLE
