#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <LittleFS.h>

#include "spiflash.h"
#include "flashprogrammer.h"
#include "wificonfig.h"

const char* ssid     = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;
const int fpgaResetPin = D0;
const int spiSS        = D1;

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

boolean heldInReset = false;

SPIFlash flash = SPIFlash(spiSS);
FlashProgrammer flashProgrammer = FlashProgrammer(flash);

long lastLedToggle = 0;
void toggleLed() {
  if(millis() - lastLedToggle > 50) {
    lastLedToggle = millis();
    digitalWrite(led, !digitalRead(led));
  }
}

void fpgaReset(bool doReset) {
  if(doReset) {
    pinMode(fpgaResetPin, OUTPUT);
  }else{
    pinMode(fpgaResetPin, INPUT_PULLUP);
  }
  digitalWrite(fpgaResetPin, doReset ? LOW : HIGH);
}

void flashBegin() {
  fpgaReset(true);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(spiSS, OUTPUT);    
  SPI.begin();
  flash.begin();
  flash.reset();
  
}

void flashEnd() {
  pinMode(MOSI, INPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, INPUT);
  pinMode(spiSS, INPUT);
  flash.end();
  SPI.end();
  fpgaReset(false);
}

uint32_t determineFlashSize() {
  flashBegin();
  
  flashEnd();
  return 1 << 24; //TODO
}

void apiGetJID() {
  flashBegin();
  server.send(200, "text/plain", toHex64String(flash.getJedecId()));
  flashEnd();
}

void apiGetUID() {
  flashBegin();
  server.send(200, "text/plain", toHex64String(flash.getUniqueId()));
  flashEnd();
}

void apiSetTargetReset() {
  fpgaReset(true);
  server.send(200);
}

void apiSetTargetRelease() {
  fpgaReset(false);
  server.send(200);  
}

void apiGetTargetReset() {
  server.send(200, "text/plain", digitalRead(fpgaResetPin) ? "false" : "true");
}

void apiDownloadFlash() {
  int readSize = determineFlashSize();
  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(readSize); 
  server.send(200, "application/octet-stream", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  
  flashBegin();
  flash.busyWait();
  flash.fastReadBegin(0);

  int bufSize = 256;
  char buf[bufSize];
  for(int i = 0; i < readSize; i++) {
    buf[i & 0xFF ] = flash.fastReadByte();
    if((i & 0xFF) == 0xFF) {
      server.sendContent(buf, bufSize);
    }
    toggleLed();
    ESP.wdtFeed(); //wdt reset
  }
  flash.fastReadEnd();
  flashEnd();
}

void apiGetFile() {
  apiReadFileOr404(server.uri()); 
}

std::map <String, String> mimetypes = {
  std::pair <String, String> (".html", "text/html"),
  std::pair <String, String> (".css", "text/css"),
  std::pair <String, String> (".png", "image/png"),
  std::pair <String, String> (".jpg", "image/jpeg"),
  std::pair <String, String> (".ico", "image/x-icon"),
  std::pair <String, String> (".gif", "image/gif"),
  std::pair <String, String> (".js", "application/javascript"),
  std::pair <String, String> (".bin", "application/octet-stream"),
};

String decodeMimeType(String path) {
  String suffix = ".bin";
  if(path.lastIndexOf('.') != -1) {
    suffix = path.substring(path.lastIndexOf('.'));
  }

  std::map<String, String>::iterator it;
  it = mimetypes.find(suffix);
  if ( it != mimetypes.end() ) {
    return it->second;
  } else {
    return "application/octet-stream";
  }
}

boolean apiReadFile(String path) {
  toggleLed();
  if(LittleFS.exists(path)) {
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
  }else{
    return false;
  }
}

void apiReadFileOr404(String path) {
  if(!apiReadFile(path)) {
    if(!apiReadFile("404.html")) {
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
  pinMode(spiSS, INPUT);
  fpgaReset(false); //do not reset
  
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  //Serial.begin(115200);
  Serial.begin(2000000);
  
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("wiprog")) {
    Serial.println("MDNS responder started");
  }

  if(LittleFS.begin()){
    Serial.println("FS init ok.");
  }else{
    Serial.println("FS init failed.");
  }

  server.on("/", [](){apiReadFileOr404("index.html");});
  server.on("/upload",   [](){
      SendHTML_Header(200);
      webpage += F("<h3>Select File to Upload</h3>"); 
      webpage += F("<FORM action='/api/v1/flash.bin' method='post' enctype='multipart/form-data'>");
      webpage += F("<input class='buttons' style='width:40%' type='file' name='flash' id = 'flash' value=''><br>");
      webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
      webpage += F("<a href='/'>[Back]</a><br><br>");
      SendHTML_Content();
      SendHTML_Stop();
  });
  server.on("/api/v1/targetreset", HTTP_GET, apiGetTargetReset);
  server.on("/api/v1/targetreset", HTTP_POST, apiSetTargetReset);
  server.on("/api/v1/targetrelease", HTTP_POST, apiSetTargetRelease);
  server.on("/api/v1/jid", HTTP_GET, apiGetJID);
  server.on("/api/v1/uid", HTTP_GET, apiGetUID);
  server.on("/api/v1/flash.bin", HTTP_GET, apiDownloadFlash);  
  server.on("/api/v1/flash.bin", HTTP_POST, [](){ server.send(200);}, [](){
    HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                              // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
    if(uploadfile.status == UPLOAD_FILE_START)
    {
      
      String filename = uploadfile.filename;
      Serial.print("Uploading file:");
      Serial.println(filename);
      
      fpgaReset(true); //reset fpga
      flashBegin();
      flashProgrammer.begin(0);
      
    }
    else if (uploadfile.status == UPLOAD_FILE_WRITE)
    {
      Serial.print("Received chunk of ");
      Serial.println(uploadfile.currentSize);

      for(uint64_t i = 0; i < uploadfile.currentSize; i++) {
        flashProgrammer.write(uploadfile.buf[i]);
        toggleLed();
      }
    } 
    else if (uploadfile.status == UPLOAD_FILE_END)
    {
      if(uploadfile.currentSize > 0) {
        Serial.print("Received chunk of ");
        Serial.println(uploadfile.currentSize);
  
        for(uint64_t i = 0; i < uploadfile.currentSize; i++) {
          flashProgrammer.write(uploadfile.buf[i]);
        }
      }
      flashProgrammer.end();
      
      SPI.end();
      pinMode(MOSI, INPUT);
      pinMode(MISO, INPUT);
      pinMode(SCK, INPUT);
      pinMode(spiSS, INPUT);
      fpgaReset(false); //un-reset fpga
      
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);

      if(!apiReadFile("uploadok.html")){
        SendHTML_Header(200);
        webpage += F("<h3>File was successfully uploaded</h3>"); 
        webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
        webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
        SendHTML_Content();
        SendHTML_Stop();
      }
    }
  });
  server.onNotFound(apiGetFile);
  
  server.begin();

  Serial.println("HTTP server started");
}
void loop(void) {
  server.handleClient();
}

void SendHTML_Header(int code){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(code, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  webpage = "";
}
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}

void ReportCouldNotCreateFile(String target){
  SendHTML_Header(403);
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  SendHTML_Content();
  SendHTML_Stop();
}
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KiB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MiB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GiB";
  return fsize;
}
String toHex64String(uint64_t n) {
  String str = "";
  char hexmap[] = "0123456789ABCDEF";
  for(int i = 64-4; i >= 0; i-=4) {
    str += hexmap[(n >> i) & 0x0F];
  }
  return str;
}
