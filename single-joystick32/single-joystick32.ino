/* Boxy~the~robot 
   created by: kyle
   For: Reuseum Educational 
   This is boxy controlled
   by single joystick
     .__________________.          
     |                  |
     |     ~~      ~~   |
  ---|      #       #   |---
{<><}|          ^       |{<><}
{<><}|__________________|{<><}
  ---                    ---
*/

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Access point credentials
const char* ssid = "boxy"; //basicaly the robots name: "ssid.local" 
const char* password = ""; // password to secure your robot
int channel = 6;  // Specify your desired channel //wifi chanel 1-3-11 work best

WebServer server(80);
WebSocketsServer webSocket(81); // WebSocket server on port 81

// Motor pins
const int motor1A = 16; // Motor 1 pin A
const int motor1B = 17; // Motor 1 pin B
const int motor2A = 18; // Motor 2 pin A
const int motor2B = 19; // Motor 2 pin B

// LED pin
const int ledPin = 2; // LED pin

// Direction variable
bool yDir = false;

// Maximum speed for motors
int maxSpeed = 200; // Default value, can be changed via WebSocket

// Timeout for WebSocket clients
unsigned long lastActivityTime = 0;
const unsigned long clientTimeout = 30000; // 30 seconds

// WebSocket event handling function
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT) {
        String message((char*)payload);
        Serial.println("WebSocket message received: " + message);

        if (message == "toggleLED") { // Check if message is "toggleLED"
            digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED state
            // Send current LED state back to client
            webSocket.sendTXT(num, digitalRead(ledPin) ? "LED ON" : "LED OFF");
            return;
        }

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error) {
            Serial.println(String("Failed to parse WebSocket message: ") + error.c_str());
            return;
        }

        // Update maxSpeed if provided in the WebSocket message
        if (doc.containsKey("maxSpeed")) {
            maxSpeed = doc["maxSpeed"].as<int>();
        }

        if (doc.containsKey("joy1X") && doc.containsKey("joy1Y")) {
            int x = doc["joy1X"].as<int>() * 10;
            int y = doc["joy1Y"].as<int>() * 10;

            int aSpeed = abs(y);
            int bSpeed = abs(y);

            aSpeed = constrain(aSpeed + x / 2, 0, maxSpeed);
            bSpeed = constrain(bSpeed - x / 2, 0, maxSpeed);

            Serial.print("aSpeed=");
            Serial.println(aSpeed);
            Serial.print("bSpeed=");
            Serial.println(bSpeed);

            if (x == 0 && y == 0) {
                // Stop the motors if both X and Y values are 0
                analogWrite(motor1A, 0);
                analogWrite(motor1B, 0);
                analogWrite(motor2A, 0);
                analogWrite(motor2B, 0);
                return;
            } else if (y < 0) {
                yDir = false; // Reverse direction
                // Motor 2
                digitalWrite(motor2B, yDir);
                analogWrite(motor2A, aSpeed);
                // Motor 1
                digitalWrite(motor1B, yDir);
                analogWrite(motor1A, bSpeed);
            } else if (y > 0) {
                yDir = true; // Forward direction
                // Motor 2
                analogWrite(motor2B, aSpeed);
                digitalWrite(motor2A, yDir);
                // Motor 1
                analogWrite(motor1B, bSpeed);
                digitalWrite(motor1A, yDir);
            } else {
                // Stop the motors if y is 0
                analogWrite(motor2B, LOW);
                digitalWrite(motor2A, LOW);
                analogWrite(motor1B, LOW);
                digitalWrite(motor1A, LOW);
            }
        }

        // Update last activity time
        lastActivityTime = millis();
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("Client [%u] disconnected.\n", num);
    } else if (type == WStype_CONNECTED) {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("Client [%u] connected from %s.\n", num, ip.toString().c_str());
        webSocket.sendTXT(num, "Connected to Boxy!");
    }
}

// Setup function for configuring the ESP8266
void setup() {
    pinMode(ledPin, OUTPUT);
    Serial.begin(115200);

    // Configuring the access point
    WiFi.softAP(ssid, password, channel);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    // Mounting SPIFFS for serving static HTML and JS
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS mount failed");
    } else {
        Serial.println("SPIFFS mount successful");
    }

  // Initialize mDNS
  if (!MDNS.begin(ssid)) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  
    // Serving static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/singlejoy.html");
    server.serveStatic("/VirtualJoystick.js", SPIFFS, "/VirtualJoystick.js");

    // Starting the WebSocket server and setting the event handler
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    server.begin();

    // Initialize last activity time
    lastActivityTime = millis();
}

// Main loop to handle WebSocket and HTTP server events
void loop() {
    server.handleClient();
    webSocket.loop(); // Handle WebSocket events

    // Check for client timeout
    if (millis() - lastActivityTime > clientTimeout) {
        Serial.println("Client timed out. Restarting WebSocket server...");
        webSocket.disconnect();
        webSocket.begin();
        lastActivityTime = millis(); // Reset last activity time
    }
}
