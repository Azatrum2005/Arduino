#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>

const char* ssid_AP = "ESP32";
const char* password_AP= "esp32Mur";

IPAddress local_IP(192,168,1,100);
IPAddress gateway(192,168,1,10);   
IPAddress subnet(255,255,255,0);  

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Store client info
struct ClientInfo {
    uint8_t num;     // WebSocket number
    int userNumber;  // Assigned user number
    bool inUse;      // Whether this slot is being used
    const char* name;
};
// const char** name=(const char**)calloc(10,sizeof(const char*));
const int MAX_CLIENTS = 10;
ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;

String webpage1 = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dynamic WebSocket Example</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f6f9;
            margin: 0;
            padding: 0;
        }

        #myUserNumber {
            font-size: 1.5rem;
            margin: 20px auto;
            text-align: center;
            color: #333;
            font-weight: bold;
        }

        #outer {
            max-width: 800px;
            margin: 20px auto;
            padding: 10px;
            background: #ffffff;
            border-radius: 10px;
            box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.1);
        }

        .message-box {
            border: 1px solid #ddd;
            border-radius: 10px;
            padding: 20px;
            margin-bottom: 15px;
            background-color: #fafafa;
        }

        .message-box.my-box {
            background-color: #e6f3ff;
        }

        .message-box h3 {
            margin: 0;
            color: #0056b3;
        }

        input[type="text"] {
            width: calc(100% - 80px);
            padding: 10px;
            margin: 10px 0;
            border-radius: 5px;
            border: 1px solid #ccc;
            font-size: 1rem;
            box-sizing: border-box;
        }

        button {
            padding: 10px 20px;
            margin: 10px 0;
            background-color: #007bff;
            color: #fff;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1rem;
        }

        button:hover {
            background-color: #0056b3;
        }

        p {
            margin: 10px 0;
            font-size: 1rem;
            color: #444;
        }

        .message {
            padding: 10px;
            background-color: #f1f1f1;
            border-radius: 5px;
            margin-top: 5px;
        }
    </style>
</head>
<body>
    <div id="myUserNumber"></div>
    <div id="outer"></div>
</body>
<script>
    var Socket;
    var myNumber = 0;
    var messgcount = 0;
    var name = ``;

    function init() {
        Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        
        Socket.onmessage = function(event) {
            processCommand(event);
        };
        
        Socket.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
    }

    function Name(event) {
        var button = event.target;
        var number = button.id.replace('sbnm', '');

        if (number == myNumber) {
            var inputField = document.getElementById(`nmin${number}`);
            var messageContent = inputField.value;
            var msg = {
                action: 'send_name',
                number: number,
                content: messageContent
            };
            Socket.send(JSON.stringify(msg));
            inputField.value = '';
        } else {
            alert('You can only set your own name!');
        }
    }

    function message(event) {
        var button = event.target;
        var number = button.id.replace('sb', '');

        if (number == myNumber) {
            var inputField = document.getElementById(`in${number}`);
            var messageContent = inputField.value;
            var msg = {
                action: 'send_message',
                number: number,
                content: messageContent
            };
            Socket.send(JSON.stringify(msg));
            inputField.value = '';
        } else {
            alert('You can only send messages from your own message box!');
        }
    }

    function processCommand(event) {
        var obj = JSON.parse(event.data);
        
        if (obj.action === 'assign_number') {
            myNumber = obj.assigned_number;
            document.getElementById('myUserNumber').textContent = 'You are User ' + myNumber;
            createMessageBox(myNumber);
        }
        else if (obj.action === 'assign_rest_number') {
            createMessageBox(obj.assigned_number);
        }
        else if (obj.action === 'update_name') {
            name = obj.content;
            let nameElement = document.getElementById(`nm${obj.number}`);
            if (nameElement) {
                nameElement.textContent = name;
            }
        }
        else if (obj.action === 'update_message') {
            messgcount++;
            var p = document.createElement('p');
            p.classList.add('message');
            p.id = `m${obj.number}${messgcount}`;
            document.getElementById(`${obj.number}`).appendChild(p);
            let messageElement = document.getElementById(`m${obj.number}${messgcount}`);
            if (messageElement) {
                messageElement.textContent = `User ${obj.number}: ${obj.content}`;
            }
        }
    }

    function createMessageBox(num) {
        var div = document.createElement('div');
        div.classList.add('message-box');
        if (num === myNumber) div.classList.add('my-box');
        div.id = num;

        div.innerHTML = `
            <h3>User ${num}</h3>
            <input id='nmin${num}' type='text' placeholder='Enter your name' ${num !== myNumber ? 'disabled' : ''} />
            <button type='button' id='sbnm${num}'>Set Name</button>
            <h1><p id='nm${num}'></p></h1>
            <input id='in${num}' type='text' placeholder='Type a message' ${num !== myNumber ? 'disabled' : ''} />
            <button type='button' id='sb${num}'>Send Message</button>
        `;
        document.getElementById('outer').appendChild(div);
        document.getElementById(`sb${num}`).addEventListener('click', message);
        document.getElementById(`sbnm${num}`).addEventListener('click', Name);
    }

    window.onload = function(event) {
        init();
    };
