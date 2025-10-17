#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Servo.h>
#include <HEMS_inferencing.h>
// #include "tflite_learn_4_compiled.h"  // Edge Impulse compiled model header

// Pin Definitions
#define IR_RECEIVE_PIN 5         // IR Receiver pin
#define LDR_PIN A0                // Analog pin for LDR sensor
#define HOUSE_LIGHT_PIN 4        // Digital pin for house light control (active LOW)
#define SERVO_PIN 12              // PWM pin for curtain servo control

// WiFi Credentials
const char* ssid = "Amrit";
const char* password = "kali@254";

// Create HTTP and WebSocket servers
ESP8266WebServer httpServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// IR receiver setup
IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

// Servo for curtain control
Servo curtainServo;

// Global state variables
bool occupancy = false;             // True if someone is present (set via IR or UI)
bool houseLightState = false;       // Current state of house light (true = ON)
unsigned long lastInferenceTime = 0;
const unsigned long inferenceInterval = 5000; // Run inference every 5 seconds

// IR code for toggling house light
const uint32_t IR_TOGGLE_CODE = 0x807F4AB5;

// WebSocket event handler: process incoming commands from the UI
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = String((char*)payload);
    Serial.print("WebSocket message received: ");
    Serial.println(msg);
    // Look for house light toggle command from UI with occupancy info.
    if (msg.indexOf("\"device\":\"houseLight\"") != -1 && msg.indexOf("\"action\":\"toggle\"") != -1) {
      if (msg.indexOf("\"occupancy\":1") != -1) {
        houseLightState = true;
        occupancy = true;
        digitalWrite(HOUSE_LIGHT_PIN, LOW);
        Serial.println("House Light turned ON via UI toggle. Occupancy set to TRUE.");
      } else if (msg.indexOf("\"occupancy\":0") != -1) {
        houseLightState = false;
        occupancy = false;
        digitalWrite(HOUSE_LIGHT_PIN, HIGH);
        Serial.println("House Light turned OFF via UI toggle. Occupancy set to FALSE.");
      }
    }
  }
}

//new part
String runInference(float ldrValue, float occupancyValue) {
    // Create input buffer aligned with your Edge Impulse model's expectations
    static float input_buf[] = {ldrValue, occupancyValue};
    
    // Create signal structure
    signal_t signal;
    numpy::signal_from_buffer(input_buf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

    // Initialize and run classifier
    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    
    if (res != EI_IMPULSE_OK) {
        return "Inference failed";
    }

    // Get prediction with highest confidence
    String prediction = "";
    float best_score = -1.0;
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > best_score) {
            best_score = result.classification[ix].value;
            prediction = result.classification[ix].label;
        }
    }
    
    return prediction;
}



/*
// runInference: calls the Edge Impulse model inference using LDR and occupancy
String runInference(float ldrValue, float occupancyValue) {
  float inputData[2];
  inputData[0] = ldrValue;
  inputData[1] = occupancyValue;
  
  ei_impulse_result_t result = {0};
  run_classifier(inputData, &result); // Call the Edge Impulse inference function
  
  // Assume the model returns a composite label such as:
  // "Turn Off House Light|Open Curtain" or "Turn On House Light|Keep Curtain Closed"
  String prediction = String(result.classification[0].label);
  return prediction;
}
*/


void setup() {
  Serial.begin(115200);
  Serial.println("ESP8266 TinyML Energy Optimization Starting...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS for serving UI files
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  
  httpServer.serveStatic("/", LittleFS, "/");
  httpServer.begin();
  Serial.println("HTTP server started on port 80");
  
  // Initialize WebSocket server and assign event handler
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println("WebSocket server started on port 81");

  // Initialize IR receiver
  irrecv.enableIRIn();
  Serial.println("IR Receiver enabled.");

  // Initialize house light control pin (active LOW: LOW = ON)
  pinMode(HOUSE_LIGHT_PIN, OUTPUT);
  digitalWrite(HOUSE_LIGHT_PIN, HIGH); // Start with house light off
  
  // Initialize servo for curtain control (0° = closed, 90° = open)
  curtainServo.attach(SERVO_PIN);
  curtainServo.write(0);
}

void loop() {
  // Handle HTTP and WebSocket client requests
  httpServer.handleClient();
  webSocket.loop();
  
  // Check for IR signals
  if (irrecv.decode(&results)) {
    uint32_t code = results.value;
    Serial.print("IR Code received: ");
    Serial.println(code, HEX);
    if (code == IR_TOGGLE_CODE) {
      houseLightState = !houseLightState;
      if (houseLightState) {
        digitalWrite(HOUSE_LIGHT_PIN, LOW);
        occupancy = true;
        Serial.println("House Light turned ON via IR. Occupancy set to TRUE.");
      } else {
        digitalWrite(HOUSE_LIGHT_PIN, HIGH);
        occupancy = false;
        Serial.println("House Light turned OFF via IR. Occupancy set to FALSE.");
      }
    }
    irrecv.resume();
  }
  
  // Run ML inference every inferenceInterval milliseconds
  if (millis() - lastInferenceTime >= inferenceInterval) {
    lastInferenceTime = millis();
    
    int ldrValue = analogRead(LDR_PIN);
    Serial.print("LDR Reading: ");
    Serial.println(ldrValue);
    
    float occ = occupancy ? 1.0f : 0.0f;
    String modelPrediction = runInference((float)ldrValue, occ);
    Serial.print("Model Prediction: ");
    Serial.println(modelPrediction);
    
    // Control House Light based on model prediction
    if (modelPrediction.indexOf("Turn Off House Light") != -1) {
      digitalWrite(HOUSE_LIGHT_PIN, HIGH);
      houseLightState = false;
      Serial.println("House Light turned OFF by ML prediction.");
    } else if (modelPrediction.indexOf("Turn On House Light") != -1) {
      digitalWrite(HOUSE_LIGHT_PIN, LOW);
      houseLightState = true;
      Serial.println("House Light turned ON by ML prediction.");
    }
    
    // Control Curtain based on model prediction (servo movement)
    if (modelPrediction.indexOf("Open Curtain") != -1) {
      curtainServo.write(90);
      Serial.println("Curtain opened (90°) by ML prediction.");
    } else if (modelPrediction.indexOf("Keep Curtain Closed") != -1) {
      curtainServo.write(0);
      Serial.println("Curtain kept closed (0°) by ML prediction.");
    }
    
    // Broadcast sensor and prediction data via WebSocket for UI updates
    String json = "{\"ldrValue\":" + String(ldrValue) +
                  ",\"houseLight\":" + String(houseLightState ? 1 : 0) +
                  ",\"modelPrediction\":\"" + modelPrediction + "\"}";
    webSocket.broadcastTXT(json);
    
    // Reset occupancy after inference cycle
    occupancy = false;
  }
  
  delay(100);
}
