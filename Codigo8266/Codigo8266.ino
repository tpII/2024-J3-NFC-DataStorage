#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "FS.h"
#include <ESP8266WebServer.h>

// Pines y configuraciones
#if defined(ESP32)
#define SS_PIN 5
#define RST_PIN 22
#elif defined(ESP8266)
#define SS_PIN D8
#define RST_PIN D0
#endif

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

const char *ssid = "Personal-68A-2.4GHz";                    //Telecentro-8299   Julia
const char *password = "9114DF768A";                //4RFTMTGN4PRE      011235813

String address = "";
int blockR = -1;

// Configuración del servidor web
ESP8266WebServer server(80);

// Variables globales
bool fileReadyToWrite = false; // Flag para indicar que el archivo está listo para escribir
byte startBlock = 4;        // Bloque inicial para escritura
String selectedFile = "";      // Archivo seleccionado
bool sendData = false;

unsigned long previousTime = 0; // Almacena el último tiempo de ejecución
const unsigned long interval = 5000; // interval de 5 segundos

void configureWiFi();
void initializeMfrc522();
void handleFileUpload();
void readCard();
void writeFileToCard();
void handleRoot();
void listFiles();
void handleSelectFile();
void handleSelectRemove();
void handleFileContent();
void handleChange();


void receiveData();
void sendBlock(byte *buffer);

void setup()
{
  Serial.begin(9600);

  // Inicializar SPI y el lector mfrc522
  SPI.begin();
  initializeMfrc522();

  // Inicializar SPIFFS para almacenamiento temporal
  if (!SPIFFS.begin())
  {
    Serial.println("Error inicializando SPIFFS.");
    return;
  }

  // Configurar y conectar a WiFi
  configureWiFi();

  // Configurar las rutas HTTP del servidor
  server.on("/", HTTP_GET, handleRoot);              // Página principal
  server.on("/upload", HTTP_POST, handleFileUpload); // Ruta para manejar subida de archivos
  server.on("/list", HTTP_GET, listFiles);           // Ruta para listar archivos
  server.on("/write", HTTP_GET, handleSelectFile);  // Ruta para seleccionar un archivo
  server.on("/view", HTTP_GET, handleFileContent);   // Ruta para ver el contenido de un archivo
  server.on("/remove", HTTP_GET,handleSelectRemove);
  server.on("/cambiar", HTTP_GET,handleChange);
  
  
  server.begin();                                    // Iniciar el servidor web

  Serial.println("Sistema listo.");
}

void loop()
{
  server.handleClient(); // Manejar solicitudes HTTP

   // Comprobar si han pasado 5 segundos
  unsigned long tiempoActual = millis();
  if (tiempoActual - previousTime >= interval) {
    previousTime = tiempoActual; // Actualizar el tiempo anterior
    if(sendData){
      readCard();  
    }else{
      receiveData();
    }
  }
}

void deleteAllFiles() {
  // Open the root directory
  Dir root = SPIFFS.openDir("/");
  

  // Iterate over all files in the root directory
  while (root.next()) {
    String fileName = root.fileName();
    fileName = fileName;
    Serial.print("Deleting file: ");
    Serial.println(fileName);

    if (SPIFFS.exists(fileName)) {
      // Delete the current file
      if (SPIFFS.remove(fileName)) {
        Serial.println("File deleted successfully.");
      } else {
        Serial.println("Failed to delete file.");
      }
    } else {
      Serial.println("El archivo no existe.");
    }
  }

  Serial.println("All files deleted.");
}

/**
 * Configura y conecta el ESP32 a la red WiFi.
 */
void configureWiFi()
{
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
  Serial.println("\nConectado a la red WiFi");
  Serial.print("IP del servidor: ");
  Serial.println(WiFi.localIP());
}

/**
 * Inicializa el lector mfrc522.
 */
void initializeMfrc522()
{
  mfrc522.PCD_Init();
  Serial.println("Lector mfrc522 inicializado.");

  // Configurar llave de autenticación por defecto
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
}

