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

class StatusProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  StatusProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORStatusProvisioningMessage, StatusProvisioningMessageId) {}
protected:
  MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class ListWifiNetworksProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  ListWifiNetworksProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORListWifiNetworksProvisioningMessage, ListWifiNetworksProvisioningMessageId) {}
protected:
  MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class UniqueHardwareIdProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  UniqueHardwareIdProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORUniqueHardwareIdProvisioningMessage, UniqueHardwareIdProvisioningMessageId) {}
protected:
  MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class JWTProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  JWTProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORJWTProvisioningMessage, JWTProvisioningMessageId) {}
protected:
  MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class BLEMacAddressProvisioningMessageEncoder: public CBORMessageEncoderInterface {
public:
  BLEMacAddressProvisioningMessageEncoder()
  : CBORMessageEncoderInterface(CBORBLEMacAddressProvisioningMessage, BLEMacAddressProvisioningMessageId) {}
protected:
  MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
};

class ProvSketchVersionProvisioningMessageEncoder: public CBORMessageEncoderInterface {
  public:
  ProvSketchVersionProvisioningMessageEncoder()
    : CBORMessageEncoderInterface(CBORProvSketchVersionProvisioningMessage, ProvSketchVersionProvisioningMessageId) {}
  protected:
    MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
  };

class NetConfigLibVersProvisioningMessageEncoder: public CBORMessageEncoderInterface {
  public:
  NetConfigLibVersProvisioningMessageEncoder()
    : CBORMessageEncoderInterface(CBORNetConfigLibVersProvisioningMessage, NetConfigLibVersProvisioningMessageId) {}
  protected:
    MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
  };

class ProvPublicKeyProvisioningMessageEncoder: public CBORMessageEncoderInterface {
  public:
  ProvPublicKeyProvisioningMessageEncoder()
    : CBORMessageEncoderInterface(CBORProvPublicKeyProvisioningMessage, ProvPublicKeyProvisioningMessageId) {}
  protected:
    MessageEncoder::Status encode(CborEncoder* encoder, Message *msg) override;
  };
