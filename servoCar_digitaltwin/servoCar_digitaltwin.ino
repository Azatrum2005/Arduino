#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <ESP32Servo.h> 
#include "esp_camera.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#define INTERRUPT_PIN 4  // use pin 2 on Arduino Uno & most boards
#define LED_PIN 2 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}
MPU6050_6Axis_MotionApps20 mpu;
#define SDA 13
#define SCL 14
MPU6050 mpu6050(Wire);//Attach the IIC
int16_t ax,ay,az;//define acceleration values of 3 axes
int16_t gx,gy,gz;//define variables to save the values in 3 axes of gyroscope
long timer = 0;

void getMotion6(void){
  ax=mpu6050.getRawAccX();//gain the values of X axis acceleration raw data
  ay=mpu6050.getRawAccY();//gain the values of Y axis acceleration raw data
  az=mpu6050.getRawAccZ();//gain the values of Z axis acceleration raw data
  gx=mpu6050.getRawGyroX();//gain the values of X axis Gyroscope raw data
  gy=mpu6050.getRawGyroY();//gain the values of Y axis Gyroscope raw data
  gz=mpu6050.getRawGyroZ();//gain the values of Z axis Gyroscope raw data
}

Servo servoL;
Servo servoR;
int servoPinL = 25;      // GPIO pin used to connect the servo control (digital out)
int servoPinR = 26;
int speedL = 1500;
int speedR = 1500;

// SSID and password of Wifi connection:
const char* ssid_AP = "ESP32";
const char* password_AP= "esp32Mur";
IPAddress local_IP(192,168,1,100);//Set the IP address of ESP32 itself
IPAddress gateway(192,168,1,10);   //Set the gateway of ESP32 itself
IPAddress subnet(255,255,255,0);  //Set the subnet mask for ESP32 itself

WebSocketsServer webSocket = WebSocketsServer(81); 

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
        speedL=doc_rx["speedL"];
        speedR=doc_rx["speedR"];
        Serial.printf("speedL:%d\n",speedL);
        Serial.printf("speedR:%d\n",speedR);
        servoL.writeMicroseconds(speedL);
        servoR.writeMicroseconds(speedR);
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

void setup() {
  Serial.begin(115200); 
  Serial.setDebugOutput(true);
  Serial.println();
  
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
  
  webSocket.begin();           // start websocket
  webSocket.onEvent(webSocketEvent);
  delay(1000);
  
  Wire.begin(SDA, SCL, 400000);          //attach the IIC pin
  mpu6050.begin();               //initialize the MPU6050
  mpu6050.calcGyroOffsets(true);
  // pinMode(2,OUTPUT);
  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // // wait for ready
  // Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  // while (Serial.available() && Serial.read()); // empty buffer
  // while (!Serial.available());                 // wait for data
  // while (Serial.available() && Serial.read()); // empty buffer again

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXAccelOffset(-1620);
  mpu.setYAccelOffset(110);
  mpu.setZAccelOffset(1660);
  mpu.setXGyroOffset(24);
  mpu.setYGyroOffset(110);
  mpu.setZGyroOffset(22);

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
      // Calibration Time: generate offsets and calibrate our MPU6050
      mpu.CalibrateAccel(6);
      mpu.CalibrateGyro(6);
      mpu.PrintActiveOffsets();
      // turn on the DMP, now that it's ready
      Serial.println(F("Enabling DMP..."));
      mpu.setDMPEnabled(true);

      // enable Arduino interrupt detection
      Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
      Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
      Serial.println(F(")..."));
      attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
      mpuIntStatus = mpu.getIntStatus();

      // set our DMP Ready flag so the main loop() function knows it's okay to use it
      Serial.println(F("DMP ready! Waiting for first interrupt..."));
      dmpReady = true;

      // get expected DMP packet size for later comparison
      packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
      // ERROR!
      // 1 = initial memory load failed
      // 2 = DMP configuration updates failed
      // (if it's going to break, usually the code will be 1)
      Serial.print(F("DMP Initialization failed (code "));
      Serial.print(devStatus);
      Serial.println(F(")"));
  }

  // configure LED for output
  pinMode(LED_PIN, OUTPUT);

  servoL.setPeriodHertz(50);// Standard 50hz servo   
  servoR.setPeriodHertz(50);
  servoL.attach(servoPinL, 900, 2100);   
  servoR.attach(servoPinR, 900, 2100);
  servoL.writeMicroseconds(speedL);
  servoR.writeMicroseconds(speedR);  
}

