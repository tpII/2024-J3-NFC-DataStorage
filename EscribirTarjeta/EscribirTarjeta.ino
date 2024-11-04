// LISTA DE REPRODUCCION - ESP32 - ESP8266
// https://www.youtube.com/playlist?list=PLZHVfZzF2DYID9jGK8EpcMni-U2CSTrw3

// LISTA DE REPRODCUION DEL CURSO DE ARDUINO DESDE CERO
// https://www.youtube.com/playlist?list=PLZHVfZzF2DYJeLXXxz6YtpBj4u7FoGPWN

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
  #define RST_PIN  9    //Pin 9 para el reset del RC522
  #define SS_PIN  10   //Pin 10 para el SS (SDA) del RC522
#endif

MFRC522 rfid(SS_PIN, RST_PIN);  // Crear una instancia del objeto MFRC522
MFRC522::MIFARE_Key key;        // Llave para autenticación


byte i = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();           // Iniciar bus SPI
  rfid.PCD_Init();       // Inicializar lector MFRC522

  // Configurar la llave por defecto para autenticación (es la que tienen por defecto las tarjetas nuevas)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  delay(5000);
    
  Serial.println("Coloca una tarjeta para escribir  ...");
}

void loop() {
     // Verificar si hay una tarjeta cerca
  /*if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(500);
    return;
  }*/

  // Encender la antena para verificar si hay una tarjeta cerca
  rfid.PCD_AntennaOn();

  rfid.PICC_IsNewCardPresent();
  rfid.PICC_ReadCardSerial();

  // Autenticar el bloque 4 (por ejemplo) usando la llave y la tarjeta
  byte block = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Error de autenticación: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  i= i + 2;
  
  // Datos para escribir (16 bytes)
  byte dataBlock[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14, i
  };

  // Escribir los datos en el bloque 4
  status = rfid.MIFARE_Write(block, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Error escribiendo bloque: ");
    Serial.println(rfid.GetStatusCodeName(status));
  } else {
    Serial.println("Datos escritos correctamente en el bloque ");
    Serial.print(block);
  }

  // Detener la tarjeta
  //rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Apagar la antena nuevamente después de leer la tarjeta
  rfid.PCD_AntennaOff();
  
  delay(5000);  // Esperar un poco antes de intentar escribir en otra tarjeta
}
