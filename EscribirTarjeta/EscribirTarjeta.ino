// ESP32
#include <WiFi.h>
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

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

const char *ssid = "SSID";
const char *password = "PASSWORD";

WiFiServer server(80);

byte valorSeleccionado = 0x06;
byte opciones[] = {0x01, 0x02, 0x03, 0x04};

void setup()
{
  Serial.begin(9600);
  SPI.begin();     // Iniciar bus SPI
  rfid.PCD_Init(); // Inicializar lector MFRC522

  // Configurar la llave por defecto para autenticación
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  // Configuración WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");

  int max_retries = 5;
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < max_retries)
  {
    delay(1000);
    Serial.print(".");
    retries++;
  }
  if (retries >= max_retries)
  {
    Serial.println("No se pudo conectar a WiFi, reiniciando...");
    ESP.restart();
  }
  Serial.println();

  Serial.println("Conectado a la red WiFi");
  Serial.print("IP del servidor: ");
  Serial.println(WiFi.localIP());

  // Iniciar servidor web
  server.begin();

  delay(5000);
  Serial.println("Coloca una tarjeta para escribir...");
}

void loop()
{
  // Manejo de la conexión con el cliente HTTP para la selección del valor
  WiFiClient client = server.available();
  if (client)
  {
    String request = client.readStringUntil('\r');
    client.flush();

    // Manejar la selección de valor desde la solicitud HTTP
    for (byte opcion : opciones)
    {
      String ruta = "/set/" + String(opcion, HEX);
      if (request.indexOf(ruta) != -1)
      {
        valorSeleccionado = opcion; // Actualiza el valor seleccionado
        Serial.print("Nuevo valor seleccionado: ");
        Serial.println(valorSeleccionado, HEX);
        break;
      }
    }

    // Enviar página HTML con opciones
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<h1>Selecciona un Valor para Transmitir</h1>");
    for (byte opcion : opciones)
    {
      client.println("<p><a href=\"/set/" + String(opcion, HEX) + "\">Seleccionar " + String(opcion, HEX) + "</a></p>");
    }
    client.println("<p>Valor actual para escribir: " + String(valorSeleccionado, HEX) + "</p>");
    client.println("</html>");
    delay(1);
    client.stop();
  }

  rfid.PICC_IsNewCardPresent();
  rfid.PICC_ReadCardSerial();

  // Encender la antena para verificar si hay una tarjeta cerca
  rfid.PCD_AntennaOn();

  // Autenticar el bloque 4 usando la llave y la tarjeta
  byte block = 4;
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Error de autenticación: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  // Datos para escribir (16 bytes), utilizando el valor seleccionado
  byte dataBlock[16] = {
      valorSeleccionado, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

  // Escribir los datos en el bloque 4
  status = rfid.MIFARE_Write(block, dataBlock, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Error escribiendo bloque: ");
    Serial.println(rfid.GetStatusCodeName(status));
  }
  else
  {
    Serial.println("Datos escritos correctamente en el bloque ");
    Serial.print(block);
  }

  // Detener la tarjeta
  rfid.PCD_StopCrypto1();
  rfid.PCD_AntennaOff();

  delay(5000); // Esperar un poco antes de intentar escribir en otra tarjeta
}