void loop() {
  webSocket.loop();

  // mpu6050.update();            //update the MPU6050
  // getMotion6();                //gain the values of Acceleration and Gyroscope value
  // Serial.print((float)ax / 16384); Serial.print("g\t");
  // Serial.print((float)ay / 16384); Serial.print("g\t");
  // Serial.print((float)az / 16384); Serial.print("g\t");
  // Serial.print((float)gx / 131); Serial.print("d/s \t");
  // Serial.print((float)gy / 131); Serial.print("d/s \t");
  // Serial.print((float)gz / 131); Serial.print("d/s \n");
  // Serial.print("AccX:"+String(mpu6050.getAccX()));   // g
  // Serial.print("\t");
  // Serial.print("AccAngleX:"+String(mpu6050.getAccAngleX()));
  // Serial.print("\t");
  // Serial.print("AngleX:"+String(mpu6050.getAngleX()));
  // Serial.print("\t");
  // Serial.print("AccY:"+String(mpu6050.getAccY()));   // g
  // Serial.print("\t");
  // Serial.print("AccAngleY:"+String(mpu6050.getAccAngleY()));
  // Serial.print("\t");
  // Serial.print("AngleY:"+String(mpu6050.getAngleY()));
  // Serial.print("\t");
  // Serial.print("AccZ:"+String(mpu6050.getAccZ()));   // g
  // Serial.print("\t");
  // Serial.print(mpu6050.getAngleZ());
  // Serial.print("\t");
  // Serial.println("GyroZ:"+String(mpu6050.getGyroZ())); // deg/s

  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet 
      String jsonString;
      StaticJsonDocument<200> doc_tx;
      doc_tx["data"] = "imu";

      // display quaternion values in easy matrix form: w x y z
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      Serial.print("quat\t\n");
      // Serial.print(q.w);
      doc_tx["qw"] = q.w;
      // Serial.print("\t");
      // Serial.print(q.x);
      doc_tx["qx"] = q.x;
      // Serial.print("\t");
      // Serial.print(q.y);
      doc_tx["qy"] = q.y;
      // Serial.print("\t");
      // Serial.println(q.z);
      doc_tx["qz"] = q.z;;

      // mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetAccel(&aa, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
      Serial.print("areal\t\n");
      // Serial.print((float)aaReal.x/16384.0f);
      doc_tx["ax"] = (float)aaReal.x / 16384.0f;
      // Serial.print("\t");
      // Serial.print((float)aaReal.y/16384.0f);
      doc_tx["ay"] = (float)aaReal.y / 16384.0f;
      // Serial.print("\t");
      // Serial.println((float)aaReal.z/16384.0f);
      doc_tx["az"] = (float)aaReal.z / 16384.0f;

      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);

      doc_tx["gz"] = mpu6050.getGyroZ();

      serializeJson(doc_tx, jsonString);
      webSocket.broadcastTXT(jsonString);
  }

  // String jsonString;
  // StaticJsonDocument<200> doc_tx;
  // doc_tx["data"] = "imu";

  // doc_tx["ax"] = (float)ax / 16384;
  // doc_tx["ay"] = (float)ay / 16384;
  // doc_tx["az"] = (float)az / 16384;
  // doc_tx["gx"] = (float)gx / 131;
  // doc_tx["gy"] = (float)gy / 131;
  // doc_tx["gz"] = (float)gz / 131;

  // doc_tx["ax"] = mpu6050.getAccX();
  // doc_tx["aangx"] = mpu6050.getAccAngleX();
  // doc_tx["angx"] = mpu6050.getAngleX();
  // doc_tx["ay"] = mpu6050.getAccY();
  // doc_tx["aangy"] = mpu6050.getAccAngleY();
  // doc_tx["angy"] = mpu6050.getAngleY();
  // doc_tx["gz"] = mpu6050.getGyroZ();

  // doc_tx["ax"] = (float)aaReal.x / 16384.0f;
  // doc_tx["ay"] = (float)aaReal.y / 16384.0f;
  // doc_tx["az"] = (float)aaReal.z / 16384.0f;
  // doc_tx["qw"] = q.w;
  // doc_tx["qx"] = q.x;
  // doc_tx["qy"] = q.y;
  // doc_tx["qz"] = q.z;

  // serializeJson(doc_tx, jsonString);
  // webSocket.broadcastTXT(jsonString);

  delay(10);
}

