#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#define MYSSID "larryb"
#define PASSWD "clownfish"

AsyncWebServer server(80);

boolean wifiConnect(char* host) {
#ifdef MYSSID
  WiFi.begin(MYSSID,PASSWD);
  WiFi.waitForConnectResult();
#else
  WiFi.begin();  
  WiFi.waitForConnectResult();
#endif
  Serial.println(WiFi.localIP());
  return (WiFi.status() == WL_CONNECTED);
}

void handleRoot(AsyncWebServerRequest *request) {
  request->redirect("/update");
}

void handleUpdate(AsyncWebServerRequest *request) {
  char* html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index){
    log_i("Update");
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > 0) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the switch reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}

boolean webInit() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {request->redirect("/update");});
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
  server.on("/doUpdate", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
  );
  server.onNotFound([](AsyncWebServerRequest *request){request->send(404);});
  server.begin();
}

void setup() {
  Serial.begin(115200);
  char host[16];
  snprintf(host, 16, "ESP%012llX", ESP.getEfuseMac());
  if(!wifiConnect(host)) {
    Serial.println("Connection failed");
    return;
  }
  MDNS.begin(host);
  if(webInit()) MDNS.addService("http", "tcp", 80);
  Serial.printf("Ready! Open http://%s.local in your browser\n", host);
}

void loop() {vTaskDelete(NULL);}