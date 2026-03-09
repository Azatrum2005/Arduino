#include <UltrasonicSensor.h>
UltrasonicSensor ultrasonic(32, 33);

int sensorPin = 5;
int ledPin = 18;

struct MOTOR_PINS
{
  int pinEn;  
  int pinIN1;
  int pinIN2;    
};

int motorPins[][3] = 
{
  {13, 12, 14},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {13, 15, 0},  //LEFT_MOTOR  Pins (EnB, IN3, IN4)
};

#define UP 1
#define DOWN 2
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 500; /* 10 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;

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
void setUpPinModes()
{
  //Set up PWM
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);

  for (int i = 0; i < 2; i++)
  {
    pinMode(motorPins[i][0], OUTPUT);    
    pinMode(motorPins[i][1], OUTPUT);
    pinMode(motorPins[i][2], OUTPUT);  

    /* Attach the PWM Channel to the motor enb Pin */
    ledcAttachPin(motorPins[i][0], PWMSpeedChannel);
  }

}


void setup(void) 
{
  setUpPinModes();
  pinMode(sensorPin, INPUT);  // initialize the sensor pin as input
  pinMode(ledPin, OUTPUT);
  int temperature = 25;
  ultrasonic.setTemperature(temperature);
  Serial.begin(115200);
}
void loop(){
  digitalWrite(ledPin, digitalRead(sensorPin));
  int distance = ultrasonic.distanceInCentimeters();
  Serial.printf("Distance: %dcm\n",distance);
  // ledcWrite(PWMSpeedChannel,0);
  if(1){
    if(distance>15){
      ledcWrite(PWMSpeedChannel,100+2*distance);
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD); 
    }
    else if(distance<10){
      ledcWrite(PWMSpeedChannel,180);
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
    }
    else{
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);
    }
  }
  delay(100);
}