// #include <WiFi.h>
// #include <WebServer.h>
// #include <WebSocketsServer.h>
// #include <ArduinoJson.h>
// #include <Wire.h>
// #include <ESP32Servo.h>
// #include "I2Cdev.h"
// #include "MPU6050_6Axis_MotionApps20.h"

// #define INTERRUPT_PIN 4
// #define LED_PIN 2
// #define SDA 13
// #define SCL 14

// // MPU control/status vars
// bool dmpReady = false;
// uint8_t mpuIntStatus;
// uint8_t devStatus;
// uint16_t packetSize;
// uint16_t fifoCount;
// uint8_t fifoBuffer[64];

// // orientation/motion vars
// Quaternion q;
// VectorInt16 aa;
// VectorInt16 aaReal;
// VectorFloat gravity;

// volatile bool mpuInterrupt = false;
// void dmpDataReady() {
//     mpuInterrupt = true;
// }

// MPU6050_6Axis_MotionApps20 mpu;

// Servo servoL;
// Servo servoR;
// int servoPinL = 25;
// int servoPinR = 26;
// int speedL = 1500;
// int speedR = 1500;

// const char* ssid_AP = "ESP32";
// const char* password_AP= "esp32Mur";
// IPAddress local_IP(192,168,1,100);
// IPAddress gateway(192,168,1,10);
// IPAddress subnet(255,255,255,0);

// WebSocketsServer webSocket = WebSocketsServer(81);

// void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
//   switch (type) {
//     case WStype_DISCONNECTED:
//       Serial.println("Client " + String(num) + " disconnected");
//       break;
//     case WStype_CONNECTED:
//       Serial.println("Client " + String(num) + " connected");
//       break;
//     case WStype_TEXT:
//       StaticJsonDocument<200> doc_rx;
//       DeserializationError error = deserializeJson(doc_rx, payload);
      
//       if (error) {
//         Serial.print(F("deserializeJson() failed: "));
//         Serial.println(error.f_str());
//         return;
//       }
//       else if (doc_rx["action"] == "move") {
//         speedL = doc_rx["speedL"];
//         speedR = doc_rx["speedR"];
//         servoL.writeMicroseconds(speedL);
//         servoR.writeMicroseconds(speedR);
//       }
//       break;
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
  
//   // WiFi Setup
//   Serial.println("Setting soft-AP configuration...");
//   WiFi.disconnect();
//   WiFi.mode(WIFI_AP);
//   WiFi.softAPConfig(local_IP, gateway, subnet);
  
//   boolean result = WiFi.softAP(ssid_AP, password_AP);
//   if(result){
//     Serial.println("AP Ready");
//     Serial.println("IP: " + WiFi.softAPIP().toString());
//   }
  
//   webSocket.begin();
//   webSocket.onEvent(webSocketEvent);
  
//   // MPU6050 Setup
//   Wire.begin(SDA, SCL, 400000);
  
