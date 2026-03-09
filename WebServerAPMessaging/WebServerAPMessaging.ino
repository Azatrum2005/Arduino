#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// SSID and password of Wifi connection:
const char* ssid_AP = "ESP32";
const char* password_AP= "esp32Mur";

IPAddress local_IP(192,168,1,100);
IPAddress gateway(192,168,1,10);   
IPAddress subnet(255,255,255,0);  

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String webpage1 = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Dynamic WebSocket Messaging</title>
</head>
<body>
    <div id='1'>
        <input id='sr1' type='text' placeholder='user1 enter a message' />
        <button type='button' id='sb1'>Submit</button>
        <p id='m1'></p>
        <div id='2'></div>
    </div>
</body>
<script>
    var Socket;
    function init() {
        Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        Socket.onmessage = function(event) {
            processCommand(event);
        };
        Socket.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
        document.getElementById('sb1').addEventListener('click', message);
    }

    function message(event) {
        var button = event.target;
        var number = button.id.replace('sb', '');
        var inputField = document.getElementById(`sr${number}`);
        var messageContent = inputField.value;
        var msg = {
            action: 'send_message',
            number: number,
            content: messageContent
        };
        Socket.send(JSON.stringify(msg));
        inputField.value = '';
    }

    function processCommand(event) {
        var obj = JSON.parse(event.data);
        if (obj.action === 'create') {
            create(obj.num);
        } 
        else if (obj.action === 'update_message') {
            // Update messages for all users
            let messageElement = document.getElementById(`m${obj.number}`);
            if (messageElement) {
                messageElement.textContent = obj.content;
            }
        }
        else if (obj.action === 'new_user') {
            create(obj.num);
        }
    }

    function create(num) {
        document.getElementById(num).innerHTML = `
            <input id='sr${num}' type='text' placeholder='user${num} enter a message' />
            <button type='button' id='sb${num}'>Submit</button>
            <p id='m${num}'></p>
            <div id='${parseInt(num) + 1}'></div>
        `;
        document.getElementById(`sb${num}`).addEventListener('click', message);
    }

    window.onload = function(event) {
        init();
    };
</script>
</html>
)rawliteral";

int count = 0;
int interval = 2000;
unsigned long previousMillis = 0;
StaticJsonDocument<200> doc_tx;
StaticJsonDocument<200> doc_rx;

void broadcastUserCount() {
    String jsonString;
    doc_tx.clear();
    doc_tx["action"] = "new_user";
    doc_tx["num"] = count;
    serializeJson(doc_tx, jsonString);
    webSocket.broadcastTXT(jsonString);
}

void broadcastMessage(const String& message, int userNumber) {
    String jsonString;
    doc_tx.clear();
    doc_tx["action"] = "update_message";
    doc_tx["content"] = message;
    doc_tx["number"] = userNumber;
    doc_tx["num"] = count;
    serializeJson(doc_tx, jsonString);
    webSocket.broadcastTXT(jsonString);
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("Client " + String(num) + " disconnected");
            count--;
            broadcastUserCount();
            break;

        case WStype_CONNECTED:
            Serial.println("Client " + String(num) + " connected");
            count++;
            broadcastUserCount();
            break;

        case WStype_TEXT: {
            DeserializationError error = deserializeJson(doc_rx, payload);
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            if (doc_rx["action"] == "send_message") {
                const char* content = doc_rx["content"];
                int userNumber = doc_rx["number"];
                Serial.println("Message received: " + String(content));
                broadcastMessage(content, userNumber);
            }
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);                 
    delay(2000);
    
    Serial.println("Setting soft-AP configuration ... ");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
    
    Serial.println("Setting soft-AP ... ");
    boolean result = WiFi.softAP(ssid_AP, password_AP);
    if(result) {
        Serial.println("Ready");
        Serial.println(String("Soft-AP IP address = ") + WiFi.softAPIP().toString());
        Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
    } else {
        Serial.println("Failed!");
    }
    
    server.on("/", [](){
        server.send(200, "text/html", webpage1);
    });
    
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    
    Serial.println("Setup Complete");
}

void loop() {
    // for(int i=1;i<=count;i++){
    // server.on("/", [](){server.send(200, "text/html", webpage1);});
    // server.begin();
    // }
    server.handleClient();
    webSocket.loop();
}
