#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "credentials.h"
#include "webinterface.h"

// Pin definitions
#define GPIO_D2 4
#define GPIO_D5 14
#define IR_RECEIVER_PIN GPIO_D5
#define IR_LED_PIN GPIO_D2
#define SERIAL_BAUD_RATE 115200

// IR settings
#define CAPTURE_BUFFER_SIZE 1024
#define MESSAGE_END_TIMEOUT 50
#define IR_LED_FREQUENCY 38000

// Limits
#define MAX_REMOTES 10
#define MAX_BUTTONS_PER_REMOTE 20
#define MAX_NAME_LENGTH 30

// Data structures
struct IRSignal {
  uint16_t* data;
  uint16_t length;
  bool isValid;
};

struct Button {
  char name[MAX_NAME_LENGTH];
  IRSignal signal;
  bool isActive;
};

struct Remote {
  char name[MAX_NAME_LENGTH];
  Button buttons[MAX_BUTTONS_PER_REMOTE];
  int buttonCount;
  bool isActive;
};

// Global variables
Remote remotes[MAX_REMOTES];
int remoteCount = 0;
IRsend irsend(IR_LED_PIN);
IRrecv irrecv(IR_RECEIVER_PIN, CAPTURE_BUFFER_SIZE, MESSAGE_END_TIMEOUT, false);
decode_results irReadingResults;
ESP8266WebServer server(80);

// Recording mode variables
bool recordingMode = false;
int recordingRemoteId = -1;
int recordingButtonId = -1;

// Persistence functions
void saveData() {
  Serial.println("Saving data to LittleFS...");

  DynamicJsonDocument doc(16384); // 16KB buffer
  JsonArray remotesArray = doc.createNestedArray("remotes");

  for (int i = 0; i < remoteCount; i++) {
    if (!remotes[i].isActive) continue;

    JsonObject remoteObj = remotesArray.createNestedObject();
    remoteObj["id"] = i;
    remoteObj["name"] = remotes[i].name;

    JsonArray buttonsArray = remoteObj.createNestedArray("buttons");
    for (int j = 0; j < remotes[i].buttonCount; j++) {
      if (!remotes[i].buttons[j].isActive) continue;

      JsonObject buttonObj = buttonsArray.createNestedObject();
      buttonObj["id"] = j;
      buttonObj["name"] = remotes[i].buttons[j].name;
      buttonObj["hasSignal"] = remotes[i].buttons[j].signal.isValid;

      if (remotes[i].buttons[j].signal.isValid) {
        buttonObj["length"] = remotes[i].buttons[j].signal.length;

        JsonArray signalArray = buttonObj.createNestedArray("data");
        for (int k = 0; k < remotes[i].buttons[j].signal.length; k++) {
          signalArray.add(remotes[i].buttons[j].signal.data[k]);
        }
      }
    }
  }

  File file = LittleFS.open("/remotes.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  } else {
    Serial.println("Data saved successfully");
  }

  file.close();
}