</script>
</html>

)rawliteral";

// void broadcastAllNames() {
//     for (int i = 0; i < MAX_CLIENTS; i++) {
//         if (clients[i].inUse) {
//             String jsonString;
//             StaticJsonDocument<200> doc;
//             doc["action"] = "update_name";
//             doc["content"] = clients[i].name;
//             doc["number"] = i + 1;
//             serializeJson(doc, jsonString);
//             webSocket.broadcastTXT(jsonString);
//         }
//     }
// }

void sendExistingNamesToClient(uint8_t num) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].inUse && clients[i].name != nullptr) {
            String jsonString;
            StaticJsonDocument<200> doc;
            doc["action"] = "update_name";
            doc["content"] = clients[i].name;
            doc["number"] = i + 1;
            serializeJson(doc, jsonString);
            webSocket.sendTXT(num, jsonString);
        }
    }
}
void assignClientNumber(uint8_t num) {
    // Find next available user number
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].inUse) {
            clients[i].num = num;
            clients[i].userNumber = i + 1;
            clients[i].inUse = true;
            
            // Send assigned number to client
            String jsonString;
            StaticJsonDocument<200> doc;
            doc["action"] = "assign_number";
            doc["assigned_number"] = clients[i].userNumber;
            serializeJson(doc, jsonString);
            webSocket.sendTXT(num, jsonString);
            
            // Send existing client numbers to new client
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].inUse && clients[j].num != num) {
                    doc.clear();
                    doc["action"] = "assign_rest_number";
                    doc["assigned_number"] = clients[j].userNumber;
                    jsonString = "";
                    serializeJson(doc, jsonString);
                    webSocket.sendTXT(num, jsonString);
                }
            }
            sendExistingNamesToClient(num);
            break;
        }
    }
}

void removeClient(uint8_t num) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].inUse && clients[i].num == num) {
            clients[i].inUse = false;
            if (clients[i].name != nullptr) {
                // Clear the name when client disconnects
                clients[i].name = nullptr;
                // Broadcast name update (empty) to all clients
                String jsonString;
                StaticJsonDocument<200> doc;
                doc["action"] = "update_name";
                doc["content"] = "";
                doc["number"] = i + 1;
                serializeJson(doc, jsonString);
                webSocket.broadcastTXT(jsonString);
            }
            break;
        }
    }
}

int getClientUserNumber(uint8_t num) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].inUse && clients[i].num == num) {
            return clients[i].userNumber;
        }
    }
    return -1;  // Not found
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            clientCount--;
            Serial.println("Client " + String(num) + " disconnected");
            removeClient(num);
            break;

        case WStype_CONNECTED:
            Serial.println("Client " + String(num) + " connected");
            clientCount++;
            assignClientNumber(num);
            break;

        case WStype_TEXT: {
            StaticJsonDocument<200> doc_rx;
            DeserializationError error = deserializeJson(doc_rx, payload);
            
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            if (doc_rx["action"] == "send_name") {
                int senderNumber = getClientUserNumber(num);
                int claimedNumber = doc_rx["number"];
                
                // Verify sender is using their assigned number
                if (senderNumber == claimedNumber) {
                    Serial.printf("%d,%s",senderNumber,doc_rx["content"].as<const char*>());
                    // clients[senderNumber-1].name =doc_rx["content"].as<const char*>();
                    const char* newName = doc_rx["content"].as<const char*>();
                    clients[senderNumber-1].name = strdup(newName);
                    String jsonString;
                    StaticJsonDocument<200> doc_tx;
                    doc_tx["action"] = "update_name";
                    doc_tx["content"] = doc_rx["content"].as<const char*>();
                    doc_tx["number"] = senderNumber;
                    serializeJson(doc_tx, jsonString);
                    webSocket.broadcastTXT(jsonString);
                }
            }

            if (doc_rx["action"] == "send_message") {
                int senderNumber = getClientUserNumber(num);
                int claimedNumber = doc_rx["number"];
                
                // Verify sender is using their assigned number
                if (senderNumber == claimedNumber) {
                    String jsonString;
                    StaticJsonDocument<200> doc_tx;
                    doc_tx["action"] = "update_message";
                    doc_tx["content"] = doc_rx["content"].as<const char*>();
                    doc_tx["number"] = senderNumber;
                    serializeJson(doc_tx, jsonString);
                    webSocket.broadcastTXT(jsonString);
                }
            }
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);                 
    delay(2000);
    
    // Initialize clients array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].inUse = false;
    }
    
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
    server.handleClient();
    webSocket.loop();
}