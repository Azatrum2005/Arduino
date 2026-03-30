#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>

// SSID and password of Wifi connection:
const char* ssid = "H155-380_198A";
const char* password = "<password>";
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

String webpage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dynamic WebSocket Example</title>
    <style>
    body {
        font-family: Arial, sans-serif;
        background-color: #121212;
        margin: 0;
        padding: 0;
        color: #e0e0e0;
    }

    #myUserNumber {
        font-size: 1.5rem;
        margin: 20px auto;
        text-align: center;
        color: #bb86fc;
        font-weight: bold;
    }

    #outer {
        max-width: 800px;
        margin: 20px auto;
        padding: 10px;
        background: #1e1e1e;
        border-radius: 10px;
        box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.4);
    }

    .message-box {
        border: 1px solid #333;
        border-radius: 10px;
        padding: 20px;
        margin-bottom: 15px;
        background-color: #2c2c2c;
    }

    .message-box.my-box {
        background-color: #3700b3;
        color: #ffffff;
    }

    .message-box h3 {
        margin: 0;
        color: #bb86fc;
    }

    input[type="text"] {
        width: calc(100% - 80px);
        padding: 10px;
        margin: 10px 0;
        border-radius: 5px;
        border: 1px solid #444;
        font-size: 1rem;
        box-sizing: border-box;
        background-color: #1e1e1e;
        color: #e0e0e0;
    }

    input[type="text"]::placeholder {
        color: #888;
    }

    button {
        padding: 10px 20px;
        margin: 10px 0;
        background-color: #bb86fc;
        color: #121212;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        font-size: 1rem;
    }

    button:hover {
        background-color: #3700b3;
        color: #ffffff;
    }

    p {
        margin: 10px 0;
        font-size: 1rem;
        color: #e0e0e0;
    }

    .message {
        padding: 10px;
        background-color: #2c2c2c;
        border-radius: 5px;
        margin-top: 5px;
        position: relative;
    }

    .message.bot-message {
        background-color: #3700b3;
        color: #ffffff;
    }

    .timestamp {
        font-size: 0.8rem;
        color: #888;
        position: absolute;
        right: 10px;
        bottom: 5px;
    }

    .typing-indicator {
        font-style: italic;
        color: #bb86fc;
        padding: 5px 10px;
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
                content: messageContent,
                metadata: {
                    timestamp: new Date().toLocaleTimeString(),
                    sender: `User ${number}`
                }
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
                content: messageContent,
                metadata: {
                    timestamp: new Date().toLocaleTimeString(),
                    sender: name || `User ${number}`,
                    messageType: 'user_message'
                }
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
        else if (obj.action === 'typing_status') {
            updateTypingIndicator(obj.number, obj.content);
        }
        else if (obj.action === 'update_message' || obj.action === 'send_reply') {
            messgcount++;
            var p = document.createElement('p');
            p.classList.add('message');
            if (obj.number === 1) {
                p.classList.add('bot-message');
            }
            p.id = `m${obj.number}${messgcount}`;
            
            // Add timestamp if available
            if (obj.metadata && obj.metadata.timestamp) {
                var timestamp = document.createElement('span');
                timestamp.classList.add('timestamp');
                timestamp.textContent = obj.metadata.timestamp;
                p.appendChild(timestamp);
            }

            var content = document.createElement('div');
            content.textContent = `${`${obj.name}` || `User ${obj.number}`}: ${obj.content}`;
            content.style.whiteSpace='pre-line';
            p.appendChild(content);

            document.getElementById(`${obj.number}`).appendChild(p);
            p.scrollIntoView({ behavior: 'smooth', block: 'end' });
        }
    }

    function updateTypingIndicator(number, text) {
        let boxElement = document.getElementById(number);
        let existingIndicator = boxElement.querySelector('.typing-indicator');
        
        if (text) {
            if (!existingIndicator) {
                let indicator = document.createElement('div');
                indicator.classList.add('typing-indicator');
                indicator.textContent = text;
                boxElement.appendChild(indicator);
            } else {
                existingIndicator.textContent = text;
            }
        } else if (existingIndicator) {
            existingIndicator.remove();
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

void sendNamesToClient(uint8_t num) {
    // Send all existing names to the new client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].inUse && clients[i].name != nullptr) {
            String jsonString;
            StaticJsonDocument<200> doc;
            doc["action"] = "update_name";
            doc["content"] = clients[i].name;
            doc["number"] = clients[i].userNumber;
            serializeJson(doc, jsonString);
            webSocket.sendTXT(num, jsonString);
        }
    }
}

