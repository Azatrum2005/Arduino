#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <UltrasonicSensor.h>
UltrasonicSensor ultrasonic(32, 33);

struct MOTOR_PINS
{
  int pinEn;  
  int pinIN1;
  int pinIN2;    
};
int move=0;
int input=0;

int motorPins[][3] = 
{
  {13, 12, 14},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {13, 15, 0},  //LEFT_MOTOR  Pins (EnB, IN3, IN4)
};

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 500; /* 10 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;

unsigned long lastTime=0;
int distance=0;
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
// SSID and password of Wifi connection:
const char* ssid = "H155-380_198A";
const char* password = "aBAFbt3ALF8";

WebSocketsServer webSocket = WebSocketsServer(81); 

#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void startCameraServer();

void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber][1], HIGH);
    digitalWrite(motorPins[motorNumber][2], LOW);    
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber][1], LOW);
    digitalWrite(motorPins[motorNumber][2], HIGH);     
  }
  else
  {
    digitalWrite(motorPins[motorNumber][1], LOW);
    digitalWrite(motorPins[motorNumber][2], LOW);       
  }
}

void moveCar(int inputValue)
{
  // Serial.printf("Got value as %d\n", inputValue);  
  switch(inputValue)
  {

    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);                  
      break;
  
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
  
    case LEFT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
  
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD); 
      break;
 
    case STOP:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
  
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
  }
}

void setUpPinModes()
{
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);

  for (int i = 0; i < 2; i++)
  {
    pinMode(motorPins[i][0], OUTPUT);    
    pinMode(motorPins[i][1], OUTPUT);
    pinMode(motorPins[i][2], OUTPUT);  

    ledcAttachPin(motorPins[i][0], PWMSpeedChannel);
  }
  ledcWrite(PWMSpeedChannel,0);
}

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
      StaticJsonDocument<200> doc_rx;
      DeserializationError error = deserializeJson(doc_rx, payload);
            
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else if (doc_rx["action"] == "move") {
        move = doc_rx["content"];
        // Serial.printf("input:%d\n",move);
      }
      else if (doc_rx["action"] == "input") {
        input=doc_rx["content"];
        // ledcWrite(PWMSpeedChannel,250);
        // moveCar(input);
        // Serial.printf("input:%d\n",input);
        String jsonString;
        StaticJsonDocument<200> doc_tx;
        doc_tx["action"] = "update_message";
        doc_tx["content"] = doc_rx["content"];
        serializeJson(doc_tx, jsonString);
        webSocket.broadcastTXT(jsonString);
      }
      break;
  }
}

void setup() {
  Serial.begin(115200); 
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 15;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_VGA);

  delay(2000);
  WiFi.begin(ssid, password);
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());

  startCameraServer();
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);

  setUpPinModes();
  int temperature = 25;
  ultrasonic.setTemperature(temperature);
}

void loop() {
  webSocket.loop();
  unsigned long now = millis();
  if(now-lastTime>100.0){
    distance = ultrasonic.distanceInCentimeters();
    Serial.printf("Distance: %dcm\n",distance);
    lastTime = now;
  }
  if(move){
    if(distance>15){
      ledcWrite(PWMSpeedChannel,100+2*distance);
      moveCar(UP);
    }
    else if(distance<10){
      ledcWrite(PWMSpeedChannel,180);
      moveCar(DOWN);
    }
    else{
      moveCar(STOP);
    }
  }
  else{
    ledcWrite(PWMSpeedChannel,250);
    moveCar(input);
  }
}