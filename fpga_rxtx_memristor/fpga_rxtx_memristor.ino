// // ESP32 Memristance Plotter Receiver
// // Wiring: FPGA  (TX) -> ESP32  (RX2)
// // Ground -> Ground

// #define RXD2 14
// #define TXD2 13

// void setup() {
//   Serial.begin(115200); // Fast speed for PC (Plotter)
//   Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // Match FPGA Baud Rate
// }

// void loop() {
//   // We need at least 5 bytes (Header + 4 Data)
//   if (Serial2.available() >= 5) {
    
//     // 1. Look for the Header (0xAA)
//     byte b = Serial2.read();
//     if (b != 0xAA) {
//       // If not header, discard and try next byte (re-sync)
//       return; 
//     }

//     // 2. Read the next 4 bytes
//     // We wait briefly to ensure buffer has them (UART is slow)
//     while(Serial2.available() < 4); 
    
//     unsigned long byte3 = Serial2.read();
//     unsigned long byte2 = Serial2.read();
//     unsigned long byte1 = Serial2.read();
//     unsigned long byte0 = Serial2.read();

//     // 3. Recombine into a 32-bit Integer
//     // (byte3 << 24) | (byte2 << 16) ...
//     unsigned long memristance = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;

//     // 4. Send to Plotter
//     // Arduino Plotter looks for "Variable_Name: Value"
//     Serial.print("Resistance:");
//     Serial.println(memristance >> 16);
//   }
// }

#define RXD2 14
#define TXD2 15
#define PIN_BUTTON 13
#define DAC_PIN 25
#define PIN_ANALOG_IN  4
#define PIN_PWM   33
#define CHN       3  
#define FRQ       10
#define PWM_BIT   8    

int dacVal = 0;
int step = 255;
bool increasing = true;
unsigned long prevtime = 0;
float Dc = 0;
float Tp = 100;
int del = 1;
int count = 0;
int c = 0;
float memristance = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(PIN_BUTTON, INPUT);
  // ledcSetup(CHN, FRQ, PWM_BIT); 
  // ledcAttachPin(PIN_PWM, CHN); 
}

void loop() {
  // ledcWrite(CHN, 10);

  int adcVal = analogRead(PIN_ANALOG_IN);
  // Dc = ((float)adcVal/4096)*Tp;
  // Tp = ((float)adcVal/4096)*100;
  // Tp = map(adcVal, 0, 4095, 10, 1000);

  if (digitalRead(PIN_BUTTON) ==HIGH) {
    if (memristance > 19800){c=0;}
    if (memristance < 5200){c=1;}
    if (c==1)Dc = Tp*0.9; 
    if (c==0)Dc = Tp*0.1; 
    if (increasing) {
      if(dacVal<255) dacVal += step;
      if (dacVal >= 255) dacVal = 255;
      if (dacVal >= 255 && (millis() - prevtime) > Dc ){ increasing = false; prevtime = millis(); count++;}
    } 
    else {
      if(dacVal>0) dacVal -= step;
      if(dacVal<=0) dacVal = 0;
      if (dacVal <= 0 && (millis() - prevtime) > (Tp-Dc)){ increasing = true; prevtime = millis();}
    }
  }
  else {
    count=0;
    dacVal = map(adcVal, 0, 4095, 0, 255);
    Serial.println(dacVal);
  }

  // Serial.println(dacVal);
  dacWrite(DAC_PIN, dacVal);
  float currentVoltage = (dacVal / 255.0) * 3.3;

  if (Serial2.available() >= 5) {
    if (Serial2.read() == 0xAA) { // Header
      // Wait for the 4 data bytes
      while(Serial2.available() < 4);
      
      uint32_t b3 = Serial2.read();
      uint32_t b2 = Serial2.read();
      uint32_t b1 = Serial2.read();
      uint32_t b0 = Serial2.read();

      uint32_t rawM = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
      memristance = rawM / 65536.0;

      // Serial.print("Voltage_V:");
      // Serial.print("/*");
      Serial.print(currentVoltage);
      Serial.print(",");
      // Serial.print("Memristance_Ohms:");
      Serial.print(memristance);
      Serial.print(",");
      // Serial.println(Tp);
      Serial.println(count);
      // Serial.println("*/");
    }
  }
  
  // Small delay to control wave frequency
  delay(del);
}