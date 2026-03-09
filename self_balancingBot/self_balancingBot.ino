#include "I2Cdev.h"
#include <PID_v1_bc.h> //From https://github.com/br3ttb/Arduino-PID-Library/blob/master/PID_v1.h
#include "MPU6050_6Axis_MotionApps20.h" //https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050
#include "Wire.h"
#include <ESP32Servo.h> 

Servo servo1;
Servo servo2;
int servoPin1 = 25;      // GPIO pin used to connect the servo control (digital out)
int servoPin2 = 26;

#define INTERRUPT_PIN 4  // use pin 2 on Arduino Uno & most boards
#define LED_PIN 2 // (Arduino is 13, Teensy is 11, Teensy++ is 6)
#define SDA 13
#define SCL 14
bool blinkState = false;
MPU6050_6Axis_MotionApps20 mpu;

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

double setpoint= 0; //set the value when the bot is perpendicular to ground using serial monitor. 
double Kp = 30; //21 Set this first
double Kd = 1; //0.8 Set this secound
double Ki = 100; //140 Finally set this 
double input, output;
PID pid(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}

void setup() {
    
  servo1.setPeriodHertz(50);// Standard 50hz servo   
  servo2.setPeriodHertz(50);
  servo1.attach(servoPin1, 900, 2100);   
  servo2.attach(servoPin2, 900, 2100);  

  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin(SDA, SCL, 400000);
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif
  Serial.begin(115200);
  while (!Serial); // wait for Leonardo enumeration, others continue immediately

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXAccelOffset(-1620);
  mpu.setYAccelOffset(110);
  mpu.setZAccelOffset(1660);
  mpu.setXGyroOffset(50);
  mpu.setYGyroOffset(110);
  mpu.setZGyroOffset(6);

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

  pid.SetMode(AUTOMATIC);
  pid.SetSampleTime(10);
  pid.SetOutputLimits(900, 2100);
}

void Forward() //Code to rotate the wheel forward 
{
  servo1.writeMicroseconds(output);
  servo2.writeMicroseconds(output);
  Serial.print("F"); //Debugging information 
}
void Reverse() //Code to rotate the wheel Backward  
{
  servo1.writeMicroseconds(output);
  servo2.writeMicroseconds(output);
  Serial.print("R"); 
}
void Stop() //Code to stop both the wheels
{
  servo1.writeMicroseconds(1490);
  servo2.writeMicroseconds(1490);
  Serial.print("S");
}

void loop() {
  // if programming failed, don't try to do anything
  if (!dmpReady) return;
  // read a packet from FIFO
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet 
    // display quaternion values in easy matrix form: w x y z
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    // Serial.print("quat\t");
    // Serial.print(q.w);
    // Serial.print("\t");
    // Serial.print(q.x);
    // Serial.print("\t");
    // Serial.print(q.y);
    // Serial.print("\t");
    // Serial.println(q.z);

    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    Serial.print("ypr\t");
    Serial.print(ypr[0] * 180/M_PI);
    Serial.print("\t");
    Serial.print(ypr[1] * 180/M_PI);
    Serial.print("\t");
    Serial.println(ypr[2] * 180/M_PI);
    
    input = ypr[1] * 180/M_PI;
    pid.Compute();    
    //Print the value of Input and Output on serial monitor to check how it is working.
    Serial.print(input); Serial.print(" =>"); Serial.println(output);
    if (input<-5 || input>5){//If the Bot is falling
      if (output>1500) //Falling towards front 
        Forward(); //Rotate the wheels forward 
      else if (output<1500) //Falling towards back
        Reverse(); //Rotate the wheels backward 
    }
    else //If Bot not falling
      Stop(); //Hold the wheels still

    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
  }
  delay(5);
}
