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

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
// Init array that will store new NUID
byte nuidPICC[4];

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
String DatoHex;
const String UserReg_1 = "23B36511";
const String UserReg_2 = "B33786A3";
const String UserReg_3 = "7762C83B";
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

byte block = 1; // El bloque donde quieres escribir (sector 1, bloque 4 en una tarjeta MIFARE 1K)
byte dataBlock[] = {0x0A, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}; // Datos a escribir (16 bytes)

byte buffer[18]; // Buffer para almacenar los datos leídos (debe ser mayor que 16)
byte size = sizeof(buffer); // Tamaño del buffer

void setup() 
{
   Serial.begin(115200);
   SPI.begin(); // Init SPI bus
   rfid.PCD_Init(); // Init MFRC522
   Serial.println();
   Serial.print(F("Reader :"));
   rfid.PCD_DumpVersionToSerial();
   for (byte i = 0; i < 6; i++) {
     key.keyByte[i] = 0xFF;
   } 
   DatoHex = printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  
   // Modificar RxSelReg para poner UARTSel en 10 (bits 7-6)
   //byte currentModeReg = rfid.PCD_ReadRegister(MFRC522::ModeReg); // Leer valor actual
   //currentModeReg &= 0xF7;  // Limpiar el bit 3 (de UARTSel)
   //currentTxModeReg |= 0x08;  // pone un 1 en el bit 3 (de UARTSel)
   //rfid.PCD_WriteRegister(MFRC522::ModeReg, currentModeReg); // Escribir el nuevo valor en RxSelReg
   //currentModeReg = rfid.PCD_ReadRegister(MFRC522::ModeReg); // Leer valor actual
   //Serial.println(currentModeReg , HEX);
   Serial.println();
   Serial.println("Iniciando el Programa");
}

void loop() {
   byte dataToSend[] = {0xF0, 0xFF};  // Datos a transmitir
   byte buffer[18];  // Buffer para respuesta (se puede ajustar según necesidad)
  
   // 1. Escribir los datos en el registro FIFO
   for (byte i = 0; i < sizeof(dataToSend); i++) {
    rfid.PCD_WriteRegister(MFRC522::FIFODataReg, dataToSend[i]); // 0x09 es el registro FIFO_DATA_REGISTER
   }
  
   // 2. Transmitir los datos en el FIFO
   rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transmit); // Enviar el comando de transmisión
   
   delay(500); // Esperar antes de la siguiente transmisión
   byte currentTxModeReg = rfid.PCD_ReadRegister(MFRC522::TxModeReg); // Leer valor actual
   Serial.println(currentTxModeReg , HEX);
}




/*
void loop() 
{
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! rfid.PICC_IsNewCardPresent()){return;}
     
    // Verify if the NUID has been readed
    //if ( ! rfid.PICC_ReadCardSerial()){return;}

    Serial.println("Tarjeta cerca");

    // Modificar TxSelReg para poner MFOutSel en 0101 (bits 4-0)
    byte currentTxSelReg = rfid.PCD_ReadRegister(MFRC522::RxSelReg); // Leer valor actual
    currentTxSelReg &= 0xE0;  // Limpiar los bits 4-0 (de MFOutSel)
    currentTxSelReg |= 0x05;  // Poner MFOutSel en 0101 (xx01 01xx)
    rfid.PCD_WriteRegister(MFRC522::RxSelReg, currentTxSelReg); // Escribir el nuevo valor en TxSelReg

    MFRC522::StatusCode status;
    */
/*
    // Autenticar en el bloque
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Error de autenticación: "));
      Serial.println(rfid.GetStatusCodeName(status));
      return;
    }

   */
/*     
     Serial.print(F("PICC type: "));
     MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
     Serial.println(rfid.PICC_GetTypeName(piccType));
     // Check is the PICC of Classic MIFARE type
     if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K)
     {
       Serial.println("Su Tarjeta no es del tipo MIFARE Classic.");
       return;
     }
*/

      // Escribir datos en el bloque
 //   status = rfid.MIFARE_Write(block, dataBlock, 16); // Escribir los 16 bytes
     
    
     /*DatoHex = printHex(rfid.uid.uidByte, rfid.uid.size);
     Serial.print("Codigo Tarjeta: "); Serial.println(DatoHex);
     */

/*     status = rfid.MIFARE_Read(block, buffer, &size);
   if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Error leyendo bloque: "));
      Serial.println(rfid.GetStatusCodeName(status));
      return;
    } 
*/
/*
    // Mostrar los datos leídos
    Serial.print(F("Datos del bloque "));
    Serial.print(block);
    Serial.println(F(": "));
    for (byte i = 0; i < 16; i++) {
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }
*/
    
/*     // Halt PICC
     //rfid.PICC_HaltA();
     // Stop encryption on PCD
     //rfid.PCD_StopCrypto1();
}*/


String printHex(byte *buffer, byte bufferSize)
{  
   String DatoHexAux = "";
   for (byte i = 0; i < bufferSize; i++) 
   {
       if (buffer[i] < 0x10)
       {
        DatoHexAux = DatoHexAux + "0";
        DatoHexAux = DatoHexAux + String(buffer[i], HEX);  
       }
       else { DatoHexAux = DatoHexAux + String(buffer[i], HEX); }
   }
   
   for (int i = 0; i < DatoHexAux.length(); i++) {DatoHexAux[i] = toupper(DatoHexAux[i]);}
   return DatoHexAux;
}

/* ESP 8266 NODE MCU
  Vcc <-> 3V3 (o Vin(5V) según la versión del módulo)
  RST (Reset) <-> D0
  GND (Tierra) <-> GND
  MISO (Master Input Slave Output) <-> D6
  MOSI (Master Output Slave Input) <-> D7
  SCK (Serial Clock) <-> D5
  SS/SDA (Slave select) <-> D8
*/

/* ESP 32 NODE MCU
  Vcc <-> 3V3 (o Vin(5V) según la versión del módulo)
  RST (Reset) <-> D22
  GND (Masse) <-> GND
  MISO (Master Input Slave Output) <-> 19
  MOSI (Master Output Slave Input) <-> 23
  SCK (Serial Clock) <-> 18
  SS/SDA (Slave select) <-> 5
*/
