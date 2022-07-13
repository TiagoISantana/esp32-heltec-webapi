#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <uri/UriBraces.h>

#include <Preferences.h>

#include "logo.h"
#include "heltec.h"

#define LORABAND 433E6 //868E6,915E6
#define LED_PIN 25
#define BUTTON 0

String app_version = "V0.9a";
String api_token = "";

Preferences preferences;

//Wifi connection default value if nothing is stored
String ssid = "SSID";
String password = "password";

//Hot spot configuration credentials
const char *ap_ssid     = "ESP32-API";
const char *ap_password = "854622147";

//LoRa variable to store incoming messages
String lora_resp;

//Start api on default port | TODO: SSL?
WebServer server(80);

//This handle is responsible for base URI respoonse
void handleRoot() {
 
  server.send(200, "application/json", "{\"status\":true,\"loraband\":\""+(String)LORABAND+"\",\"version\":\""+app_version+"\"}");
}

//Handle the builtin led toogle
void handleToogleLed() {

  if(digitalRead(LED_PIN) == 0)
  {
    digitalWrite(LED_PIN, HIGH);
  }else{
    digitalWrite(LED_PIN, LOW);
  }

  char device_state = digitalRead(LED_PIN);
  server.send(200, "application/json", "{\"status\":true,\"data\":\"\"}");
  
}

//Handle the builtin led turn off
void handleOffLed() {

  digitalWrite(LED_PIN, LOW);
  server.send(200, "application/json", "{\"status\":true}");
  
}

//Handle the builtin led turn on
void handleOnLed() {

  digitalWrite(LED_PIN, HIGH);
  server.send(200, "application/json", "{\"status\":true}");
  
}

//Handle the get buitin LED state
void handleStateLed() {

  String response_data =  "{\"status\":true,\"data\":";
  String end_response_data = "}";
  int device_state = digitalRead(LED_PIN);
  String json_out = response_data + device_state + end_response_data;
  
  server.send(200, "application/json",json_out);
  
}

//This handle is responsible for displaying api request top text on the screen
void handleScreen() {

  String message = server.pathArg(0);
  
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawStringMaxWidth(0, 0, 128, message);
  Heltec.display->display();
  
  server.send(200, "application/json", "{\"status\":true,\"data\":\""+ message +"\"}");
  
}

//Get the request from api and send via LORA
void handleLora() {

  String lora = server.pathArg(0);
  
  LoRa.beginPacket();
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.print(lora);
  LoRa.endPacket();
  
  server.send(200, "application/json", "{\"status\":true,\"data\":\"Sent: "+lora+"\"}");
  
}

//Handle lora recieve message
void handleLoraGet() {
  server.send(200, "application/json", "{\"status\":true,\"data\":\""+lora_resp+"\"}");
}

//Set display default logo
void handleLogo() {
  Heltec.display->clear();
  Heltec.display->drawXbm(0,0,logo_width,logo_height,logo_bits);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawStringMaxWidth(0, 0, 128, app_version);
  Heltec.display->drawStringMaxWidth(50, 0, 128, WiFi.localIP().toString().c_str());
  Heltec.display->display();
}

//Handle the logo endpoint
void handleWebLogo() {
  handleLogo();
  server.send(200, "application/json", "{\"status\":true,\"data\":\""+lora_resp+"\"}");
}

//Handle unknow api's endpoint's
void handleNotFound() {
  server.send(404, "application/json", "{\"status\":false,\"data\":\"endpoint not found\"}");
}

//Handle for storing SSID and password
void handleConfig() {

  String ussid = server.arg("ssid");
  String upsw = server.arg("password");

  preferences.begin("api-config", false);
  preferences.putString("ssid", ussid); 
  preferences.putString("password", upsw);
  preferences.end();

  server.send(200, "text/html; charset=UTF-8", "");

  //Once configured, restar de device
  ESP.restart();
  
}

//Handle for configuration mode
void handleConfigHome() {

 preferences.begin("api-config", false);
  server.send(200, "text/html; charset=UTF-8", "<!DOCTYPE html><html lang='en'><head><meta name='viewport' content='width=device-width, initial-scale=1.0'/><meta charset='UTF-8'><title>Configure</title></head><body><form method='get' action='/config/'><input name='ssid' type='text' value='SSID' /><input name='password' type='password' value='password'/><input type='submit' value='Configure' /></form></body></html>" );
  preferences.end();
}



//Load configurantion
void setup(void) {

  //Load saved SSID and password
  preferences.begin("api-config", false); 
  ssid = preferences.getString("ssid", ""); 
  password = preferences.getString("password", "");
  preferences.end();
  
  //Setup built-in LED
  pinMode(LED_PIN, OUTPUT);
  //Setup programable button
  pinMode(BUTTON, INPUT);

  //Configure heltec lib (Display,LoRa,Serial,PABOOST,LoRA BAND)
   Heltec.begin(true,true,false,true,LORABAND);

  //Check if programable button is pressed
  if(digitalRead(BUTTON) == LOW){

    WiFi.softAP(ap_ssid, ap_password);
    IPAddress IP = WiFi.softAPIP();

    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawStringMaxWidth(0, 0, 128, "Connect to:");
    Heltec.display->drawStringMaxWidth(60, 0, 128, IP.toString().c_str());
    Heltec.display->drawString(0, 20, "AP SSID:");
    Heltec.display->drawString(60, 20, ap_ssid);
    Heltec.display->drawString(0, 30, "Password:");
    Heltec.display->drawString(60, 30, ap_password);
    Heltec.display->display();

    server.on("/", handleConfigHome);
    server.on("/config/", handleConfig);

  }else{
   
    //Connect to wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin((char*) ssid.c_str(), (char*) password.c_str());

    //Wait to connect on wifi
    while (WiFi.status() != WL_CONNECTED) {
      Heltec.display->clear();
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->drawStringMaxWidth(0, 0, 128, "Unable to connect to your wifi, please restart your device with the top button pressed to configure your device.");
      Heltec.display->display(); 
      delay(500);
    }

    //Enable cors so webapplication can connect to the API
    server.enableCORS();

    //Routes to the api endpoints
    server.on("/", handleRoot);
    server.on(UriBraces("/api/led/toogle/{}"),handleToogleLed);
    server.on(UriBraces("/api/led/off/{}"),handleOffLed);
    server.on(UriBraces("/api/led/on/{}"),handleOnLed);
    server.on(UriBraces("/api/led/state/{}"),handleStateLed);
    server.on(UriBraces("/api/screen/{}"),handleScreen);
    server.on(UriBraces("/api/lora/{}"),handleLora);
    server.on(UriBraces("/api/loraget/"),handleLoraGet);
    server.on(UriBraces("/api/logo/"),handleWebLogo);
    server.onNotFound(handleNotFound);
     
    //Show logo and IP
    handleLogo();
    
  }

  //Wait for board
  if (MDNS.begin("esp32")) {}

  //Start WebServer port 80(Default)
    server.begin();
  
}


//Run software
void loop(void) {

  //Get client request
  server.handleClient();

  //Get LoRa packet
  int packetSize = LoRa.parsePacket();

  //Check if packet is complete
  if (packetSize) {

    //Clear Response
    lora_resp = "";

    //Add all words to the response
    while (LoRa.available()) {
      lora_resp += (char)LoRa.read();
    }
    
  }

  //Wait for webclient
  delay(2);
  
}
