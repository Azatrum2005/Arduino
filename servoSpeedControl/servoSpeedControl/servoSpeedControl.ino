#include <ESP32Servo.h> 
#define ADC_Max 4095    // This is the default ADC max value on the ESP32 (12 bit ADC width); 

Servo servo1;
Servo servo2;
int servoPin1 = 25;      // GPIO pin used to connect the servo control (digital out)
int servoPin2 = 26;
int potPin = 4; 
int potVal;

void setup(){
  servo1.setPeriodHertz(50);// Standard 50hz servo   
  servo2.setPeriodHertz(50);
  servo1.attach(servoPin1, 900, 2100);   
  servo2.attach(servoPin2, 900, 2100);  
  Serial.begin(115200);
  // myservo.write(input);
  delay(1000);
}
void loop(){
  potVal = analogRead(potPin);
  potVal = map(potVal, 0, ADC_Max, 900, 2100);
  Serial.printf("potVal: %d\n",potVal);
  servo1.writeMicroseconds(potVal);
  servo2.writeMicroseconds(potVal);
}