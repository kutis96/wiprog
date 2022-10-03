#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <LittleFS.h>

#include "configstore.h"
#include "spiflash.h"
#include "flashprogrammer.h"
#include "wificonfig.h"
#include "logging.h"

ESP8266WebServer server(80);

ConfigStore configStore;

const int ledPin = LED_BUILTIN;
const int targetResetPin = D0;
const int flashSelectPin = D1;

/*
 * Wemos IO: 
 * 
 *  D0 = FPGA nRESET
 *  D5 = SPI SCK
 *  D6 = SPI SDI/MISO
 *  D7 = SPI SDO/MOSI
 *  D1 = SPI CS
 */

String webpage = "";
String hostname;

SPIFlash flash = SPIFlash(flashSelectPin);
FlashProgrammer flashProgrammer = FlashProgrammer(flash);

long lastLedToggle = 0;
void toggleLed() {
  if (millis() - lastLedToggle > 50) {
    lastLedToggle = millis();
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}

void setTargetReset(bool doReset) {
  if (doReset) { //reset is active-low
    pinMode(targetResetPin, OUTPUT);
    digitalWrite(targetResetPin, LOW);
    DEBUG("Target Reset");
  } else {
    pinMode(targetResetPin, INPUT_PULLUP);
    digitalWrite(targetResetPin, HIGH);  //extraneous?
    DEBUG("Target Released");
  }
}

bool getTargetReset() {
  return !digitalRead(targetResetPin); //reset is active-low
}


bool previousResetState = false;

void flashBegin() {
  previousResetState = getTargetReset();
  setTargetReset(true);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(flashSelectPin, OUTPUT);
  SPI.begin();
  flash.begin();
  flash.reset();
}

void flashEnd() {
  pinMode(MOSI, INPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, INPUT);
  pinMode(flashSelectPin, INPUT);
  flash.end();
  SPI.end();
  setTargetReset(previousResetState);
}

void apiGetJID() {
  flashBegin();
  server.send(200, "text/plain", toHex64String(flash.getJedecId(), 24));  //24 bits
  flashEnd();
}

void apiGetUID() {
  flashBegin();
  server.send(200, "text/plain", toHex64String(flash.getUniqueId(), 64));  //64 bits
  flashEnd();
}

void apiGetHostname() {
  server.send(200, "text/plain", hostname);
}

void apiSetTargetReset() {
  setTargetReset(true);
  server.send(200);
}

void apiSetTargetRelease() {
  setTargetReset(false);
  server.send(200);
}

void apiGetTargetReset() {
  server.send(200, "text/plain", getTargetReset() ? "true" : "false");
}

void apiDownloadFlash() {
  int readSize = flash.getSize();

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(readSize);
  server.send(200, "application/octet-stream", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.

  flashBegin();
  flash.busyWait();
  flash.fastReadBegin(0);

  int bufSize = 256;
  char buf[bufSize];
  for (int i = 0; i < readSize; i++) {
    buf[i & 0xFF] = flash.fastReadByte();
    if ((i & 0xFF) == 0xFF) {
      server.sendContent(buf, bufSize);
    }
    toggleLed();
    ESP.wdtFeed();  //wdt reset
  }
  flash.fastReadEnd();
  flashEnd();
}

void apiGetFile() {
  apiReadFileOr404(server.uri());
}

std::map<String, String> mimetypes = {
  std::pair<String, String>(".html", "text/html"),
  std::pair<String, String>(".css", "text/css"),
  std::pair<String, String>(".png", "image/png"),
  std::pair<String, String>(".jpg", "image/jpeg"),
  std::pair<String, String>(".ico", "image/x-icon"),
  std::pair<String, String>(".gif", "image/gif"),
  std::pair<String, String>(".js", "application/javascript"),
  std::pair<String, String>(".bin", "application/octet-stream"),
};

String decodeMimeType(String path) {
  String suffix = ".bin";
  if (path.lastIndexOf('.') != -1) {
    suffix = path.substring(path.lastIndexOf('.'));
  }

  std::map<String, String>::iterator it;
  it = mimetypes.find(suffix);
  if (it != mimetypes.end()) {
    return it->second;
  } else {
    return "application/octet-stream";
  }
}

boolean apiReadFile(String path) {
  toggleLed();

  if(path.indexOf("config") >= 0)
    return false; //do not read config files

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    int readSize = file.size();
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(readSize);

    String mimeType = decodeMimeType(path);
    server.send(200, mimeType, "");

    int bufSize = 256;
    int bytesRead = 0;
    char buf[bufSize];
    do {
      bytesRead = file.readBytes(buf, bufSize);
      server.sendContent(buf, bytesRead);
      toggleLed();
    } while (bytesRead != 0);

    file.close();
    return true;
  } else {
    return false;
  }
}

void apiReadFileOr404(String path) {
  if (!apiReadFile(path)) {
    if (!apiReadFile("404.html")) {
      SendHTML_Header(404);
      webpage += F("<h3>File Not Found</h3>");
      SendHTML_Content();
      SendHTML_Stop();
    }
  }
}

void setup(void) {

  //Do not interfere
  pinMode(MOSI, INPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, INPUT);
  pinMode(flashSelectPin, INPUT);
  setTargetReset(false);  //do not reset

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);

  Serial.begin(2000000);
  
  if (LittleFS.begin()) {
    INFO("LittleFS init ok.");
  } else {
    WARN("LittleFS init failed.");
  }

  INFO("Loading config...");
  configStore.load();
  INFO("Config loaded.");

  hostname = configStore.getString("hostname", "wiprog");
  String wifi_ssid = configStore.getString("wifi-ssid", "SSID");
  String wifi_pass = configStore.getString("wifi-pass", "password");

  configStore.save();

  INFO("Connecting to " + wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_pass);
  INFO("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(wifi_ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started");
    Serial.println("hostname: " + hostname);
  }

  server.on("/", []() {
    apiReadFileOr404("index.html");
  });
  server.on("/api/v1/targetreset", HTTP_GET, apiGetTargetReset);
  server.on("/api/v1/targetreset", HTTP_POST, apiSetTargetReset);
  server.on("/api/v1/targetrelease", HTTP_POST, apiSetTargetRelease);
  server.on("/api/v1/jid", HTTP_GET, apiGetJID);
  server.on("/api/v1/uid", HTTP_GET, apiGetUID);
  server.on("/api/v1/flash.bin", HTTP_GET, apiDownloadFlash);
  server.on("/api/v1/hostname", HTTP_GET, apiGetHostname);
  server.on(
    "/api/v1/flash.bin", HTTP_POST, []() {
      server.send(200);
    },
    []() {
      HTTPUpload& uploadfile = server.upload();  // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/src
                                                 // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
      try {
        if (uploadfile.status == UPLOAD_FILE_START) {
          String filename = uploadfile.filename;
          Serial.print("Uploading file:");
          Serial.println(filename);

          setTargetReset(true);  //reset target
          flashBegin();
          flashProgrammer.begin(0);
        } else if (uploadfile.status == UPLOAD_FILE_WRITE) {
          Serial.print("Received chunk of ");
          Serial.println(uploadfile.currentSize);

          for (uint64_t i = 0; i < uploadfile.currentSize; i++) {
            flashProgrammer.write(uploadfile.buf[i]);
            toggleLed();
          }
        } else if (uploadfile.status == UPLOAD_FILE_END) {
          if (uploadfile.currentSize > 0) {
            Serial.print("Received chunk of ");
            Serial.println(uploadfile.currentSize);

            for (uint64_t i = 0; i < uploadfile.currentSize; i++) {
              flashProgrammer.write(uploadfile.buf[i]);
            }
          }
          flashProgrammer.end();

          SPI.end();
          pinMode(MOSI, INPUT);
          pinMode(MISO, INPUT);
          pinMode(SCK, INPUT);
          pinMode(flashSelectPin, INPUT);

          Serial.print("Upload Size: ");
          Serial.println(uploadfile.totalSize);

          if (!apiReadFile("uploadok.html")) {
            SendHTML_Header(200);
            webpage += F("<h3>File was successfully uploaded</h3>");
            webpage += F("<h2>Uploaded File Name: ");
            webpage += uploadfile.filename + "</h2>";
            webpage += F("<h2>File Size: ");
            webpage += file_size(uploadfile.totalSize) + "</h2><br>";
            SendHTML_Content();
            SendHTML_Stop();
          }
        } else {  //UPLOAD_FILE_ABORTED or unhandled
          SendHTML_Header(499);
          webpage += F("Upload aborted");
          SendHTML_Content();
          SendHTML_Stop();
        }
      } catch (SPIFlashFailure f) {
        SendHTML_Header(500);
        webpage += F("Flash device error\n");
        webpage += f;
        SendHTML_Content();
        SendHTML_Stop();
      }
    });
  server.onNotFound(apiGetFile);

  server.begin();

  Serial.println("HTTP server started");
}
void loop(void) {
  server.handleClient();
}

void SendHTML_Header(int code) {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(code, "text/html", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  webpage = "";
}
void SendHTML_Content() {
  server.sendContent(webpage);
  webpage = "";
}
void SendHTML_Stop() {
  server.sendContent("");
  server.client().stop();  // Stop is needed because no content length was sent
}

void ReportCouldNotCreateFile(String target) {
  SendHTML_Header(403);
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
  webpage += F("<a href='/");
  webpage += target + "'>[Back]</a><br><br>";
  SendHTML_Content();
  SendHTML_Stop();
}

String file_size(int bytes) {
  String fsize = "";
  if (bytes < 1024) fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024)) fsize = String(bytes / 1024.0, 3) + " KiB";
  else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MiB";
  else fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GiB";
  return fsize;
}

String toHex64String(uint64_t n, uint8_t bits) {
  uint8_t nHexChars = (bits / 4) + ((bits % 4 > 0) ? 1 : 0);

  const char hexmap[] = "0123456789ABCDEF";
  String str = "";
  for (int i = nHexChars - 1; i >= 0; i -= 1) {
    str += hexmap[(n >> (4 * i)) & 0x0F];
  }
  return str;
}