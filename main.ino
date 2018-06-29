#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <LiquidCrystal_I2C.h>

#define CS  13
#define INC 12
#define UD  14
#define PWR 7

const char* ssid = "ESP8266_AP";
const char* password = "changemeplease";
unsigned long previousMillis = 0;
const long interval = 2000;
unsigned long startMillis;
String webString = "";
int ledon = 0;
int powerLevel = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer server(80);




void setup(void){
  Serial.begin(115200);

  startMillis = millis();

  //set-up the custom IP address
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  lcd.begin();
  lcd.home();
  lcd.print(myIP);
   
  server.on("/", handle_root);
  server.on("/update", HTTP_POST, handleUpdate);
  
  server.begin();
  
  Serial.println("HTTP server started");

  // CS LOW is active
  pinMode(CS, OUTPUT);
  digitalWrite(CS, LOW); // ON

  // INC - LOW is active
  pinMode(INC, OUTPUT);
  digitalWrite(INC, HIGH); // OFF

  // UP is high, DOWN is low
  pinMode(UD, OUTPUT);

  resetResistance();
}

void loop(void){
  unsigned long currentMillis;
  server.handleClient();
} 




// resets

void resetResistance(){
  for (int i=0; i <= 90; i++){
    digitalWrite(UD, HIGH);
    digitalWrite(INC, LOW);
    delay(10);
    digitalWrite(INC, HIGH);
  }

  powerLevel = 0;

  saveResistance();
}




// helpers

void saveResistance(){
  digitalWrite(INC, HIGH);
  delayMicroseconds(1);
  digitalWrite(CS, HIGH);
  delay(20);
  digitalWrite(CS, LOW);
}

void processResistanceChange(String resistanceType){
  boolean canChange = false;

  if(((resistanceType == "decrease") && (powerLevel + 10) <= 100) 
      || ((resistanceType == "increase") && (powerLevel - 10) >= 0)){

    canChange = true;
  }

  if(canChange){
    for (int i=0; i <= 5; i++){
      digitalWrite(UD, resistanceType == "increase" ? HIGH : LOW);
      digitalWrite(INC, LOW);
      delay(10);
      digitalWrite(INC, HIGH);
      delay(10);
    }
  
    if(resistanceType == "increase") {
        powerLevel = powerLevel - 10;
    } else {
        powerLevel = powerLevel + 10;
    }
  }
}




// RESTful request handlers

void handleUpdate(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  const char* resistanceType = root["resistanceType"];

  processResistanceChange(resistanceType);

  String message = "{\"powerLevel\": \""+ String(powerLevel) +"\"}";
  server.send(200, "application/json", message);
}


void handle_root() {
  String page = getHTML();
  
  server.send(200, "text/html", page);
  delay(100);
}




// HTML builders

String getHTML() {
  String html = "<!DOCTYPE html>\
                <html>\
                  <head>\
                    <title>Razor Quad remote control</title>\
                    "+ buttonEventHandlerString() +"\
                    </head>\
                  <body>\
                    <p id='powerLevel'>"+ powerLevel +"</p>\
                    <button onClick=\"sendResistanceUpdate('increase')\">Slower</button>\
                    <button onClick=\"sendResistanceUpdate('decrease')\">Faster</button>\
                  </body>\
                </html>";
  
  return html;
}


String buttonEventHandlerString() {
  String buttonHandler = "<script>\
                            function sendResistanceUpdate(type) {\
                              var xhr = new XMLHttpRequest();\
                              xhr.open('POST', 'update');\
                              xhr.onload = function() {\
                                  var returnObj;\
                                  if (xhr.status === 200) {\
                                    returnObj = JSON.parse(xhr.responseText);\
                                    document.getElementById('powerLevel').innerHTML = returnObj.powerLevel;\
                                  }\
                              };\
                              xhr.send('{resistanceType: '+type+'}');\
                            }\
                         </script>";

  return buttonHandler;
}