void loadData() {
  Serial.println("Loading data from LittleFS...");

  if (!LittleFS.exists("/remotes.json")) {
    Serial.println("No saved data found");
    return;
  }

  File file = LittleFS.open("/remotes.json", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray remotesArray = doc["remotes"];
  remoteCount = 0;

  for (JsonObject remoteObj : remotesArray) {
    int id = remoteObj["id"];
    const char* name = remoteObj["name"];

    if (id >= 0 && id < MAX_REMOTES) {
      strncpy(remotes[id].name, name, MAX_NAME_LENGTH - 1);
      remotes[id].name[MAX_NAME_LENGTH - 1] = '\0';
      remotes[id].isActive = true;
      remotes[id].buttonCount = 0;

      JsonArray buttonsArray = remoteObj["buttons"];
      for (JsonObject buttonObj : buttonsArray) {
        int btnId = buttonObj["id"];
        const char* btnName = buttonObj["name"];
        bool hasSignal = buttonObj["hasSignal"];

        if (btnId >= 0 && btnId < MAX_BUTTONS_PER_REMOTE) {
          strncpy(remotes[id].buttons[btnId].name, btnName, MAX_NAME_LENGTH - 1);
          remotes[id].buttons[btnId].name[MAX_NAME_LENGTH - 1] = '\0';
          remotes[id].buttons[btnId].isActive = true;
          remotes[id].buttons[btnId].signal.isValid = hasSignal;

          if (hasSignal) {
            uint16_t length = buttonObj["length"];
            remotes[id].buttons[btnId].signal.length = length;

            // Allocate memory for signal data
            remotes[id].buttons[btnId].signal.data = new uint16_t[length];

            JsonArray signalArray = buttonObj["data"];
            int k = 0;
            for (JsonVariant value : signalArray) {
              if (k < length) {
                remotes[id].buttons[btnId].signal.data[k] = value.as<uint16_t>();
                k++;
              }
            }
          } else {
            remotes[id].buttons[btnId].signal.data = nullptr;
            remotes[id].buttons[btnId].signal.length = 0;
          }

          remotes[id].buttonCount = max(remotes[id].buttonCount, btnId + 1);
        }
      }

      remoteCount = max(remoteCount, id + 1);
    }
  }

  Serial.print("Loaded ");
  Serial.print(remoteCount);
  Serial.println(" remotes from storage");
}

// IR helper functions
IRSignal captureIRSignal() {
  IRSignal signal;
  signal.isValid = false;
  signal.data = nullptr;
  signal.length = 0;

  if (irrecv.decode(&irReadingResults)) {
    signal.length = getCorrectedRawLength(&irReadingResults);
    signal.data = resultToRawArray(&irReadingResults);
    signal.isValid = true;
    irrecv.resume();
  }

  return signal;
}

void sendIRSignal(IRSignal signal) {
  if (signal.isValid && signal.data != nullptr && signal.length > 0) {
    irsend.sendRaw(signal.data, signal.length, IR_LED_FREQUENCY);
    delay(50);
  }
}

// Remote control management functions
int addRemote(const char* name) {
  if (remoteCount >= MAX_REMOTES) return -1;

  strncpy(remotes[remoteCount].name, name, MAX_NAME_LENGTH - 1);
  remotes[remoteCount].name[MAX_NAME_LENGTH - 1] = '\0';
  remotes[remoteCount].buttonCount = 0;
  remotes[remoteCount].isActive = true;

  return remoteCount++;
}

int addButton(int remoteId, const char* name) {
  if (remoteId < 0 || remoteId >= remoteCount) return -1;
  if (remotes[remoteId].buttonCount >= MAX_BUTTONS_PER_REMOTE) return -1;

  int buttonId = remotes[remoteId].buttonCount;
  strncpy(remotes[remoteId].buttons[buttonId].name, name, MAX_NAME_LENGTH - 1);
  remotes[remoteId].buttons[buttonId].name[MAX_NAME_LENGTH - 1] = '\0';
  remotes[remoteId].buttons[buttonId].signal.isValid = false;
  remotes[remoteId].buttons[buttonId].signal.data = nullptr;
  remotes[remoteId].buttons[buttonId].signal.length = 0;
  remotes[remoteId].buttons[buttonId].isActive = true;
  remotes[remoteId].buttonCount++;

  return buttonId;
}

// HTTP Handlers
void handleRoot() {
  server.send(200, "text/html", HTML_CONTENT);
}

void handleGetRemotes() {
  DynamicJsonDocument doc(4096);
  JsonArray remotesArray = doc.createNestedArray("remotes");

  for (int i = 0; i < remoteCount; i++) {
    if (remotes[i].isActive) {
      JsonObject remoteObj = remotesArray.createNestedObject();
      remoteObj["id"] = i;
      remoteObj["name"] = remotes[i].name;

      JsonArray buttonsArray = remoteObj.createNestedArray("buttons");
      for (int j = 0; j < remotes[i].buttonCount; j++) {
        if (remotes[i].buttons[j].isActive) {
          JsonObject buttonObj = buttonsArray.createNestedObject();
          buttonObj["id"] = j;
          buttonObj["name"] = remotes[i].buttons[j].name;
          buttonObj["hasSignal"] = remotes[i].buttons[j].signal.isValid;
        }
      }
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleAddRemote() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  const char* name = doc["name"];
  int id = addRemote(name);

  if (id >= 0) {
    saveData(); // Save to flash
    DynamicJsonDocument responseDoc(128);
    responseDoc["success"] = true;
    responseDoc["id"] = id;
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(500, "application/json", "{\"error\":\"Failed to add remote\"}");
  }
}

void handleAddButton() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];
  const char* name = doc["name"];
  int buttonId = addButton(remoteId, name);

  if (buttonId >= 0) {
    saveData(); // Save to flash
    DynamicJsonDocument responseDoc(128);
    responseDoc["success"] = true;
    responseDoc["id"] = buttonId;
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(500, "application/json", "{\"error\":\"Failed to add button\"}");
  }
}

void handleStartRecording() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  recordingRemoteId = doc["remoteId"];
  recordingButtonId = doc["buttonId"];
  recordingMode = true;

  // Clear IR buffer
  irrecv.resume();

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Recording started\"}");
  Serial.println("Modo de gravacao iniciado");
}

