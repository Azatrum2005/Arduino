#define PIN_ANALOG_IN1   4
#define PIN_ANALOG_IN2   2
#define PIN_LED1         25
#define PIN_LED2         26
#define CHAN1            0
#define CHAN2            15
void setup() {
  Serial.begin(115200); 
  ledcSetup(CHAN1, 1000, 12);
  ledcAttachPin(PIN_LED1, CHAN1);
  ledcSetup(CHAN2, 1000, 12);
  ledcAttachPin(PIN_LED2, CHAN2);
}

void loop() {
  int adcVal1 = analogRead(PIN_ANALOG_IN1); //read adc
  Serial.println("1"+String(adcVal1));
  int adcVal2 = analogRead(PIN_ANALOG_IN2);
  Serial.println("2"+String(adcVal2));
  // int pwmVal = adcVal;
  ledcWrite(CHAN1, adcVal1);
  ledcWrite(CHAN2, adcVal2); 
  delay(10);
}