/**
 * Maneja la página principal con formulario de subida de archivo.
 */
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Servidor Web ESP8266</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f5;
      color: #333;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 1em;
      text-align: center;
      width: 100%;
    }
    main {
      padding: 20px;
      max-width: 800px;
      text-align: center;
    }
    h1 {
      color: #333;
    }
    button {
      background-color: #4CAF50;
      color: white;
      border: none;
      padding: 10px 20px;
      margin: 5px;
      cursor: pointer;
      border-radius: 5px;
      font-size: 16px;
    }
    button:hover {
      background-color: #45a049;
    }
    input[type="file"], input[type="submit"] {
      display: block;
      margin: 10px auto;
      padding: 10px;
      border-radius: 5px;
      border: 1px solid #ccc;
    }
    a {
      color: #4CAF50;
      text-decoration: none;
      font-size: 18px;
    }
    a:hover {
      text-decoration: underline;
    }
    .controls {
      margin-top: 20px;
    }
  </style>
</head>
<body>
<header>
  <h1>Servidor Web ESP8266</h1>
</header>
<main>
  <h2>Subir Archivo</h2>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="file">
    <input type="submit" value="Subir Archivo">
  </form>
  <div class="controls">
    <a href="/list">Ver Archivos</a><br>
    <button onclick="location.href='/cambiar?cambio=recibir'">Recibir</button>
    <button onclick="location.href='/cambiar?cambio=enviar'">Enviar</button>
  </div>
</main>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}



char *generateRandomFileName()
{
  // Create a buffer to hold the random file name (max length)
  char *fileName = (char *)malloc(30); // Allocate space for the file name (adjust length as needed)

  if (fileName == NULL)
  {
    Serial.println("Memory allocation failed!");
    return NULL; // Return NULL if memory allocation fails
  }

  // Generate a random number and get current time (in millis)
  unsigned long timeMillis = millis()/1000;
  int randomNum = random(100, 999); // Random number to make the file name unique

  // Format the file name with the timestamp and random number
  snprintf(fileName, 30, "/file_%lu_%d.txt", timeMillis, randomNum);

  return fileName;
}
/**
 * Maneja la subida de archivos y guarda el archivo en SPIFFS.
 */
void handleFileUpload() {
  HTTPUpload &upload = server.upload();
  File file = SPIFFS.open("/" + upload.filename, "w");

  if (!file) {
    server.send(500, "text/html", "Error abriendo archivo para escribir.");
    return;
  }

  while (upload.status == UPLOAD_FILE_WRITE) {
    file.write(upload.buf, upload.currentSize);
  }

  if (upload.status == UPLOAD_FILE_END) {
    String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Archivo Subido</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f5;
      color: #333;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 1em;
      text-align: center;
      width: 100%;
    }
    main {
      padding: 20px;
      text-align: center;
    }
    a {
      color: #4CAF50;
      text-decoration: none;
      font-size: 18px;
    }
    a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
<header>
  <h1>Archivo Subido</h1>
</header>
<main>
  <p>El archivo <strong>)rawliteral" +
                     upload.filename +
                     R"rawliteral(</strong> se subió correctamente.</p>
  <a href="/">Volver a la página principal</a>
</main>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  }

  file.close();
}