void broadcastName(int userNumber, const char* userName) {
    String jsonString;
    StaticJsonDocument<200> doc;
    doc["action"] = "update_name";
    doc["content"] = userName;
    doc["number"] = userNumber;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}

void assignClientNumber(uint8_t num) {
    // Find next available user number
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].inUse) {
            clients[i].num = num;
            clients[i].userNumber = i + 1;
            clients[i].inUse = true;
            clients[i].name = nullptr;
            
            // Send assigned number to client
            String jsonString;
            StaticJsonDocument<200> doc;
            doc["action"] = "assign_number";
            doc["assigned_number"] = clients[i].userNumber;
            serializeJson(doc, jsonString);
            webSocket.sendTXT(num, jsonString);
            
            // Send existing client numbers to new client
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].inUse && j != i) {
                    doc.clear();
                    doc["action"] = "assign_rest_number";
                    doc["assigned_number"] = clients[j].userNumber;
                    jsonString = "";
                    serializeJson(doc, jsonString);
                    webSocket.sendTXT(num, jsonString);
                }
            }
            
            // Send all existing names to the new client
            sendNamesToClient(num);
            break;
        }
    }
}

void removeClient(uint8_t num) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].inUse && clients[i].num == num) {
            // Broadcast empty name before removing
            broadcastName(clients[i].userNumber, "");
            clients[i].inUse = false;
            clients[i].name = nullptr;
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
            Serial.println("Client " + String(num) + " disconnected");
            clientCount--;
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
                if (senderNumber == claimedNumber ) {
                    String jsonString;
                    StaticJsonDocument<200> doc_tx;
                    doc_tx["action"] = "update_message";
                    doc_tx["content"] = doc_rx["content"].as<const char*>();
                    doc_tx["name"] = clients[senderNumber-1].name;
                    doc_tx["number"] = senderNumber;
                    doc_tx["metadata"] = doc_rx["metadata"];
                    serializeJson(doc_tx, jsonString);
                    webSocket.broadcastTXT(jsonString);
                }
            }

            if (doc_rx["action"] == "send_reply") {
                int senderNumber = getClientUserNumber(num);
                int claimedNumber = doc_rx["number"];
                
                // Verify sender is using their assigned number
                if (senderNumber == claimedNumber) {
                    String jsonString;
                    StaticJsonDocument<200> doc_tx;
                    doc_tx["action"] = "update_message";
                    doc_tx["content"] = doc_rx["content"].as<const char*>();
                    doc_tx["name"] = clients[senderNumber-1].name;
                    doc_tx["number"] = senderNumber;
                    doc_tx["metadata"] = doc_rx["metadata"];
                    serializeJson(doc_tx, jsonString);
                    webSocket.broadcastTXT(jsonString);
                }
            }

            if (doc_rx["action"] == "typing_status") {
                int senderNumber = getClientUserNumber(num);
                int claimedNumber = doc_rx["number"];
                
                // Verify sender is using their assigned number
                if (senderNumber == claimedNumber) {
                    String jsonString;
                    StaticJsonDocument<200> doc_tx;
                    doc_tx["action"] = "typing_status";
                    doc_tx["content"] = doc_rx["content"].as<const char*>();
                    doc_tx["name"] = clients[senderNumber-1].name;
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
    WiFi.begin(ssid, password);
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", [](){
        server.send(200, "text/html", webpage);
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
