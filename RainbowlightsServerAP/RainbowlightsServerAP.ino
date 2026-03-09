#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"

#define LEDS_COUNT  8
#define LEDS_PIN	2
#define CHANNEL		0

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, CHANNEL, TYPE_GRB);
uint8_t m_color[5][3] = { {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255} };
// SSID and password of Wifi connection:
const char* ssid_AP = "ESP32";
const char* password_AP= "esp32Mur";

IPAddress local_IP(192,168,1,100);//Set the IP address of ESP32 itself
IPAddress gateway(192,168,1,10);   //Set the gateway of ESP32 itself
IPAddress subnet(255,255,255,0);  //Set the subnet mask for ESP32 itself

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String webpage="<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1.0'> <title>LedToggle</title> <style> div{ display: flex; flex-direction: column; place-items: center; margin-top: 8%; } #ledstate{ background-color:darkgray; border-radius:10px; border:2px black dashed; padding:5px; text-align:center; font-size:15px; } #led{ background-color:darkgray; border-radius:30px; border:10px black double; padding:10px; text-align:center; font-size:25px; } .switch { place-items:initial; position: relative; width: 210px; height: 50px; box-sizing: border-box; padding: 3px; background: #0d0d0d; border-radius: 6px; box-shadow: inset 0 1px 1px 1px rgba(0, 0, 0, 0.5), 0 1px 0 0 rgba(255, 255, 255, 0.1); } .switch input[type='checkbox'] { position: absolute; z-index: 1; width: 100%; height: 100%; opacity: 0; cursor: pointer; } .switch input[type='checkbox'] + label { position: relative; display: block; left: 0; width: 50%; height: 100%; background: #1b1c1c; border-radius: 3px; box-shadow: inset 0 1px 0 0 rgba(255, 255, 255, 0.1); transition: all 0.5s ease-in-out; } .switch input[type='checkbox'] + label:before { content: ''; display: inline-block; width: 5px; height: 5px; margin-left: 10px; background: #fff; border-radius: 50%; vertical-align: middle; box-shadow: 0 0 5px 2px rgba(165, 15, 15, 0.9), 0 0 3px 1px rgba(165, 15, 15, 0.9); transition: all 0.5s ease-in-out; } .switch input[type='checkbox'] + label:after { content: ''; display: inline-block; width: 0; height: 100%; vertical-align: middle; } .switch input[type='checkbox'] + label i { display: block; position: absolute; top: 50%; left: 50%; width: 3px; height: 24px; margin-top: -12px; margin-left: -1.5px; border-radius: 2px; background: #0d0d0d; box-shadow: 0 1px 0 0 rgba(255, 255, 255, 0.3); } .switch input[type='checkbox'] + label i:before, .switch input[type='checkbox'] + label i:after { content: ''; display: block; position: absolute; width: 100%; height: 100%; border-radius: 2px; background: #0d0d0d; box-shadow: 0 1px 0 0 rgba(255, 255, 255, 0.3); } .switch input[type='checkbox'] + label i:before { left: -7px; } .switch input[type='checkbox'] + label i:after { left: 7px; } .switch input[type='checkbox']:checked + label { left: 50%; } .switch input[type='checkbox']:checked + label:before { box-shadow: 0 0 5px 2px rgba(15, 165, 70, 0.9), 0 0 3px 1px rgba(15, 165, 70, 0.9); } </style></head><body style='background-color: rgb(72, 72, 72);'> <div><div id='led'><b>LED SWITCH<b></div> <div class='switch'> <input id='toggle' type='checkbox' /> <label class='toggle' for='toggle'> <i></i> </label> </div> <div id='ledstate'></div> </div></body><script> var Socket; let ledstate=0; document.getElementById('toggle').addEventListener('change',state); function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { }; } function state(){ if(ledstate==0){ ledstate=1; console.log(1); document.getElementById('ledstate').innerHTML =` Lights ON `; var msg = { send: 1 }; Socket.send(JSON.stringify(msg)); } else{ ledstate=0; console.log(0); document.getElementById('ledstate').innerHTML =` Lights OFF `; var msg = { send:0 }; Socket.send(JSON.stringify(msg)); } } window.onload = function(event) { init(); } </script></html>";
StaticJsonDocument<200> doc_rx;
String ledstate;

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:
       DeserializationError error = deserializeJson(doc_rx, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
        const int g_send = doc_rx["send"];
        Serial.println("Recieved: " + String(g_send));
        ledstate=String(g_send);
      }
      Serial.println("");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  strip.begin(); 
  strip.setBrightness(200);                
  delay(2000);
  Serial.println("Setting soft-AP configuration ... ");
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.println("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid_AP, password_AP);
  if(result){
    Serial.println("Ready");
    Serial.println(String("Soft-AP IP address = ") + WiFi.softAPIP().toString());
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
  }else{
    Serial.println("Failed!");
  }
  Serial.println("Setup End");

  server.on("/",[](){
    server.send(200,"text\html",webpage);
  });
  server.begin();
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent); 
}

void loop() {
  server.handleClient();
  webSocket.loop();
  if(ledstate=="1"){
    strip.setBrightness(200);
    for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			strip.setLedColorData(i, m_color[j][0], m_color[j][1], m_color[j][2]);
			strip.show();
      strip.setLedColorData(i+4, m_color[4-j][0], m_color[4-j][1], m_color[4-j][2]);
			strip.show();
			delay(40);
		}
    delay(30);
	}
  }
  if(ledstate=="0"){
    for (int i = 0; i < 8; i++) {
			strip.setLedColorData(i, 0,0,0);
			strip.show();
    }
  }
}
