// bool A = 0;
// bool B = 0;
// void setup() {
//   Serial.begin(115200);
//   pinMode(2, OUTPUT);
//   pinMode(4, OUTPUT);
//   pinMode(15, INPUT);
//   pinMode(13, INPUT);
// }

// void loop() {
//   A = digitalRead(15);
//   B = digitalRead(13);
//   Serial.printf("%d\t",A);
//   Serial.printf("%d\n",B);
//   digitalWrite(2, A);
//   digitalWrite(4, B);
// }

#include <ESP32Encoder.h>
// Red: Motor+ (Driver A)
// Black: Motor- (Driver B)
// Blue: Encoder 3.3V
// Green: Encoder GND
#define A 13  // Yellow Wire (Signal A)
#define B  15  // White Wire (Signal B)

ESP32Encoder encoder;
long oldPosition = 0;
unsigned long lastTime = 0;
// 11 pulses per rev (internal) * 50 gear ratio * 4 (quadrature) = 2200
const float COUNTS_PER_REV = 2200.0; 

void setup() {
  Serial.begin(115200);
  encoder.attachHalfQuad(A, B);
  // encoder.attachFullQuad(B, A);
  // Set starting count to 0
  encoder.setCount(0);
  Serial.println("Encoder Initialized! Start rotating the motor...");
}

void loop() {
  // We calculate RPM
  if (millis() - lastTime >= 100) {
    // 1. Get current data
    long newPosition = encoder.getCount();
    unsigned long currentTime = millis();
    // 2. Calculate change in position and time
    long deltaCount = newPosition - oldPosition;
    unsigned long deltaTime = currentTime - lastTime;
    // 3. Calculate RPM
    // RPM = (Delta Pulses / Pulses per Rev) * (60000 ms / Delta Time)
    float rpm = ((float)deltaCount / COUNTS_PER_REV) * (60000.0 / deltaTime);
    // 4. Calculate Angle (0-360 degrees)
    // The modulo operator (%) keeps it within one circle
    float angle = (newPosition % (long)COUNTS_PER_REV) * 360.0 / COUNTS_PER_REV; //fmod(((newPosition)* 360.0 / COUNTS_PER_REV), 360.0); //
    if (angle < 0) {
      angle += 360.0;
    }
    // 5. Determine Direction
    String direction = (rpm > 0) ? "CW" : (rpm < 0) ? "CCW" : "STOP";

    Serial.print(newPosition);
    Serial.print("  Dir: "); Serial.print(direction);
    Serial.print(" | Angle: "); Serial.print(angle);
    Serial.print(" | RPM: "); Serial.println(rpm); // 1 decimal place
    // Update old values for next loop
    oldPosition = newPosition;
    lastTime = currentTime;
  }
}
