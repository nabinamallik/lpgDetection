#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
#define FIREBASE_HOST "https://lpgdetectionllp-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyATV1e7uytyRCnRG6haEFGO9gsz5N6P7mo"
const char* ssid = "vivo T2x 5G";
const char* password = "11111111";

// Firebase objects
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData firebaseData;
FirebaseJson json;

// Sensor pins
const int gasSensorPin = 34;

// Buzzer, LED, and fan pins
const int buzzerPin = 25;
const int redLEDPin = 26;
const int fanPin = 27;

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global variables for fan and buzzer timers
unsigned long fanStartTime = 0;
unsigned long buzzerStartTime = 0;
bool fanRunning = false;
bool buzzerRunning = false;

// Variables for gas level averaging
const int numReadings = 10;  // Number of readings to average
int readings[numReadings];   // Array to store readings
int readIndex = 0;           // Index of the current reading
int total = 0;               // Sum of all readings
int averageGasLevel = 0;     // The average gas level

// Setup
void setup() {
  Serial.begin(115200);

  // Initialize sensor pins
  pinMode(gasSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi.");

  // Set Firebase configuration
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;
  
  // Optional: Assign the user authentication data if needed
  auth.user.email = "pankajakumarsahoo2004@gmail.com";
  auth.user.password = "pankajakumarsahoo";
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize gas readings array
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  // Display welcome message on OLED for 2 seconds
  displayWelcomeMessage();
}

// Loop
void loop() {
  // Take multiple readings and average them to reduce fluctuations
  takeGasReading();

  // Display averaged gas level and warnings on OLED
  displayDataOnOLED(averageGasLevel);

  // Control fan, buzzer, and LED based on gas level
  if (averageGasLevel > 2020 && !fanRunning) {
    activateAlarmAndFan(averageGasLevel);
  }

  // Manage fan running time
  if (fanRunning && (millis() - fanStartTime > 30000)) {
    stopFan();
  }

  // Manage buzzer running time (7 seconds)
  if (buzzerRunning && (millis() - buzzerStartTime > 7000)) {
    stopBuzzer();
  }

  // Add a small delay to reduce rapid fluctuation
  delay(200);
}

// Function to take gas readings and calculate the average
void takeGasReading() {
  // Subtract the last reading
  total = total - readings[readIndex];

  // Read the new gas level
  readings[readIndex] = analogRead(gasSensorPin);

  // Add the new reading to the total
  total = total + readings[readIndex];

  // Advance to the next position in the array
  readIndex = (readIndex + 1) % numReadings;

  // Calculate the average gas level
  averageGasLevel = total / numReadings;

  // Create JSON data
  json.clear();
  json.set("/gasReading", averageGasLevel);

  // Push data to Firebase
  if (Firebase.updateNode(firebaseData, "/Sensor", json)) {
    Serial.println("Data updated successfully");
  } else {
    Serial.println("Failed to update data");
    Serial.println(firebaseData.errorReason());
  }
}

// Function to display welcome message
void displayWelcomeMessage() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("WELCOME TO");
  display.println("LPG GAS");
  display.println("DETECTION");
  display.println("SYSTEM");
  display.display();
  delay(2000);  // Wait for 2 seconds
}

// Function to display gas level on OLED
void displayDataOnOLED(int gasLevel) {
  display.clearDisplay();

  // Display gas level in bold
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.println("Gas Level:");
  display.setTextSize(3);
  display.println(gasLevel);

  // Show warning if gas level is too high
  if (gasLevel > 2020) {
    display.setTextSize(2);
    display.setCursor(0, 50);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.println("WARNING");
  }

  display.display();
}

// Function to activate alarm and fan
void activateAlarmAndFan(int gasLevel) {
  digitalWrite(buzzerPin, HIGH);
  digitalWrite(redLEDPin, HIGH);

  // Set fan speed based on gas level
  int fanSpeed = map(gasLevel, 2020, 4095, 128, 255);  // Proportional fan speed from half to full speed
  analogWrite(fanPin, fanSpeed);

  // Start fan and buzzer timers
  fanStartTime = millis();
  buzzerStartTime = millis();

  fanRunning = true;
  buzzerRunning = true;

  // Ensure the warning is shown when gas level exceeds threshold
  displayDataOnOLED(gasLevel);  // Update OLED with warning
}

// Function to stop the fan after 30 seconds
void stopFan() {
  analogWrite(fanPin, 0);  // Turn off fan
  fanRunning = false;
}

// Function to stop the buzzer after 7 seconds
void stopBuzzer() {
  digitalWrite(buzzerPin, LOW);  // Turn off buzzer
  digitalWrite(redLEDPin, LOW);  // Turn off LED
  buzzerRunning = false;
}