//   Serial.println(F("Initializing MPU6050..."));
//   mpu.initialize();
//   pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  
//   if (!mpu.testConnection()) {
//     Serial.println(F("MPU6050 connection failed!"));
//     while(1); // Halt if MPU not found
//   }
//   Serial.println(F("MPU6050 connected"));
  
//   // Initialize DMP
//   Serial.println(F("Initializing DMP..."));
//   devStatus = mpu.dmpInitialize();
  
//   // Gyro offsets (calibrate these for your sensor)
//   mpu.setXAccelOffset(-1620);
//   mpu.setYAccelOffset(110);
//   mpu.setZAccelOffset(1660);
//   mpu.setXGyroOffset(24);
//   mpu.setYGyroOffset(110);
//   mpu.setZGyroOffset(22);
  
//   if (devStatus == 0) {
//     mpu.CalibrateAccel(6);
//     mpu.CalibrateGyro(6);
//     mpu.PrintActiveOffsets();
    
//     Serial.println(F("Enabling DMP..."));
//     mpu.setDMPEnabled(true);
    
//     attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
//     mpuIntStatus = mpu.getIntStatus();
    
//     Serial.println(F("DMP ready!"));
//     dmpReady = true;
//     packetSize = mpu.dmpGetFIFOPacketSize();
//   } else {
//     Serial.print(F("DMP Init failed (code "));
//     Serial.print(devStatus);
//     Serial.println(F(")"));
//     while(1); // Halt on DMP failure
//   }
  
//   pinMode(LED_PIN, OUTPUT);
  
//   // Servo Setup
//   servoL.setPeriodHertz(50);
//   servoR.setPeriodHertz(50);
//   servoL.attach(servoPinL, 900, 2100);
//   servoR.attach(servoPinR, 900, 2100);
//   servoL.writeMicroseconds(speedL);
//   servoR.writeMicroseconds(speedR);
  
//   Serial.println("Setup complete!");
// }

// unsigned long lastSendTime = 0;
// const unsigned long sendInterval = 10; // Send every 10ms (100Hz)

// void loop() {
//   webSocket.loop();
  
//   if (!dmpReady) return;
  
//   // Check if we have new data
//   if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    
//     // Only send at controlled rate
//     unsigned long currentTime = millis();
//     if (currentTime - lastSendTime >= sendInterval) {
//       lastSendTime = currentTime;
      
//       // Get quaternion
//       mpu.dmpGetQuaternion(&q, fifoBuffer);
      
//       // Get gravity-free acceleration
//       mpu.dmpGetAccel(&aa, fifoBuffer);
//       mpu.dmpGetGravity(&gravity, &q);
//       mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
      
//       // Get gyro data
//       int16_t gx, gy, gz;
//       mpu.getRotation(&gx, &gy, &gz);
      
//       // Create JSON
//       StaticJsonDocument<256> doc_tx;
//       doc_tx["data"] = "imu";
//       doc_tx["qw"] = q.w;
//       doc_tx["qx"] = q.x;
//       doc_tx["qy"] = q.y;
//       doc_tx["qz"] = q.z;
//       doc_tx["ax"] = (float)aaReal.x / 16384.0f;
//       doc_tx["ay"] = (float)aaReal.y / 16384.0f;
//       doc_tx["az"] = (float)aaReal.z / 16384.0f;
//       doc_tx["gz"] = (float)gz / 131.0f; // Convert to deg/s
      
//       String jsonString;
//       serializeJson(doc_tx, jsonString);
//       webSocket.broadcastTXT(jsonString);
      
//       // Blink LED
//       digitalWrite(LED_PIN, !digitalRead(LED_PIN));
//     }
//   }
  
//   // Reset FIFO if overflow detected
//   if ((mpuIntStatus & 0x10) || fifoCount >= 1024) {
//     mpu.resetFIFO();
//     Serial.println(F("FIFO overflow! Resetting..."));
//   }
  
//   // Small delay to prevent watchdog timeout
//   delay(1);
// }