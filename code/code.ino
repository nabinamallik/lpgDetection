#include <WiFi.h>
#include <Preferences.h>
#include <FirebaseESP32.h>

Preferences preferences;

char ssid[32];
char password[64];

FirebaseConfig config;
FirebaseAuth auth;

// Firebase data object
FirebaseData firebaseData;

const int buzzerPin = 19;

void setup() {
  Serial.begin(115200);
  preferences.begin("wifi_creds", false);
  pinMode(buzzerPin, OUTPUT);

  // Check if credentials are stored
  if (preferences.getString("ssid", ssid, sizeof(ssid)) && 
      preferences.getString("password", password, sizeof(password))) {
    Serial.println("Credentials found in NVS. Connecting...");

    // Connect to WiFi using stored credentials
    WiFi.begin(ssid, password);

  } else {
    Serial.println("No credentials found. Please enter:");
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
  }
  
  // Set Firebase configuration
  config.host = "buzzer-72273-default-rtdb.asia-southeast1.firebasedatabase.app";
  config.api_key = "AIzaSyClmdSDU0xx8xQ94fZ8IFxjYY6-QJeOn4I";
  
  // Optional: Assign the user authentication data if needed
  auth.user.email = "nabinamallik2003@gmail.com";
  auth.user.password = "nabinamallik";
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Start listening to changes in the "/buzzerStatus" node
  if (!Firebase.beginStream(firebaseData, "/buzzerStatus")) {
    Serial.println("Could not start stream");
    Serial.println(firebaseData.errorReason());
  }

  // Set callback functions
  Firebase.setStreamCallback(firebaseData, streamCallback, streamTimeoutCallback);
}

void loop() {
  // Check for Serial input to trigger credential change
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove leading/trailing whitespace

    if (command == "setwifi") {
      Serial.println("Enter new WiFi SSID:");
      while (!Serial.available()) {
        delay(1);
      }
      int ssidLen = Serial.readBytesUntil('\n', ssid, sizeof(ssid) - 1);
      ssid[ssidLen] = '\0';

      Serial.println("Enter new WiFi Password:");
      while (!Serial.available()) {
        delay(1);
      }
      int passwordLen = Serial.readBytesUntil('\n', password, sizeof(password) - 1);
      password[passwordLen] = '\0';

      // Store new credentials in NVS
      preferences.putString("ssid", ssid);
      preferences.putString("password", password);

      Serial.println("New credentials saved. Restarting...");
      ESP.restart(); // Restart the ESP32 to use the new credentials
    }
  }
}

void streamCallback(StreamData data) {
  if (data.dataType() == "int") {
    int buzzerState = data.intData();
    digitalWrite(buzzerPin, buzzerState);
    Serial.println(buzzerState == HIGH ? "Buzzer ON" : "Buzzer OFF");
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Stream timeout, reconnecting...");
    Firebase.beginStream(firebaseData, "/buzzerStatus");
  }
}