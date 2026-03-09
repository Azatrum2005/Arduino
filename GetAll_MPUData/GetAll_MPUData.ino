
// #include <MPU6050_tockn.h>
// #include <Wire.h>

// MPU6050 mpu6050(Wire);

// long timer = 0;

// void setup() {
//   Serial.begin(115200);
//   Wire.begin(13,14);
//   mpu6050.begin();
//   mpu6050.calcGyroOffsets(true);
// }

// void loop() {
//   mpu6050.update();

//   if(millis() - timer > 100){
    
//     Serial.println("=======================================================");
//     Serial.print("temp : ");Serial.println(mpu6050.getTemp());
//     Serial.print("accX : ");Serial.print(mpu6050.getAccX());
//     Serial.print("\taccY : ");Serial.print(mpu6050.getAccY());
//     Serial.print("\taccZ : ");Serial.println(mpu6050.getAccZ());
  
//     Serial.print("gyroX : ");Serial.print(mpu6050.getGyroX());
//     Serial.print("\tgyroY : ");Serial.print(mpu6050.getGyroY());
//     Serial.print("\tgyroZ : ");Serial.println(mpu6050.getGyroZ());
  
//     Serial.print("accAngleX : ");Serial.print(mpu6050.getAccAngleX());
//     Serial.print("\taccAngleY : ");Serial.println(mpu6050.getAccAngleY());
  
//     Serial.print("gyroAngleX : ");Serial.print(mpu6050.getGyroAngleX());
//     Serial.print("\tgyroAngleY : ");Serial.print(mpu6050.getGyroAngleY());
//     Serial.print("\tgyroAngleZ : ");Serial.println(mpu6050.getGyroAngleZ());
    
//     Serial.print("angleX : ");Serial.print(mpu6050.getAngleX());
//     Serial.print("\tangleY : ");Serial.print(mpu6050.getAngleY());
//     Serial.print("\tangleZ : ");Serial.println(mpu6050.getAngleZ());
//     Serial.println("=======================================================\n");
//     timer = millis();
    
//   }

// }


#include <MPU6050_tockn.h>
#include <Wire.h>

#define SDA_PIN 13
#define SCL_PIN 14

MPU6050 mpu6050(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
}

void loop() {
  mpu6050.update();

  // Output ONLY numbers (tab-separated)
  Serial.print(mpu6050.getAccX());   // g
  Serial.print("\t");
  Serial.print(mpu6050.getAccAngleX());
  Serial.print("\t");
  Serial.print(mpu6050.getAngleX());
  Serial.print("\t");
  Serial.print(mpu6050.getAccY());   // g
  Serial.print("\t");
  Serial.print(mpu6050.getAccAngleY());
  Serial.print("\t");
  Serial.print(mpu6050.getAngleY());
  Serial.print("\t");
  Serial.print(mpu6050.getAccZ());   // g
  Serial.print("\t");
  // Serial.print(mpu6050.getAngleZ());
  // Serial.print("\t");
  Serial.println(mpu6050.getGyroZ()); // deg/s

  delay(20);  // ~50 Hz
}