void listFiles() {
  String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Lista de Archivos</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f0f0f5; color: #333; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; }
    header { background-color: #4CAF50; color: white; padding: 1em; text-align: center; width: 100%; }
    main { padding: 20px; max-width: 800px; text-align: center; }
    .file-item { border: 1px solid #ccc; padding: 10px; border-radius: 5px; margin: 10px 0; display: flex; justify-content: space-between; align-items: center; }
    button { background-color: #4CAF50; color: white; border: none; padding: 10px 20px; margin: 5px; cursor: pointer; border-radius: 5px; font-size: 16px; }
    button:hover { background-color: #45a049; }
    a { color: #4CAF50; text-decoration: none; font-size: 18px; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
<header>
  <h1>Archivos en SPIFFS</h1>
</header>
<main>
  <div class="file-list">
)rawliteral";

  Dir root = SPIFFS.openDir("/");
  while (root.next()) {
    String fileName = root.fileName();
    html += "<div class=\"file-item\">";
    html += "<p>" + fileName + "</p>";
    html += "<button onclick=\"location.href='/view?file=" + fileName + "'\">Ver Contenido</button>";
    html += "<button onclick=\"location.href='/write?file=" + fileName + "'\">Seleccionar</button>";
    html += "<button onclick=\"location.href='/remove?file=" + fileName + "'\">Eliminar</button>";
    html += "</div>";
  }

  html += R"rawliteral(
  </div>
  <a href="/">Volver a la página principal</a>
</main>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}



void handleSelectRemove() {
  if (server.hasArg("file")) {
    String fileName = server.arg("file");
    if (SPIFFS.exists(fileName)) {
      if (SPIFFS.remove(fileName)) {
        String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Archivo Eliminado</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f5;
      color: #333;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 1em;
      text-align: center;
      width: 100%;
    }
    main {
      padding: 20px;
      text-align: center;
    }
    a {
      color: #4CAF50;
      text-decoration: none;
      font-size: 18px;
    }
    a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
<header>
  <h1>Archivo Eliminado</h1>
</header>
<main>
  <p>El archivo <strong>)rawliteral" +
                     fileName +
                     R"rawliteral(</strong> se eliminó correctamente.</p>
  <a href="/">Volver a la página principal</a>
</main>
</body>
</html>
)rawliteral";
        server.send(200, "text/html", html);
      } else {
        server.send(400, "text/html", "Failed to delete file.");
      }
    } else {
      server.send(400, "text/html", "El archivo no existe.");
    }
  } else {
    server.send(400, "text/html", "No se ha seleccionado un archivo.");
  }
}



void handleChange() {
  if (server.hasArg("cambio")) {
    String cambio = server.arg("cambio");
    if (cambio == "enviar") {
      sendData = true;
    } else {
      sendData = false;
    }
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Readdressando...");
  } else {
    server.send(400, "text/html", "Error: No se ha especificado la acción.");
  }
}



/**
 * Selecciona un archivo de la lista para escribir en la tarjeta.
 */
void handleSelectFile() {
  if (server.hasArg("file")) {
    selectedFile = server.arg("file");
    String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Archivo Seleccionado</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f5;
      color: #333;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 1em;
      text-align: center;
      width: 100%;
    }
    main {
      padding: 20px;
      text-align: center;
    }
    a {
      color: #4CAF50;
      text-decoration: none;
      font-size: 18px;
    }
    a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
<header>
  <h1>Archivo Seleccionado</h1>
</header>
<main>
  <p>El archivo <strong>)rawliteral" +
                     selectedFile +
                     R"rawliteral(</strong> se seleccionó correctamente.</p>
  <a href="/">Volver a la página principal</a>
</main>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  } else {
    server.send(400, "text/html", "No se ha seleccionado un archivo.");
  }
}




/**
 * Muestra el contenido de un archivo.
 */
void handleFileContent() {
  if (server.hasArg("file")) {
    String fileName = server.arg("file");
    if (SPIFFS.exists(fileName)) {
      File file = SPIFFS.open(fileName, "r");
      String content = file.readString();
      file.close();

      String html = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Contenido del Archivo</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f0f0f5;
      color: #333;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 1em;
      text-align: center;
      width: 100%;
    }
    main {
      padding: 20px;
      max-width: 800px;
      text-align: center;
    }
    h1 {
      color: #333;
    }
    pre {
      background-color: #fff;
      border: 1px solid #ccc;
      border-radius: 5px;
      padding: 15px;
      text-align: left;
      max-width: 100%;
      overflow-x: auto;
      white-space: pre-wrap;
    }
    a {
      color: #4CAF50;
      text-decoration: none;
      font-size: 18px;
    }
    a:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
<header>
  <h1>Contenido del Archivo</h1>
</header>
<main>
  <h2>)rawliteral" + fileName + R"rawliteral(</h2>
  <pre>)rawliteral" + content + R"rawliteral(</pre>
  <a href="/list">Volver a la lista de archivos</a>
</main>
</body>
</html>
)rawliteral";

      server.send(200, "text/html", html);
    } else {
      server.send(404, "text/html", "<h1>Error 404</h1><p>Archivo no encontrado.</p><a href='/list'>Volver a la lista</a>");
    }
  } else {
    server.send(400, "text/html", "<h1>Error 400</h1><p>No se ha seleccionado un archivo.</p><a href='/list'>Volver a la lista</a>");
  }
}


