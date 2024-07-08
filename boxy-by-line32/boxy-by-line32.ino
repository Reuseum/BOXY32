/* Boxy~the~robot 
   created by: kyle
   For: Reuseum Educational 
   This is boxy controlled
   by line input code
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
#include <FS.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <ESP32Servo.h>

// Replace with your network credentials
const char* ssid = "boxy"; //basicaly the robots name: "ssid.local" 
const char* password = ""; // password to secure your robot
int channel = 6;  // Specify your desired channel //wifi chanel 1-3-11 work best

// Define motor pins
const int motor1a = 16;  // Replace with your actual pin numbers
const int motor1b = 17;
const int motor2a = 18;  // Replace with your actual pin numbers
const int motor2b = 19;

// Define LED pin
const int ledPin = 2;  // Replace with your actual pin number

// Define servo pin
const int servoPin = 13;  // Changed to GPIO 13 (or another available pin)

// Define servo object
Servo servo;

// Define motor speed variable
int pwm = 255; // Default PWM value (adjust as needed)

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
    Serial.begin(115200);

    // Set up motor pins
    pinMode(motor1a, OUTPUT);
    pinMode(motor1b, OUTPUT);
    pinMode(motor2a, OUTPUT);
    pinMode(motor2b, OUTPUT);

    // Set up LED pin
    pinMode(ledPin, OUTPUT);

    // Set up servo
    servo.attach(servoPin);

    // Connect to Wi-Fi
    WiFi.softAP(ssid, password, channel);
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("Access Point IP address: %s\n", apIP.toString().c_str());

    // Initialize SPIFFS
    int retryCount = 0;
    while (!SPIFFS.begin() && retryCount < 5) {
        Serial.println("Failed to mount file system. Retrying...");
        retryCount++;
        delay(1000);
    }
    if (retryCount == 5) {
        Serial.println("Failed to mount file system after 5 attempts. Please check the filesystem.");
        return;
    }
    
  // Initialize mDNS
  if (!MDNS.begin(ssid)) {   // Set the hostname to "esp32.local"
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

    // Serve "byline.html" from SPIFFS when root URL is requested
    server.serveStatic("/", SPIFFS, "/byline.html");

    // Start the server
    server.begin();
    Serial.println("HTTP server started");

    // Initialize WebSocket
    webSocket.begin();
    webSocket.onEvent(handleWebSocketMessage); // Set the WebSocket event handler
}

void loop() {
    server.handleClient();
    webSocket.loop();
    delay(10); // Add a small delay to avoid watchdog timer reset
    Serial.printf("Free heap memory: %u\n", ESP.getFreeHeap());

    // Check Wi-Fi connection
    if (WiFi.softAPgetStationNum() == 0) {
        Serial.println("No clients connected, restarting AP...");
        WiFi.softAPdisconnect(true);
        WiFi.softAP(ssid, password);
        IPAddress apIP = WiFi.softAPIP();
        Serial.printf("Access Point IP address: %s\n", apIP.toString().c_str());
    }
}

// Function to handle WebSocket messages
void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String message = (char *)payload;
        message.trim(); // Trim any extra whitespace
        Serial.printf("Received WebSocket message: %s\n", message.c_str());

        // Parse the message and execute corresponding actions
        if (message.startsWith("f,")) {
            int time_ms_forward = message.substring(2).toInt();
            moveForward(time_ms_forward);
        } else if (message.startsWith("b,")) {
            int time_ms_backward = message.substring(2).toInt();
            moveBackward(time_ms_backward);
        } else if (message.startsWith("r,")) {
            int time_ms_right = message.substring(2).toInt();
            turnRight(time_ms_right);
        } else if (message.startsWith("l,")) {
            int time_ms_left = message.substring(2).toInt();
            turnLeft(time_ms_left);
        } else if (message.startsWith("p,")) {
            pwm = message.substring(2).toInt();
            analogWrite(motor1a, pwm); // Example for motor1a
            analogWrite(motor2a, pwm); // Example for motor2a
            Serial.print("Setting PWM to: ");
            Serial.println(pwm);
        } else if (message.startsWith("s,")) {
            int servoPos = message.substring(2).toInt();
            servo.write(servoPos);
            Serial.print("Setting servo position to: ");
            Serial.println(servoPos);
        } else {
            String errorMessage = "Invalid command: " + message;
            sendErrorMessage(errorMessage);
        }
        delay(100); // Add a small delay to avoid overloading the system
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("Client [%u] disconnected.\n", num);
    } else if (type == WStype_CONNECTED) {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("Client [%u] connected from %s.\n", num, ip.toString().c_str());
        webSocket.sendTXT(num, "Connected to Boxy!");
    }
}

void sendErrorMessage(String message) {
    webSocket.broadcastTXT(message); // Send to all connected clients
    Serial.println("Sent error message: " + message);
}

void moveForward(int duration) {
    digitalWrite(motor1a, HIGH);
    digitalWrite(motor1b, LOW);
    digitalWrite(motor2a, HIGH);
    digitalWrite(motor2b, LOW);
    delay(duration);
    stopMotors();
    Serial.println("Moving forward");
}

void moveBackward(int duration) {
    digitalWrite(motor1a, LOW);
    digitalWrite(motor1b, HIGH);
    digitalWrite(motor2a, LOW);
    digitalWrite(motor2b, HIGH);
    delay(duration);
    stopMotors();
    Serial.println("Moving backward");
}

void turnRight(int duration) {
    digitalWrite(motor1a, HIGH);
    digitalWrite(motor1b, LOW);
    digitalWrite(motor2a, LOW);
    digitalWrite(motor2b, HIGH);
    delay(duration);
    stopMotors();
    Serial.println("Turning right");
}

void turnLeft(int duration) {
    digitalWrite(motor1a, LOW);
    digitalWrite(motor1b, HIGH);
    digitalWrite(motor2a, HIGH);
    digitalWrite(motor2b, LOW);
    delay(duration);
    stopMotors();
    Serial.println("Turning left");
}

void stopMotors() {
    digitalWrite(motor1a, LOW);
    digitalWrite(motor1b, LOW);
    digitalWrite(motor2a, LOW);
    digitalWrite(motor2b, LOW);
}
