/*
  Copyright (c) 2024 Arduino SA

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CSRHandler.h"
#include "SElementJWS.h"
#include <utility/SElementArduinoCloudCertificate.h>
#include <utility/SElementCSR.h>
#include <utility/SElementArduinoCloudDeviceId.h>
#include <utility/SElementArduinoCloud.h>
#include "SecretsHelper.h"
#include <utility/time/TimeService.h>
#include <stdlib.h>

#define RESPONSE_MAX_LEN 1100
#define RESPONSE_TIMEOUT 5000
#define BACKEND_INTERVAL_s 12
#define MAX_CSR_REQUEST_INTERVAL 180000
#define MAX_CSR_REQUEST_INTERVAL_ATTEMPTS 15


const char *server = "boards-v2.oniudra.cc";

CSRHandlerClass::~CSRHandlerClass() {
  if (_certForCSR) {
    delete _certForCSR;
    _certForCSR = nullptr;
  }
  if (_client) {
    delete _client;
    _client = nullptr;
  }
}

void CSRHandlerClass::begin(ConnectionHandler *connectionHandler, SecureElement *secureElement, String *uhwid) {
  _connectionHandler = connectionHandler;
  _secureElement = secureElement;
  _uhwid = uhwid;
  if (*_uhwid == "") {
    Serial.println("Error: UHWID not found");
    _state = CSRHandlerStates::ERROR;
    return;
  }

#ifdef BOARD_HAS_WIFI
  _fw_version = WiFi.firmwareVersion();
  if (_fw_version[0] == 'v'){
    _fw_version.remove(0, 1);
  }
#else
  _fw_version = "";
#endif

  _tlsClient.begin(*_connectionHandler);
  _tlsClient.setTimeout(RESPONSE_TIMEOUT);
  _client = new HttpClient(_tlsClient, server, 443);
  TimeService.begin(_connectionHandler);
  _requestAttempt = 0;
  _nextRequestAt = 0;
  _state = CSRHandlerStates::BUILD_CSR;
}

void CSRHandlerClass::end() {
  if (_client) {
    _client->stop();
  }
  _fw_version = "";
  _state = CSRHandlerStates::END;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::poll() {
  switch (_state) {
    case CSRHandlerStates::BUILD_CSR:            _state = handleBuildCSR          (); break;
    case CSRHandlerStates::REQUEST_SIGNATURE:    _state = handleRequestSignature  (); break;
    case CSRHandlerStates::WAITING_RESPONSE:     _state = handleWaitingResponse   (); break;
    case CSRHandlerStates::PARSE_RESPONSE:       _state = handleParseResponse     (); break;
    case CSRHandlerStates::BUILD_CERTIFICATE:    _state = handleBuildCertificate  (); break;
    case CSRHandlerStates::CERT_CREATED:         _state = handleCertCreated       (); break;
    case CSRHandlerStates::WAITING_COMPLETE_RES: _state = handleWaitingCompleteRes(); break;
    case CSRHandlerStates::COMPLETED:                                                 break;
    case CSRHandlerStates::ERROR:                                                     break;
    case CSRHandlerStates::END:                                                       break;
  }

  return _state;
}

void CSRHandlerClass::updateNextRequestAt() {
  uint32_t delay;
  if(_requestAttempt <= MAX_CSR_REQUEST_INTERVAL_ATTEMPTS) {
    delay = BACKEND_INTERVAL_s * _requestAttempt * 1000; // use linear backoff since backend has a rate limit
  }else {
    delay = MAX_CSR_REQUEST_INTERVAL;
  }

  _nextRequestAt = millis() + delay + jitter();
  Serial.print("Next request at: ");
  Serial.println(_nextRequestAt);
}

uint32_t CSRHandlerClass::jitter(uint32_t base, uint32_t max) {
  srand(millis());
  return base + rand() % (max - base);
}

bool CSRHandlerClass::postRequest(const char *url, String &postData) {
  if(!_client){
    _client = new HttpClient(_tlsClient, server, 443);
  }

  uint32_t ts = getTimestamp();
  if(ts == 0){
    Serial.println("Error getting timestamp");
    return false;
  }

  String token = CreateJWTToken(*_uhwid, ts, _secureElement);
  Serial.print("Token: ");
  Serial.println(token);

  _requestAttempt++;
  _client->beginRequest();

  if(_client->post(url) == 0){
    _client->sendHeader("Host", server);
    _client->sendHeader("Connection", "close");
    _client->sendHeader("Content-Type", "application/json;charset=UTF-8");
    _client->sendHeader("Authorization", "Bearer " + token);
    _client->sendHeader("Content-Length", postData.length());
    _client->beginBody();
    _client->print(postData);
    _startWaitingResponse = millis();
    return true;
  }
  return false;
}

uint32_t CSRHandlerClass::getTimestamp() {
  uint8_t getTsAttempt = 0;
  uint32_t ts = 0;
  do{
    ts = TimeService.getTime();
    getTsAttempt++;
  }while(ts == 0 && getTsAttempt < 3);

  return ts;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleBuildCSR() {
  if (!_certForCSR) {
    _certForCSR = new ECP256Certificate();
  }
  if (!_certForCSR->begin()) {
    Serial.println("Error starting CSR generation!");
    return CSRHandlerStates::ERROR;
  }

  _certForCSR->setSubjectCommonName(*_uhwid);

  if (!SElementCSR::build(*_secureElement, *_certForCSR, static_cast<int>(SElementArduinoCloudSlot::Key), true)) {
    Serial.println("Error generating CSR!");
    return CSRHandlerStates::ERROR;
  }
  return CSRHandlerStates::REQUEST_SIGNATURE;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleRequestSignature() {
  CSRHandlerStates nextState = _state;

  if(millis() < _nextRequestAt) {
    return nextState;
  }

  NetworkConnectionState connectionRes = _connectionHandler->check();
  if (connectionRes != NetworkConnectionState::CONNECTED) {
    return nextState;
  }

  if(!_certForCSR){
    return CSRHandlerStates::BUILD_CSR;
  }

  String csr = _certForCSR->getCSRPEM();
  csr.replace("\n", "\\n");

  String PostData = "{\"csr\":\"";
  PostData += csr;
  PostData += "\"}";
  Serial.println("Downloading certificate...");

  if(postRequest("/provisioning/v1/onboarding/provision/csr", PostData)){
    nextState = CSRHandlerStates::WAITING_RESPONSE;
  } else {
    Serial.println("Error sending request");
    updateNextRequestAt();
  }

  return nextState;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleWaitingResponse() {
  CSRHandlerStates nextState = _state;
  NetworkConnectionState connectionRes = _connectionHandler->check();
  if (connectionRes != NetworkConnectionState::CONNECTED) {
    _client->stop();
    return CSRHandlerStates::REQUEST_SIGNATURE;
  }

  if (millis() - _startWaitingResponse > RESPONSE_TIMEOUT) {
    _client->stop();
    Serial.println("Timeout waiting for response");
    updateNextRequestAt();
    nextState = CSRHandlerStates::REQUEST_SIGNATURE;
  }


  int statusCode = _client->responseStatusCode();
  if(statusCode == 200){
    nextState = CSRHandlerStates::PARSE_RESPONSE;
  } else {
    _client->stop();
    Serial.print("Error response: ");
    Serial.println(statusCode);
    updateNextRequestAt();
    nextState = CSRHandlerStates::REQUEST_SIGNATURE;
  }

  return nextState;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleParseResponse() {
  CSRHandlerStates nextState = _state;
  String certResponse = _client->responseBody();
  _client->stop();
  Serial.println("Here's the answer to csr request");
  Serial.println(certResponse);

  JSONVar myObject = JSON.parse(certResponse);

  if (myObject.hasOwnProperty("device_id") && myObject["compressed"].hasOwnProperty("not_before") && \
  myObject["compressed"].hasOwnProperty("serial") && myObject["compressed"].hasOwnProperty("authority_key_identifier") && \
  myObject["compressed"].hasOwnProperty("signature_asn1_x") && myObject["compressed"].hasOwnProperty("signature_asn1_x")) {

    _deviceId = (const char *)myObject["device_id"];
    _not_before = (const char *)myObject["compressed"]["not_before"];
    _serialNumber = (const char *)myObject["compressed"]["serial"];
    _authorityKeyIdentifier = (const char *)myObject["compressed"]["authority_key_identifier"];
    _signature = (const char *)myObject["compressed"]["signature_asn1_x"];
    _signature += String((const char *)myObject["compressed"]["signature_asn1_y"]);
    nextState = CSRHandlerStates::BUILD_CERTIFICATE;
  } else {
    Serial.println("Error parsing cloud certificate");
    updateNextRequestAt();
    nextState = CSRHandlerStates::REQUEST_SIGNATURE;
  }

  return nextState;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleBuildCertificate() {
  byte serialNumberBytes[16];
  byte authorityKeyIdentifierBytes[20];
  byte signatureBytes[64];
  String issueYear = _not_before.substring(0, 4);
  String issueMonth = _not_before.substring(5, 7);
  String issueDay = _not_before.substring(8, 10);
  String issueHour = _not_before.substring(11, 13);
  int expireYears = 31;

  hexStringToBytes(_serialNumber, serialNumberBytes, sizeof(serialNumberBytes));
  hexStringToBytes(_authorityKeyIdentifier, authorityKeyIdentifierBytes, sizeof(authorityKeyIdentifierBytes));
  hexStringToBytes(_signature, signatureBytes, sizeof(signatureBytes));

  if (!SElementArduinoCloudDeviceId::write(*_secureElement, _deviceId, SElementArduinoCloudSlot::DeviceId)) {
    Serial.println("Error storing device id!");
    return CSRHandlerStates::ERROR;
  }


  ECP256Certificate cert;
  if (!cert.begin()) {
    Serial.println("Error starting secureElement storage!");
    return CSRHandlerStates::ERROR;
  }

  cert.setSubjectCommonName(_deviceId);
  cert.setIssuerCountryName("US");
  cert.setIssuerOrganizationName("Arduino LLC US");
  cert.setIssuerOrganizationalUnitName("IT");
  cert.setIssuerCommonName("Arduino");
  cert.setSignature(signatureBytes, sizeof(signatureBytes));
  cert.setAuthorityKeyId(authorityKeyIdentifierBytes, sizeof(authorityKeyIdentifierBytes));
  cert.setSerialNumber(serialNumberBytes, sizeof(serialNumberBytes));
  cert.setIssueYear(issueYear.toInt());
  cert.setIssueMonth(issueMonth.toInt());
  cert.setIssueDay(issueDay.toInt());
  cert.setIssueHour(issueHour.toInt());
  cert.setExpireYears(expireYears);

  if (!SElementArduinoCloudCertificate::build(*_secureElement, cert, static_cast<int>(SElementArduinoCloudSlot::Key))) {
    Serial.println("Error building secureElement compressed cert!");
    return CSRHandlerStates::ERROR;
  }

  if (!SElementArduinoCloudCertificate::write(*_secureElement, cert, SElementArduinoCloudSlot::CompressedCertificate)) {
    Serial.println("Error storing cert!");
    return CSRHandlerStates::ERROR;
  }

  Serial.println("Certificate created!");
  _nextRequestAt = 0;
  _requestAttempt = 0;
  return CSRHandlerStates::CERT_CREATED;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleCertCreated() {
  CSRHandlerStates nextState = _state;

  if(millis() < _nextRequestAt) {
    return nextState;
  }

  NetworkConnectionState connectionRes = _connectionHandler->check();
  if (connectionRes != NetworkConnectionState::CONNECTED) {
    return nextState;
  }

  String PostData = "{\"wifi_fw_version\":\"";
  PostData += _fw_version;
  PostData += "\"}";
  if(postRequest("/provisioning/v1/onboarding/provision/complete", PostData)){
    nextState = CSRHandlerStates::WAITING_COMPLETE_RES;
  } else {
    Serial.println("Error sending request");
    updateNextRequestAt();
  }

  return nextState;
}

CSRHandlerClass::CSRHandlerStates CSRHandlerClass::handleWaitingCompleteRes() {
  CSRHandlerStates nextState = _state;
  NetworkConnectionState connectionRes = _connectionHandler->check();
  if (connectionRes != NetworkConnectionState::CONNECTED) {
    _client->stop();
    return CSRHandlerStates::CERT_CREATED;
  }

  if (millis() - _startWaitingResponse > RESPONSE_TIMEOUT) {
    _client->stop();
    Serial.println("Timeout waiting for response");
    updateNextRequestAt();
    nextState = CSRHandlerStates::CERT_CREATED;
  }

  int statusCode = _client->responseStatusCode();
  if(statusCode == 200){
    if (_certForCSR) {
      delete _certForCSR;
      _certForCSR = nullptr;
    }
    nextState = CSRHandlerStates::COMPLETED;
  } else if (statusCode == 429 || statusCode == 503) {
    updateNextRequestAt();
    nextState = CSRHandlerStates::CERT_CREATED;
  } else {
    Serial.print("Error response: ");
    Serial.println(statusCode);
    Serial.println("error in complete csr operation, retry");
    _requestAttempt = 0;
    _nextRequestAt = 0;
    nextState = CSRHandlerStates::REQUEST_SIGNATURE;
  }
  _client->stop();

  return nextState;
}
