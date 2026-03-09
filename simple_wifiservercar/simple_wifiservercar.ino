#include <WiFi.h>
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
#define LEFT 3
#define RIGHT 4
#define STOP 0
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1
#define ledPin 32
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

const char* ssid     = "X28-2.4G-551C16";
const char* password = "728A33C5";

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    setUpPinModes();
    pinMode(ledPin,OUTPUT);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

}

void loop(){
 digitalWrite(ledPin,HIGH);
 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<h1>Forward</h1>");
            client.print("<a href=\"/forward\">forward</a>");
            client.print("<h1>Backward</h1>");
            client.print("<a href=\"/backward\">backward</a>");
            client.print("<h1>Right</h1>");
            client.print("<a href=\"/right\">right</a>");
            client.print("<h1>Left</h1>");
            client.print("<a href=\"/left\">left</a>");
            client.print("<h1>Stop</h1>");
            client.print("<a href=\"/stop\">stop</a>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /forward")) {
          ledcWrite(PWMSpeedChannel,254);
          rotateMotor(RIGHT_MOTOR, FORWARD);
          rotateMotor(LEFT_MOTOR, FORWARD);
        }
        if (currentLine.endsWith("GET /backward")) {
          ledcWrite(PWMSpeedChannel,254);
          rotateMotor(RIGHT_MOTOR, BACKWARD);
          rotateMotor(LEFT_MOTOR, BACKWARD);
        }
        if (currentLine.endsWith("GET /stop")) {
          rotateMotor(RIGHT_MOTOR, STOP);
          rotateMotor(LEFT_MOTOR, STOP);
        }
        if (currentLine.endsWith("GET /right")) {
          ledcWrite(PWMSpeedChannel,254);
          rotateMotor(RIGHT_MOTOR, BACKWARD);
          rotateMotor(LEFT_MOTOR, FORWARD);
        }
        if (currentLine.endsWith("GET /left")) {
          ledcWrite(PWMSpeedChannel,254);
          rotateMotor(RIGHT_MOTOR, FORWARD);
          rotateMotor(LEFT_MOTOR, BACKWARD);
        }
      } 
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

