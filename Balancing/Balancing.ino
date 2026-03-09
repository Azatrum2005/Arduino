#include <ESP32Servo.h>

class Servo myservo;  
int d= 0;
int a=90;
int posVal = 90;    
int servoPin = 15; 
#include <UltrasonicSensor.h>

UltrasonicSensor ultrasonic(32, 33);
void setup() {
  Serial.begin(115200);
  myservo.setPeriodHertz(50); 
  myservo.attach(servoPin, 500, 2500);  
  int temperature = 30;
  ultrasonic.setTemperature(temperature);
}

void loop() {
  int distance1 = ultrasonic.distanceInCentimeters();
  Serial.printf("Distance: %dcm\n",distance1);
  d=15-distance1;
  Serial.print(d);
  delay(90);
  int distance2 = ultrasonic.distanceInCentimeters();
  int b=15-distance2;
  Serial.print(b);
  delay(10);
  if(d==b){
    if(a<90-d*9){
      for (posVal=a; posVal <=90-d*9; posVal += 1) { 
        myservo.write(posVal);      
        delay(3);  
      }
      a=posVal;
      Serial.print(a);
    }
    else if(a>90-d*9){
      for (posVal=a; posVal >=90-d*9; posVal -= 1) { 
        myservo.write(posVal);      
        delay(3);  
      }
      a=posVal;
      Serial.print(a);
    
    }
  }
  if(d!=b){
    Serial.print("----------");
    if(b>d){
      for (posVal=a; posVal >= a-(b-d)*15; posVal -= 1) { 
        myservo.write(posVal);      
        delay(3);  
      }
      Serial.print(".....");
      a=posVal;
      Serial.print(a);
      delay(500);
    } 
    else if(d>b){
      for (posVal=a; posVal <= a+(d-b)*15; posVal += 1) { 
        myservo.write(posVal);      
        delay(3);  
      }
      Serial.print("_____");
      a=posVal;
      Serial.print(a);
      delay(500);
    }
  }
}