/**
 * Lee la tarjeta y escribe el archivo seleccionado si está listo.
 */
void readCard()
{
  Serial.println("file leerTarjeta :" + selectedFile);
  if (selectedFile == "")
    return; // No hay archivo seleccionado

  mfrc522.PCD_AntennaOn();
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return; // No hay tarjeta presente
  mfrc522.PCD_AntennaOff();

  Serial.println("Tarjeta detectada. Escribiendo archivo...");
  writeFileToCard();
}

/**
 * Escribe el archivo seleccionado en la tarjeta mfrc522.
 */
void writeFileToCard()
{
  Serial.println("Entro.");
  File file = SPIFFS.open( selectedFile, "r");
  if (!file)
  {
    Serial.println("Error al abrir archivo para escribir en tarjeta.");
    return;
  }

  byte buffer[16]; // Bloques de 16 bytes
  byte block = 0;

    // Encender la antena para verificar si hay una tarjeta cerca
    mfrc522.PCD_AntennaOn();

    mfrc522.PICC_IsNewCardPresent();
    mfrc522.PICC_ReadCardSerial();

    // Autenticación del bloque
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
      Serial.println("Error de autenticación.");
    }

    block = 0x00;  //para la creacion del archivo

    Serial.print("Crear el archivo");
    buffer[0] = 0x00;

    //es hasta 15 letras
    String titulo = "Titulo";

    // Convertir el String al buffer
    int longitud = selectedFile.length()-1;
    int i = 0;
    while(i<15 && selectedFile[i]!='.' && i < longitud){
      buffer[i+1] = (byte)selectedFile[i+1];
      Serial.print(selectedFile[i+1]);
      i++;
    }
    if(selectedFile[i]=='.'){
      for (int axi = i; axi < sizeof(buffer); axi++){
        buffer[axi] = '\0'; // Rellenar con ceros si es menor a 16 bytes
      }
    }

    Serial.println("Contenido del buffer (Hex):");
    for (int i = 0; i < 16; i++) {
      if (buffer[i] < 0x10) Serial.print("0"); // Asegurar dos dígitos
      Serial.print(buffer[i], HEX); // Imprimir en hexadecimal
      Serial.print(" "); // Espaciado entre valores
    }
    Serial.println(); // Nueva línea al final
  
    // Escribir el bloque
    status = mfrc522.MIFARE_Write(4, buffer, sizeof(buffer));
    if (status != MFRC522::STATUS_OK)
    {
      Serial.println("Error escribiendo en bloque: " + String(4));
    }
    Serial.println("Bloque " + String(4) + " escrito con éxito.");


    // Liberar tarjeta y reiniciar estado
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    mfrc522.PCD_AntennaOff();
    delay(5000);


  while (file.available())
  {
    // Encender la antena para verificar si hay una tarjeta cerca
    mfrc522.PCD_AntennaOn();


    
    size_t bytesRead = file.readBytes((char*)(buffer+2), sizeof(buffer)-2);
    for (size_t i = bytesRead; i < sizeof(buffer) -2; i++)
      buffer[i+2] = '\0'; // Rellenar con ceros si es menor a 16 bytes

    buffer[0]=0x01;   //codigo para la indicar agregar datos en el archivo
    buffer[1]=block;  //Numero de bloque para no guardar dos veces el mismo bloque

    block++;
    
    mfrc522.PICC_IsNewCardPresent();
    mfrc522.PICC_ReadCardSerial();

    // Autenticación del bloque
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
      Serial.println("Error de autenticación.");
    }

    // Escribir el bloque
    status = mfrc522.MIFARE_Write(4, buffer, sizeof(buffer));
    if (status != MFRC522::STATUS_OK)
    {
      Serial.println("Error escribiendo en bloque: " + String(4));
    }
    Serial.println("Bloque " + String(4) + " escrito con éxito.");
    
    
    // Limitar la escritura a bloques disponibles en la tarjeta
    if (block > 63)
    {
      Serial.println("Espacio en tarjeta agotado.");
    }
    
     Serial.println("Contenido del buffer (Hex):");
  for (int i = 0; i < 16; i++) {
    if (buffer[i] < 0x10) Serial.print("0"); // Asegurar dos dígitos
    Serial.print(buffer[i], HEX); // Imprimir en hexadecimal
    Serial.print(" "); // Espaciado entre valores
  }
    Serial.println(); // Nueva línea al final
    // Liberar tarjeta y reiniciar estado
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    mfrc522.PCD_AntennaOff();
    delay(5000);
  }
  file.close();

  //indica que es el final de archivo y se debe cerrar
  buffer[0] = 0x02;
  Serial.print("fin del archivo");
  sendBlock(buffer);
  
  

  selectedFile = ""; // Archivo ya fue escrito
  Serial.println("Archivo escrito completamente en la tarjeta.");
}



