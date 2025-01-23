/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <Arduino.h>

#include <Arduino_JSON.h>
#include "SElementJWS.h"
#include <Arduino_UniqueHWId.h>

#define CLAIMING_TOKEN_LENGTH 247  // 246 + terminator
#define TOKEN_VALIDITY 3600

inline void hexStringToBytes(String& in, byte out[], int length) {
  int inLength = in.length();
  in.toUpperCase();
  int outLength = 0;

  for (int i = 0; i < inLength && outLength < length; i += 2) {
    char highChar = in[i];
    char lowChar = in[i + 1];

    byte highByte = (highChar <= '9') ? (highChar - '0') : (highChar + 10 - 'A');
    byte lowByte = (lowChar <= '9') ? (lowChar - '0') : (lowChar + 10 - 'A');

    out[outLength++] = (highByte << 4) | (lowByte & 0xF);
  }
}

inline String GetUHWID() {
  UniqueHWId Id;
  if (Id.begin()) {
    return Id.get();
  }
  return "";
}

inline String CreateJWTToken(String issuer, uint64_t ts, SecureElement* se) {
  SElementJWS jws;

  JSONVar jwtHeader;
  JSONVar jwtClaim;

  jwtHeader["alg"] = "ES256";
  jwtHeader["typ"] = "JWT";

  jwtClaim["iat"] = (uint32_t)ts;
  jwtClaim["iss"] = issuer;
  
  String token = jws.sign(*se, 1, JSON.stringify(jwtHeader), JSON.stringify(jwtClaim));
  return token;
}
