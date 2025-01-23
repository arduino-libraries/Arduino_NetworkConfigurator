#include "SElementJWS.h"
#include <Arduino_SecureElement.h>
#include <Arduino_UniqueHWId.h>

#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define VID USB_VID
#define PID USB_PID
#elif defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_GIGA)
#include "pins_arduino.h"
#define VID BOARD_VENDORID
#define PID BOARD_PRODUCTID
#elif defined(ARDUINO_OPTA)
#include "pins_arduino.h"
#define VID _BOARD_VENDORID
#define PID _BOARD_PRODUCTID
#elif defined(ARDUINO_PORTENTA_C33)
#include "pins_arduino.h"
#define VID USB_VID
#define PID USB_PID
#elif defined(ARDUINO_UNOR4_WIFI)
#define VID 0x2341
#define PID 0x1002
#else
#error "Add your board vid and pid"
#endif

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {}
  SecureElement secureElement;
  SElementJWS jws;
  while (!secureElement.begin()) {
    Serial.println("No secureElement present!");
    delay(100);
  }

  if (!secureElement.locked()) {

    if (!secureElement.writeConfiguration()) {
      Serial.println("Writing secureElement configuration failed!");
      Serial.println("Stopping Provisioning");
      while (1)
        ;
    }

    if (!secureElement.lock()) {
      Serial.println("Locking secureElement configuration failed!");
      Serial.println("Stopping Provisioning");
      while (1)
        ;
    }

    Serial.println("secureElement locked successfully");
    Serial.println();
  }

  String publicKeyPem = jws.publicKey(secureElement, 1, true);
  if (!publicKeyPem || publicKeyPem == "") {
    Serial.println("Error generating public key!");
    while(1);
  }
  
  Serial.println("Here's your public key PEM, enjoy!");
  Serial.println();
  Serial.println(publicKeyPem);
  UniqueHWId Id;
  if (Id.begin()) {
    Serial.print("UHWID: ");
    Serial.println(Id.get());
  }

  Serial.print("VID: ");
  Serial.println(VID, HEX);
  Serial.print("PID: ");
  Serial.println(PID, HEX);
}

void loop() {
  // put your main code here, to run repeatedly:

}
