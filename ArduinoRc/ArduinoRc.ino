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
byte dataBlock[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}; // Datos a escribir (16 bytes)

byte buffer[18]; // Buffer para almacenar los datos leídos (debe ser mayor que 16)
byte size = sizeof(buffer); // Tamaño del buffer


int estado = 0; // 0: esperando a recibir el handshake para iniciar la comunicacion
byte nTrama = 0;

// Función para recibir datos, devuelve un puntero al buffer con los datos
byte* recibirDatos() {
  // 1. Leemos el tamaño de los datos recibidos
  byte bufferSize = rfid.PCD_ReadRegister(MFRC522::FIFOLevelReg);
  static byte buffer[63]; // Tamaño máximo del buffer, ajustar si es necesario


  //minimo tiene que ser 3 para poder hacer el cheque de los bits para ver que sea del tipo que es
  if(bufferSize == 2){
    buffer[0] = ~ rfid.PCD_ReadRegister(MFRC522::FIFODataReg);
    //se puede tratar de una trama de no datos
    //si es F 1 4 5 se trata de un inicio, o fin comunicacion, no importa si tiene mas datos
    if(buffer[0] == 0x0F || buffer[0] == 0x01 || buffer[0] == 0x04 || buffer[0] == 0x05){
      buffer[0] = buffer[0];
    }else{
      if(buffer[0] == 0x03){
        for (byte i = 0; i < bufferSize; i++) {
          buffer[i] = rfid.PCD_ReadRegister(MFRC522::FIFODataReg);
        }
        
      }
    }
  }else{
    buffer[0] = rfid.PCD_ReadRegister(MFRC522::FIFODataReg);
    if(bufferSize == 63 && buffer[0] == 0x02){    //se fija que es solo de datos y este completo
      static byte buffer[63]; // Tamaño máximo del buffer, ajustar si es necesario

      // 2. Leemos los datos desde el FIFO
      for (byte i = 0; i < bufferSize; i++) {
        buffer[i] = rfid.PCD_ReadRegister(MFRC522::FIFODataReg);
      }
    }
  }
  //rfid.PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x00);
  rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Receive);
  return buffer;
}

// Función para transmitir datos
void transmitirDatos(byte* dataToSend, byte dataSize) {
  // 1. Escribir los datos en el registro FIFO
  for (byte i = 0; i < dataSize; i++) {
    rfid.PCD_WriteRegister(MFRC522::FIFODataReg, dataToSend[i]);
  }

  // 2. Transmitir los datos en el FIFO
  rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transceive);

  Serial.println("Datos transmitidos.");
}

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

   //se modfica ya que se quiero utilizar los datos resividos por modulated signal from the internal analog module, default
   //para eso de debe colocar 10 en los bits 7-6 evitando que la senial pase por desencoder Miller
  // Modificar RxSelReg para poner UARTSel en 10 (bits 7-6)
  /*  byte currentRxSelReg = rfid.PCD_ReadRegister(MFRC522::TxSelReg); // Leer valor actual
    currentRxSelReg &= 0x3F;  // Limpiar los bits 7-6 (de UARTSel)
    currentRxSelReg |= 0x80;  // Poner UARTSel en 10 (10xx xxxx)
    rfid.PCD_WriteRegister(MFRC522::TxSelReg, currentRxSelReg); // Escribir el nuevo valor en RxSelReg
  */

// Leer el valor actual del registro TxSelReg
byte currentTxSelReg = rfid.PCD_ReadRegister(MFRC522::TxSelReg);

// Limpiar los 4 bits menos significativos, manteniendo los 4 más significativos
currentTxSelReg &= 0b11110000;

// Establecer los 4 bits menos significativos a 0001
currentTxSelReg |= 0b00010010;

// Escribir el nuevo valor en el registro TxSelReg
rfid.PCD_WriteRegister(MFRC522::TxSelReg, currentTxSelReg);
  

// Leer el valor actual del registro RxSelReg
byte currentRxSelReg = rfid.PCD_ReadRegister(MFRC522::RxSelReg);

// Modificar los 2 bits más significativos a 10, manteniendo los otros bits intactos
currentRxSelReg &= 0b00111111; // Poner a 0 los dos bits más significativos
currentRxSelReg |= 0b10000000; // Establecer los dos bits más significativos a 10

// Escribir el nuevo valor en el registro RxSelReg
rfid.PCD_WriteRegister(MFRC522::RxSelReg, currentRxSelReg);

//rfid.PCD_ClearRegisterBitMask(MFRC522::TxControlReg, 0x03); // Desactivar la antena

//   DatoHex = printHex(key.keyByte, txControl);
   Serial.println();
   Serial.println();
   Serial.println("Iniciando el Programa");
   rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Receive);
}


//codigo para leer datos
void loop() {
  // 1. Colocamos el MFRC522 en modo de recepción
  rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Receive);

  // 2. Esperamos a que se reciban datos en el buffer FIFO
  if (rfid.PCD_ReadRegister(MFRC522::FIFOLevelReg) == 7) {
    // 3. Leemos el tamaño de los datos recibidos
    byte bufferSize = rfid.PCD_ReadRegister(MFRC522::FIFOLevelReg);
    byte buffer[bufferSize];

    // 4. Leemos los datos recibidos desde el FIFO
    for (byte i = 0; i < bufferSize; i++) {
      buffer[i] = rfid.PCD_ReadRegister(MFRC522::FIFODataReg);
    }

    // 5. Enviamos los datos leídos al Arduino (al puerto serie en este caso)
    String hexString = printHex(buffer, bufferSize); // Convierte el buffer a hexadecimal
    Serial.println(hexString);  // Imprime la cadena hexadecimal en el monitor serie
    Serial.print("Datos recibidos: ");
    /*for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }*/
    Serial.println();
    rfid.PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x80);
  }


  byte errorReg = rfid.PCD_ReadRegister(MFRC522::ErrorReg);  // Leer el contenido del ErrorReg

