#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "RFID.h"
#include "Pins.h"

namespace RFID
{
  static MFRC522 reader(Pins::RC522_SS, Pins::RC522_RST);
  static bool ready = false;
  static String lastUid = "";

  static String byteToHex(byte value)
  {
    const char hex[] = "0123456789ABCDEF";

    String out;
    out += hex[(value >> 4) & 0x0F];
    out += hex[value & 0x0F];

    return out;
  }

  void begin()
  {
    ready = false;
    lastUid = "";

    if (!Pins::valid(Pins::RC522_SS) || !Pins::valid(Pins::RC522_RST))
      return;

    pinMode(Pins::RC522_SS, OUTPUT);
    digitalWrite(Pins::RC522_SS, HIGH);

    if (Pins::valid(Pins::SD_CARD_CS))
    {
      pinMode(Pins::SD_CARD_CS, OUTPUT);
      digitalWrite(Pins::SD_CARD_CS, HIGH);
    }

    pinMode(Pins::RC522_RST, OUTPUT);
    digitalWrite(Pins::RC522_RST, HIGH);

    SPI.begin(
        Pins::SHARED_SCK,
        Pins::SHARED_MISO,
        Pins::SHARED_MOSI,
        Pins::RC522_SS);

    reader.PCD_Init(Pins::RC522_SS, Pins::RC522_RST);
    yield();

    byte version = reader.PCD_ReadRegister(MFRC522::VersionReg);

    ready = version != 0x00 && version != 0xFF;
  }

  void update()
  {
  }

  bool isReady()
  {
    return ready;
  }

  bool readCard(String &uid)
  {
    uid = "";

    if (!ready)
      return false;

    if (Pins::valid(Pins::SD_CARD_CS))
      digitalWrite(Pins::SD_CARD_CS, HIGH);

    digitalWrite(Pins::RC522_SS, LOW);

    if (!reader.PICC_IsNewCardPresent())
    {
      digitalWrite(Pins::RC522_SS, HIGH);
      return false;
    }

    if (!reader.PICC_ReadCardSerial())
    {
      digitalWrite(Pins::RC522_SS, HIGH);
      return false;
    }

    for (byte i = 0; i < reader.uid.size; i++)
      uid += byteToHex(reader.uid.uidByte[i]);

    lastUid = uid;

    reader.PICC_HaltA();
    reader.PCD_StopCrypto1();

    digitalWrite(Pins::RC522_SS, HIGH);

    return uid.length() > 0;
  }

  String getLastUID()
  {
    return lastUid;
  }
}