// Función para enviar un bloque a la tarjeta
void sendBlock(byte *buffer) {
  // Encender la antena
  mfrc522.PCD_AntennaOn();

  // Verificar si hay una nueva tarjeta
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    Serial.println("No se detectó una tarjeta.");
    mfrc522.PCD_AntennaOff();
    return;
  }

  // Autenticar el bloque
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Error de autenticación: " + String(mfrc522.GetStatusCodeName(status)));
    mfrc522.PCD_AntennaOff();
    return;
  }

  // Escribir en el bloque
  status = mfrc522.MIFARE_Write(4, buffer, 16); // 16 es el tamaño estándar de un bloque
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Error escribiendo en bloque: " + String(4) + " - " + String(mfrc522.GetStatusCodeName(status)));
  } else {
    Serial.println("Bloque " + String(4) + " escrito con éxito.");
  }

  // Liberar tarjeta y reiniciar estado
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  mfrc522.PCD_AntennaOff();
}


void receiveData(){
File archivo;
if(address != ""){
    archivo = SPIFFS.open(address, "a+");
  }
  
  // Verificar si hay una tarjeta cerca

  // Encender la antena para verificar si hay una tarjeta cerca
  mfrc522.PCD_AntennaOn();

  mfrc522.PICC_IsNewCardPresent();
  mfrc522.PICC_ReadCardSerial();

  // Autenticar el bloque 4 (por ejemplo) usando la llave y la tarjeta
  byte block = 4;
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Error de autenticación: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  
  // Leer los datos del bloque 4
  byte buffer[18];
  byte bufferArchivo[16];
  byte bufferSize = sizeof(buffer);
  status = mfrc522.MIFARE_Read(block, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Error leyendo bloque: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  else
  {
    Serial.print("Datos leídos del bloque ");
    Serial.print(block);
    Serial.println(": ");
    for (byte i = 0; i < bufferSize; i++)
    {
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }

    Serial.println(" ");
    
    byte codigo = buffer[0];
    
    //crear el archivo
    if(codigo == 0x00){
      address = "/";
      // Convertir el buffer a String
      for (int i = 1; i < bufferSize-2; i++) {
          if(buffer[i] != 0){
            address += (char)buffer[i];
          }
       }

      address +=".txt";

      Serial.println("Mensaje recibido : "+ address);
      //address = "/mensaje1.txt";
      Serial.println(" ");
      Serial.println("Cargo la address del archivo");
      Serial.println(" ");
    }

    //agregar datos al buffer
    if(codigo == 0x01){

      if(blockR != buffer[1] && address != ""){
        blockR = buffer[1];
        
        String contenido = "";

        // Convertir el buffer a String
        for (int i = 2; i < bufferSize-2; i++) {
            if(buffer[i] != 0){
              contenido += (char)buffer[i];
            }
        }

        Serial.println("Mensaje recibido : "+ contenido);

        archivo.print(contenido);
    
        //archivo.close(); // Siempre cerrar el archivo
    
        Serial.println(" "); 
      }
    }

    //fin de la transmicion de los datos
    if(codigo == 0x02){
      address = "";
      blockR = -1;
      Serial.println(" ");
      Serial.println("fin de la comunicacion");
      Serial.println(" ");
    }
  }

  // Detener la tarjeta
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Apagar la antena nuevamente después de leer la tarjeta
  mfrc522.PCD_AntennaOff();

  Serial.println(" ");
  archivo.close();
}