void handleStopRecording() {
  recordingMode = false;
  recordingRemoteId = -1;
  recordingButtonId = -1;

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Recording stopped\"}");
  Serial.println("Modo de gravacao parado");
}

void handleSendSignal() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];
  int buttonId = doc["buttonId"];

  if (remoteId >= 0 && remoteId < remoteCount &&
      buttonId >= 0 && buttonId < remotes[remoteId].buttonCount) {

    IRSignal signal = remotes[remoteId].buttons[buttonId].signal;
    if (signal.isValid) {
      sendIRSignal(signal);
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Signal sent\"}");
      Serial.println("Sinal IR enviado");
    } else {
      server.send(400, "application/json", "{\"error\":\"No signal recorded for this button\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid remote or button ID\"}");
  }
}

void handleDeleteRemote() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];

  if (remoteId >= 0 && remoteId < remoteCount) {
    // Free button memory
    for (int i = 0; i < remotes[remoteId].buttonCount; i++) {
      if (remotes[remoteId].buttons[i].signal.data != nullptr) {
        delete[] remotes[remoteId].buttons[i].signal.data;
      }
    }
    remotes[remoteId].isActive = false;
    saveData(); // Save to flash
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid remote ID\"}");
  }
}

void handleEditRemote() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];
  const char* name = doc["name"];

  if (remoteId >= 0 && remoteId < remoteCount && name != nullptr) {
    strncpy(remotes[remoteId].name, name, MAX_NAME_LENGTH - 1);
    remotes[remoteId].name[MAX_NAME_LENGTH - 1] = '\0';
    saveData(); // Save to flash
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
  }
}

void handleDeleteButton() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];
  int buttonId = doc["buttonId"];

  if (remoteId >= 0 && remoteId < remoteCount &&
      buttonId >= 0 && buttonId < remotes[remoteId].buttonCount) {

    if (remotes[remoteId].buttons[buttonId].signal.data != nullptr) {
      delete[] remotes[remoteId].buttons[buttonId].signal.data;
    }
    remotes[remoteId].buttons[buttonId].isActive = false;
    saveData(); // Save to flash
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
  }
}

void handleEditButton() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  int remoteId = doc["remoteId"];
  int buttonId = doc["buttonId"];
  const char* name = doc["name"];

  if (remoteId >= 0 && remoteId < remoteCount &&
      buttonId >= 0 && buttonId < remotes[remoteId].buttonCount &&
      name != nullptr) {

    strncpy(remotes[remoteId].buttons[buttonId].name, name, MAX_NAME_LENGTH - 1);
    remotes[remoteId].buttons[buttonId].name[MAX_NAME_LENGTH - 1] = '\0';
    saveData(); // Save to flash
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
  }
}

