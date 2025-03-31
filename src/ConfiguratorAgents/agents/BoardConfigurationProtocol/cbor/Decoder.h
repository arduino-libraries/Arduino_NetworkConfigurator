/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <Arduino.h>

#include "ProvisioningMessage.h"
#include <Arduino_CBOR.h>

class TimestampProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  TimestampProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORTimestampProvisioningMessage, TimestampProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};

class CommandsProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  CommandsProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORCommandsProvisioningMessage, CommandsProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#if defined(BOARD_HAS_WIFI)
class WifiConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  WifiConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORWifiConfigProvisioningMessage, WifiConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_LORA)
class LoRaConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  LoRaConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORLoRaConfigProvisioningMessage, LoRaConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_CATM1_NBIOT)
class CATM1ConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  CATM1ConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORCATM1ConfigProvisioningMessage, CATM1ConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_ETHERNET)
class EthernetConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  EthernetConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBOREthernetConfigProvisioningMessage, EthernetConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_NB) || defined(BOARD_HAS_GSM) ||defined(BOARD_HAS_CELLULAR)
class CellularConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  CellularConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORCellularConfigProvisioningMessage, CellularConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_NB)
class NBIOTConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  NBIOTConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORNBIOTConfigProvisioningMessage, NBIOTConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif

#if defined(BOARD_HAS_GSM)
class GSMConfigProvisioningMessageDecoder: public CBORMessageDecoderInterface {
public:
  GSMConfigProvisioningMessageDecoder()
  : CBORMessageDecoderInterface(CBORGSMConfigProvisioningMessage, GSMConfigProvisioningMessageId) {}
protected:
  MessageDecoder::Status decode(CborValue* iter, Message *msg) override;
};
#endif
