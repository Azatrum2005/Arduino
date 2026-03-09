#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

struct MOTOR_PINS
{
  int pinEn;  
  int pinIN1;
  int pinIN2;    
};
int motorPins[][3] = 
{
  {13, 12, 14},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {33, 15, 0},  //LEFT_MOTOR  Pins (EnB, IN3, IN4)
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

const int PWMFreq = 500;
const int PWMResolution = 8;
const int PWMSpeedChannelR = 2;
const int PWMSpeedChannelL = 3;

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

#define ADC_Max 4095    // This is the default ADC max value on the ESP32 (12 bit ADC width); 
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
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
// SSID and password of Wifi connection:
const char* ssid = "H155-380_198A";
const char* password = "aBAFbt3ALF8";

WebSocketsServer webSocket = WebSocketsServer(81); 
int inputx=0;
int inputy=0;
int move=0;
int turn=0;
int outputR=0;
int outputL=0;
float ValP;     
float ValI;
float ValD;
float sp;

class PIDController {
  private:
    float kp, ki, kd;              // PID coefficients
    float setpoint;                // Desired value
    float lastError;               // Previous error for derivative calc
    float integral;                // Integral sum
    unsigned long lastTime;        // Last sample time
    float outputMin, outputMax;    // Output limits
    
  public:
    PIDController(float p, float i, float d, float min, float max) {
      kp = p;
      ki = i;
      kd = d;
      outputMin = min;
      outputMax = max;
      lastError = 0;
      integral = 0;
      lastTime = 0;
    }
    
    void setSetpoint(float sp) {
      setpoint = sp;
    }
    
    void resetIntegral() {
      integral = 0;
    }
    
    float compute(float input) {
      // Calculate time delta
      unsigned long now = millis();
      float deltaTime = (now - lastTime) / 1000.0;  // Convert to seconds
      
      if (deltaTime <= 0) {  // Skip first iteration
        lastTime = now;
        return 0;
      }
      
      // Calculate error
      float error = input-setpoint;
      
      // Proportional term
      float P = kp * error;
      
      // Integral term
      integral += error * deltaTime;
      float I = ki * integral;
      
      // Derivative term
      float derivative = (error - lastError) / deltaTime;
      float D = kd * derivative;
      
      // Calculate total output
      float output = P + I + D;
      
      // Apply output limits
      if (output > outputMax) {
        output = outputMax;
      } else if (output < outputMin) {
        output = outputMin;
      }
      
      // Save current values for next iteration
      lastError = error;
      lastTime = now;
      
      return output;
    }
};

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
      else if (doc_rx["action"] == "pid") {
        ValP=doc_rx["content"]["P"];
        ValI=doc_rx["content"]["I"];
        ValD=doc_rx["content"]["D"];
        sp=doc_rx["content"]["sp"];
        Serial.printf("ValP:%.4f",ValP);
        Serial.printf("ValI:%.4f",ValI);
        Serial.printf("ValD:%.4f\n",ValD);
        Serial.printf("Setpoint:%d\n",sp);
      }
      else if (doc_rx["action"] == "dx") {
        inputx=doc_rx["content"];
        Serial.printf("inputx:%d\n",inputx);
        // String jsonString;
        // StaticJsonDocument<200> doc_tx;
        // doc_tx["action"] = "update_message";
        // doc_tx["content"] = doc_rx["content"];
        // serializeJson(doc_tx, jsonString);
        // webSocket.broadcastTXT(jsonString);
      }
      else if (doc_rx["action"] == "dy") {
        inputy=doc_rx["content"];
        Serial.printf("inputy:%d\n",inputy);
        // String jsonString;
        // StaticJsonDocument<200> doc_tx;
        // doc_tx["action"] = "update_message";
        // doc_tx["content"] = doc_rx["content"];
        // serializeJson(doc_tx, jsonString);
        // webSocket.broadcastTXT(jsonString);
      }
      // else if (doc_rx["action"] == "led") {
      //   if(doc_rx["content"]=="on")
      //   digitalWrite(2,1);
      //   if(doc_rx["content"]=="off")
      //   digitalWrite(2,0);
      //   String jsonString;
      //   StaticJsonDocument<200> doc_tx;
      //   doc_tx["action"] = "update_message";
      //   doc_tx["content"] = doc_rx["content"];
      //   serializeJson(doc_tx, jsonString);
      //   webSocket.broadcastTXT(jsonString);
      // }
      break;
  }
}

void setUpPinModes()
{
  ledcSetup(PWMSpeedChannelL, PWMFreq, PWMResolution);
  ledcSetup(PWMSpeedChannelR, PWMFreq, PWMResolution);
  for (int i = 0; i < 2; i++)
  {
    pinMode(motorPins[i][0], OUTPUT);    
    pinMode(motorPins[i][1], OUTPUT);
    pinMode(motorPins[i][2], OUTPUT);  
  }
  ledcAttachPin(motorPins[0][0], PWMSpeedChannelR);
  ledcAttachPin(motorPins[1][0], PWMSpeedChannelL);
  ledcWrite(PWMSpeedChannelL,0);
  ledcWrite(PWMSpeedChannelR,0);
}

void setup() {
  Serial.begin(115200); 
  Serial.setDebugOutput(true);
  Serial.println();

  setUpPinModes();
  // pinMode(2,OUTPUT);

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
    config.jpeg_quality = 10;
    config.fb_count = 3;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
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

}

void loop() {
  webSocket.loop();
  PIDController pid1(1.333*ValP,ValI,ValD, 0, 250);
  pid1.setSetpoint((int)sp);
  PIDController pid2(ValP/1.333,ValI,ValD, 0, 250);
  pid2.setSetpoint((int)sp);

  if (inputy>5){
    move = pid1.compute(inputy);
    moveCar(2);
    Serial.println("Down");
  } 
  if (inputy<-5){
    move = pid1.compute(inputy*(-1));
    moveCar(1);
    Serial.println("Up");
  }
  if (inputx>5){
    turn=(int)(pid2.compute(inputx));
    outputR = move-turn;
    outputL = move+turn;
    // moveCar(4);
    Serial.println("Right");
  } 
  if (inputx<-5){
    turn=(int)(pid2.compute(inputx*(-1)));
    outputR = move+turn;
    outputL = move-turn;
    // moveCar(3);
    Serial.println("Left");
  }  
  if (outputR > 250) {
    outputR = 250;
  } 
  if (outputR < 0) {
    outputR = 0;
  }   
  if (outputL > 250) {
    outputL = 250;
  } 
  if (outputL < 0) {
    outputL = 0;
  }
  if (inputx ==0 && inputy==0){
    ledcWrite(PWMSpeedChannelR,0);
    ledcWrite(PWMSpeedChannelL,0); 
  }
  if(inputy>5){
    ledcWrite(PWMSpeedChannelR,outputR);
    ledcWrite(PWMSpeedChannelL,outputL); 
  }
  if(inputy<-5){
    ledcWrite(PWMSpeedChannelR,outputL);
    ledcWrite(PWMSpeedChannelL,outputR); 
  }
  Serial.printf("move: %d\n",move);  
  Serial.printf("turn: %d\n",turn);     
  Serial.printf("outputR: %d\n",outputR);
  Serial.printf("outputL: %d\n",outputL);
  delay(10);
}