void handleGetCSS() {
  server.send(200, "text/css", CSS_CONTENT);
}

void handleGetJS() {
  server.send(200, "application/javascript", JS_CONTENT);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(10);
  Serial.println("\n\nIniciando sistema de controle remoto IR...");

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Erro ao inicializar LittleFS");
    Serial.println("Formatando sistema de arquivos...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("Falha ao formatar LittleFS");
    } else {
      Serial.println("LittleFS formatado e iniciado com sucesso");
    }
  } else {
    Serial.println("LittleFS iniciado com sucesso");
  }

  // Load saved data
  loadData();

  // Initialize IR
  irrecv.enableIRIn();
  irsend.begin();
  Serial.println("IR inicializado");
  Serial.print("Receptor IR no pino: ");
  Serial.println(IR_RECEIVER_PIN);
  Serial.print("Emissor IR no pino: ");
  Serial.println(IR_LED_PIN);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  Serial.print("Conectando ao WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if (MDNS.begin("ir-remote")) {
    Serial.println("mDNS iniciado: http://ir-remote.local");
  }

  // Configure server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/style.css", HTTP_GET, handleGetCSS);
  server.on("/script.js", HTTP_GET, handleGetJS);
  server.on("/api/remotes", HTTP_GET, handleGetRemotes);
  server.on("/api/remote/add", HTTP_POST, handleAddRemote);
  server.on("/api/remote/delete", HTTP_POST, handleDeleteRemote);
  server.on("/api/remote/edit", HTTP_POST, handleEditRemote);
  server.on("/api/button/add", HTTP_POST, handleAddButton);
  server.on("/api/button/delete", HTTP_POST, handleDeleteButton);
  server.on("/api/button/edit", HTTP_POST, handleEditButton);
  server.on("/api/record/start", HTTP_POST, handleStartRecording);
  server.on("/api/record/stop", HTTP_POST, handleStopRecording);
  server.on("/api/signal/send", HTTP_POST, handleSendSignal);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
  Serial.println("Sistema pronto!");

  // Add a sample remote control only if no data was loaded
  if (remoteCount == 0) {
    Serial.println("Nenhum dado salvo encontrado. Criando controle de exemplo...");
    int remoteId = addRemote("Controle TV");
    addButton(remoteId, "Power");
    addButton(remoteId, "Volume +");
    addButton(remoteId, "Volume -");
    saveData(); // Save the example remote
    Serial.println("Controle de exemplo criado e salvo");
  }
}

void loop() {
  server.handleClient();
  MDNS.update();

  // Recording mode
  if (recordingMode) {
    IRSignal signal = captureIRSignal();
    if (signal.isValid) {
      if (recordingRemoteId >= 0 && recordingRemoteId < remoteCount &&
          recordingButtonId >= 0 && recordingButtonId < remotes[recordingRemoteId].buttonCount) {

        // Free previous memory if it exists
        if (remotes[recordingRemoteId].buttons[recordingButtonId].signal.data != nullptr) {
          delete[] remotes[recordingRemoteId].buttons[recordingButtonId].signal.data;
        }

        // Save new signal
        remotes[recordingRemoteId].buttons[recordingButtonId].signal = signal;

        Serial.print("Sinal gravado para: ");
        Serial.print(remotes[recordingRemoteId].name);
        Serial.print(" - ");
        Serial.println(remotes[recordingRemoteId].buttons[recordingButtonId].name);

        // Save to flash
        saveData();

        // Stop recording automatically after capture
        recordingMode = false;
        recordingRemoteId = -1;
        recordingButtonId = -1;
      }
    }
  }

  delay(10);
}
