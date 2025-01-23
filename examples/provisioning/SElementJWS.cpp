/*
  This file is part of the Arduino_SecureElement library.
  Copyright (c) 2024 Arduino SA
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/******************************************************************************
 * INCLUDE
 ******************************************************************************/

#include "SElementJWS.h"
#include <ArduinoECCX08.h>

#define ASN1_INTEGER           0x02
#define ASN1_BIT_STRING        0x03
#define ASN1_NULL              0x05
#define ASN1_OBJECT_IDENTIFIER 0x06
#define ASN1_PRINTABLE_STRING  0x13
#define ASN1_SEQUENCE          0x30
#define ASN1_SET               0x31

static String base64urlEncode(const byte in[], unsigned int length)
{
  static const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";

  int b;
  String out;

  int reserveLength = 4 * ((length + 2) / 3);
  out.reserve(reserveLength);

  for (unsigned int i = 0; i < length; i += 3) {
    b = (in[i] & 0xFC) >> 2;
    out += CODES[b];

    b = (in[i] & 0x03) << 4;
    if (i + 1 < length) {
      b |= (in[i + 1] & 0xF0) >> 4;
      out += CODES[b];
      b = (in[i + 1] & 0x0F) << 2;
      if (i + 2 < length) {
         b |= (in[i + 2] & 0xC0) >> 6;
         out += CODES[b];
         b = in[i + 2] & 0x3F;
         out += CODES[b];
      } else {
        out += CODES[b];
      }
    } else {
      out += CODES[b];
    }
  }

  while (out.lastIndexOf('=') != -1) {
    out.remove(out.length() - 1);
  }

  return out;
}

int publicKeyLength()
{
  return (2 + 2 + 9 + 10 + 4 + 64);
}

int appendPublicKey(const byte publicKey[], byte out[])
{
  int subjectPublicKeyDataLength = 2 + 9 + 10 + 4 + 64;

  // subject public key
  *out++ = ASN1_SEQUENCE;
  *out++ = (subjectPublicKeyDataLength) & 0xff;

  *out++ = ASN1_SEQUENCE;
  *out++ = 0x13;

  // EC public key
  *out++ = ASN1_OBJECT_IDENTIFIER;
  *out++ = 0x07;
  *out++ = 0x2a;
  *out++ = 0x86;
  *out++ = 0x48;
  *out++ = 0xce;
  *out++ = 0x3d;
  *out++ = 0x02;
  *out++ = 0x01;

  // PRIME 256 v1
  *out++ = ASN1_OBJECT_IDENTIFIER;
  *out++ = 0x08;
  *out++ = 0x2a;
  *out++ = 0x86;
  *out++ = 0x48;
  *out++ = 0xce;
  *out++ = 0x3d;
  *out++ = 0x03;
  *out++ = 0x01;
  *out++ = 0x07;

  *out++ = 0x03;
  *out++ = 0x42;
  *out++ = 0x00;
  *out++ = 0x04;

  memcpy(out, publicKey, 64);

  return (2 + subjectPublicKeyDataLength);
}

String base64Encode(const byte in[], unsigned int length, const char* prefix, const char* suffix)
{
  static const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int b;
  String out;

  int reserveLength = 4 * ((length + 2) / 3) + ((length / 3 * 4) / 76) + strlen(prefix) + strlen(suffix);
  out.reserve(reserveLength);

  if (prefix) {
    out += prefix;
  }
  
  for (unsigned int i = 0; i < length; i += 3) {
    if (i > 0 && (i / 3 * 4) % 76 == 0) { 
      out += '\n'; 
    }

    b = (in[i] & 0xFC) >> 2;
    out += CODES[b];

    b = (in[i] & 0x03) << 4;
    if (i + 1 < length) {
      b |= (in[i + 1] & 0xF0) >> 4;
      out += CODES[b];
      b = (in[i + 1] & 0x0F) << 2;
      if (i + 2 < length) {
         b |= (in[i + 2] & 0xC0) >> 6;
         out += CODES[b];
         b = in[i + 2] & 0x3F;
         out += CODES[b];
      } else {
        out += CODES[b];
        out += '=';
      }
    } else {
      out += CODES[b];
      out += "==";
    }
  }

  if (suffix) {
    out += suffix;
  }

  return out;
}

String SElementJWS::publicKey(SecureElement & se, int slot, bool newPrivateKey)
{
  if (slot < 0 || slot > 8) {
    return "";
  }

  byte publicKey[64];

  if (newPrivateKey) {
    if (!se.generatePrivateKey(slot, publicKey)) {
      return "";
    }
  } else {
    if (!se.generatePublicKey(slot, publicKey)) {
      return "";
    }
  }

  int length = publicKeyLength();
  byte out[length];

  appendPublicKey(publicKey, out);

  return base64Encode(out, length, "-----BEGIN PUBLIC KEY-----\n", "\n-----END PUBLIC KEY-----\n");
}

String SElementJWS::sign(SecureElement & se, int slot, const char* header, const char* payload)
{
  if (slot < 0 || slot > 8) {
    return "";
  }

  String encodedHeader = base64urlEncode((const byte*)header, strlen(header));
  String encodedPayload = base64urlEncode((const byte*)payload, strlen(payload));

  String toSign;
  toSign.reserve(encodedHeader.length() + 1 + encodedPayload.length());

  toSign += encodedHeader;
  toSign += '.';
  toSign += encodedPayload;


  byte toSignSha256[32];
  byte signature[64];

  se.SHA256((const uint8_t*)toSign.c_str(), toSign.length(), toSignSha256);

  if (!se.ecSign(slot, toSignSha256, signature)) {
    return "";
  }

  String encodedSignature = base64urlEncode(signature, sizeof(signature));

  String result;
  result.reserve(toSign.length() + 1 + encodedSignature.length());

  result += toSign;
  result += '.';
  result += encodedSignature;

  return result;
}

String SElementJWS::sign(SecureElement & se, int slot, const String& header, const String& payload)
{
  return sign(se, slot, header.c_str(), payload.c_str());
}