if (errorReg == 0) {
    // No hay errores
    Serial.println("No se detectaron errores.");
} else {
    // Hay errores presentes, verificar qué bits están en 1
    if (errorReg & 0x01) {
        Serial.println("Error de escritura (WrErr)");
    }
    if (errorReg & 0x02) {
        Serial.println("Error de temperatura (TempErr)");
    }
    if (errorReg & 0x04) {
        Serial.println("Desbordamiento del buffer (BufferOvfl)");
    }
    if (errorReg & 0x08) {
        Serial.println("Error de colisión (CollErr)");
    }
    if (errorReg & 0x10) {
        Serial.println("Error de CRC (CRCErr)");
    }
    if (errorReg & 0x20) {
        Serial.println("Error de paridad (ParityErr)");
    }
    if (errorReg & 0x40) {
        Serial.println("Error de protocolo (ProtocolErr)");
    }
}
  delay(1000); // Añadir un pequeño retardo para evitar lecturas excesivas
}


/*
void loop(){
  //Estar en modo escucha
  // 1. Colocamos el MFRC522 en modo de recepción
  //rfid.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Receive);

  switch (estado) {
  case 0:{
    //Si recibe un mensaje:
    // 2. Esperamos a que se reciban datos en el buffer FIFO
    byte* datosRecibidos = recibirDatos();
   
    if(datosRecibidos[0]== 0x0F){
      //lo que recibo es los datos 
      byte dataToSend[] = {0x01};  // Datos a transmitir

      Serial.println("empezo la comunicacion");
  
      transmitirDatos(dataToSend,1);

      nTrama = 0;
      estado = 1;
    }
    }
    break;
  case 1:
    if (rfid.PCD_ReadRegister(MFRC522::FIFOLevelReg) == 1) {
      //se recibio un mensaje es decir que al alguien cerca transmitiendo
      //chequear que el mesaje

      byte* datosRecibidos = recibirDatos();
   
      if(datosRecibidos[0]== 0x02 && datosRecibidos[1]== nTrama){ //si recibo datos y en el numero de trama
        //lo que recibo es los datos 
        byte dataToSend[] = {0x03, nTrama};  // Datos a transmitir
  
        transmitirDatos(dataToSend,2);

        nTrama = nTrama + 1;
        estado = 1;
      }
    }
    break;
    case 2:{
      byte finACK[] = {0x05};  // Datos a transmitir
      transmitirDatos(finACK,1);

      estado = 0;
    }
    break;
    default:
    // Código a ejecutar si variable no coincide con ninguno de los casos
    break;
  }*/
  /*
     Si recibe un mensaje:
        iniciar handshake que en este caso es solo de dos pasos
        envía un ACK que está listo para recibir mensajes
        Mientras no reciba un fin de comunicacion:
        Va a recibir tramas de 60 bytes.(poner time out)
            Realiza el chequeo de dato
              Si esta bien: reenvía un ACK
              sino: pide una retransmisión(poner time out 1 y re enviar)
        Cuando recibe un fin de comunicacion, enviar un ACKFin

(el tamaño del buffer en total es de 64 nos reservamos 4 para hacer el chequeo).

  */
//}


/*
void loop() 
{
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! rfid.PICC_IsNewCardPresent()){return;}
     
    // Verify if the NUID has been readed
    //if ( ! rfid.PICC_ReadCardSerial()){return;}

    Serial.println("Datos leídos correctamente");


    MFRC522::StatusCode status;
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
    //  status = rfid.MIFARE_Write(block, dataBlock, 16); // Escribir los 16 bytes
/*
     byte command = MFRC522::PCD_Transceive; // Comando para transmitir datos
    byte sendData[] = { 0x60, 0x00 };       // Datos a enviar (cambiar por lo que necesites)
    byte sendLen = sizeof(sendData);        // Longitud de los datos a enviar
    byte backData[18];                      // Buffer para los datos recibidos
    byte backLen = sizeof(backData);        // Longitud de los datos recibidos
    byte waitIRq = 0x30;                    // Esperar recibir datos (RxIRq)

      MFRC522::StatusCode status = rfid.PCD_CommunicateWithPICC(
        command,           // Comando a ejecutar
        waitIRq,           // El interrupto a esperar
        sendData,          // Los datos que enviamos
        sendLen,           // Tamaño de los datos
        backData,          // Donde almacenamos los datos recibidos
        &backLen,          // Tamaño del buffer de datos recibidos
        nullptr,           // Bits válidos (puedes ignorar con nullptr)
        0,                 // Alineación de bits (por defecto 0)
        false              // No verificar CRC
      );

if (status == MFRC522::STATUS_OK) {
    Serial.println(F("Datos enviados y respuesta recibida correctamente."));
    for (byte i = 0; i < backLen; i++) {
        Serial.print(backData[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
} else {
    Serial.print(F("Error: "));
    Serial.println(rfid.GetStatusCodeName(status));
}
    */
     /*DatoHex = printHex(rfid.uid.uidByte, rfid.uid.size);
     Serial.print("Codigo Tarjeta: "); Serial.println(DatoHex);
     */
/*
     status = rfid.MIFARE_Read(block, buffer, &size);
     // Supongamos que enviamos un comando, como leer un bloque
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("Error leyendo bloque: "));
      Serial.println(rfid.GetStatusCodeName(status));
      return;
    } 
*/
  /*  // Mostrar los datos leídos
    Serial.print(F("Datos del bloque "));
    Serial.print(block);
    Serial.println(F(": "));
    for (byte i = 0; i < 16; i++) {
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }

    
     // Halt PICC
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
