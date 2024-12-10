// ESP8266
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#if defined(ESP32)
#define SS_PIN 5
#define RST_PIN 22
#elif defined(ESP8266)
#define SS_PIN D8
#define RST_PIN D0
#elif defined(ARDUINO_AVR_UNO)
#define RST_PIN 9 // Pin 9 para el reset del RC522
#define SS_PIN 10 // Pin 10 para el SS (SDA) del RC522
#endif

MFRC522 rfid(SS_PIN, RST_PIN); // Crear una instancia del objeto MFRC522
MFRC522::MIFARE_Key key;       // Llave para autenticación

void setup()
{
  Serial.begin(9600);
  SPI.begin();     // Iniciar bus SPI
  rfid.PCD_Init(); // Inicializar lector MFRC522

  // Configurar la llave por defecto para autenticación (es la que tienen por defecto las tarjetas nuevas)
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  delay(5000);

  Serial.println("Coloca una tarjeta para leerla...");
}

void loop()
{
  // Verificar si hay una tarjeta cerca

  // Encender la antena para verificar si hay una tarjeta cerca
  rfid.PCD_AntennaOn();

  rfid.PICC_IsNewCardPresent();
  rfid.PICC_ReadCardSerial();

  // Autenticar el bloque 4 (por ejemplo) usando la llave y la tarjeta
  byte block = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  /*
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Error de autenticación: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }*/

  // Leer los datos del bloque 4
  byte buffer[18];
  byte bufferSize = sizeof(buffer);
  status = rfid.MIFARE_Read(block, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Error leyendo bloque: ");
    Serial.println(rfid.GetStatusCodeName(status));
  }
  else
  {
    Serial.print("Datos leídos del bloque ");
    Serial.print(block);
    Serial.print(": ");
    for (byte i = 0; i < bufferSize; i++)
    {
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Detener la tarjeta
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Apagar la antena nuevamente después de leer la tarjeta
  rfid.PCD_AntennaOff();

  delay(1500); // Esperar un poco antes de intentar leer otra tarjeta
}
