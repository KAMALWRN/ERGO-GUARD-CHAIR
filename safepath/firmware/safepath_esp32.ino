/**************************************************************
 * SafePath – Intelligent Fire Evacuation System
 * Board: ESP32 DevKit
 * LCD: 16x2 I2C (address 0x27)
 * Blynk: v2.0
 **************************************************************/

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------- Blynk credentials (replace with your values) -----
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "SafePath"
#define BLYNK_AUTH_TOKEN    "YOUR_DEVICE_AUTH_TOKEN"

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// -------- LCD ------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- Pins -----------
const int FLAME_PIN   = 34;  // analog
const int SMOKE_A_PIN = 35;  // analog (near Exit A corridor)
const int SMOKE_B_PIN = 32;  // analog (near Exit B corridor)

const int LED_A_GREEN = 14;
const int LED_A_RED   = 27;
const int LED_B_GREEN = 26;
const int LED_B_RED   = 25;

// Thresholds (tune experimentally with Serial monitor)
int flameThreshold   = 1500;
int smokeAThreshold  = 1500;
int smokeBThreshold  = 1500;

// Status variables
bool fireDetected    = false;
bool exitABlocked    = false;
bool exitBBlocked    = false;
String currentPath   = "Normal";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // LCD init
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SafePath System");
  lcd.setCursor(0,1);
  lcd.print("Connecting WiFi");

  // Pin modes
  pinMode(FLAME_PIN,   INPUT);
  pinMode(SMOKE_A_PIN, INPUT);
  pinMode(SMOKE_B_PIN, INPUT);

  pinMode(LED_A_GREEN, OUTPUT);
  pinMode(LED_A_RED,   OUTPUT);
  pinMode(LED_B_GREEN, OUTPUT);
  pinMode(LED_B_RED,   OUTPUT);

  setNormalLeds();

  // Connect Blynk / WiFi
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SafePath Ready");
  lcd.setCursor(0,1);
  lcd.print("Monitoring...");
}

void loop() {
  Blynk.run();
  handleSensors();
  delay(500);  // 2 Hz update
}

void handleSensors() {
  int flameVal   = analogRead(FLAME_PIN);
  int smokeAVal  = analogRead(SMOKE_A_PIN);
  int smokeBVal  = analogRead(SMOKE_B_PIN);

  Serial.print("Flame: "); Serial.print(flameVal);
  Serial.print(" | SmokeA: "); Serial.print(smokeAVal);
  Serial.print(" | SmokeB: "); Serial.println(smokeBVal);

  // Basic fire detection
  fireDetected = (flameVal > flameThreshold) ||
                 (smokeAVal > smokeAThreshold) ||
                 (smokeBVal > smokeBThreshold);

  // Determine blocked exits
  exitABlocked = (smokeAVal > smokeAThreshold); // corridor A dangerous
  exitBBlocked = (smokeBVal > smokeBThreshold); // corridor B dangerous

  if (!fireDetected) {
    currentPath = "Normal";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("No Fire Detected");
    lcd.setCursor(0,1);
    lcd.print("All exits normal");
    setNormalLeds();
    sendBlynkStatus("No fire detected", flameVal, smokeAVal, smokeBVal);
    return;
  }

  // Fire detected: evaluate safest path
  if (exitABlocked && !exitBBlocked) {
    // Use Exit B
    currentPath = "Use Exit B";
    showPathOnLeds(false, true);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fire near Exit A");
    lcd.setCursor(0,1);
    lcd.print("Use Exit B (GREEN)");
    sendBlynkStatus("Fire near Exit A - Use Exit B", flameVal, smokeAVal, smokeBVal);

  } else if (!exitABlocked && exitBBlocked) {
    // Use Exit A
    currentPath = "Use Exit A";
    showPathOnLeds(true, false);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fire near Exit B");
    lcd.setCursor(0,1);
    lcd.print("Use Exit A (GREEN)");
    sendBlynkStatus("Fire near Exit B - Use Exit A", flameVal, smokeAVal, smokeBVal);

  } else if (!exitABlocked && !exitBBlocked) {
    // Fire in hall only, both exits free: choose A as shortest
    currentPath = "Use Exit A";
    showPathOnLeds(true, false);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Fire Detected");
    lcd.setCursor(0,1);
    lcd.print("Follow Exit A");
    sendBlynkStatus("Fire detected - Use Exit A", flameVal, smokeAVal, smokeBVal);

  } else {
    // Both exits blocked
    currentPath = "All blocked";
    showBothBlocked();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ALL EXITS BLOCKED");
    lcd.setCursor(0,1);
    lcd.print("Wait for rescue");
    sendBlynkStatus("All exits blocked - Wait for rescue", flameVal, smokeAVal, smokeBVal);
  }
}

// ---------------- LED helpers ----------------

void setNormalLeds() {
  digitalWrite(LED_A_GREEN, HIGH);
  digitalWrite(LED_A_RED,   LOW);
  digitalWrite(LED_B_GREEN, HIGH);
  digitalWrite(LED_B_RED,   LOW);
}

void showPathOnLeds(bool useA, bool useB) {
  // Exit A
  digitalWrite(LED_A_GREEN, useA ? HIGH : LOW);
  digitalWrite(LED_A_RED,   useA ? LOW  : HIGH);
  // Exit B
  digitalWrite(LED_B_GREEN, useB ? HIGH : LOW);
  digitalWrite(LED_B_RED,   useB ? LOW  : HIGH);
}

void showBothBlocked() {
  digitalWrite(LED_A_GREEN, LOW);
  digitalWrite(LED_B_GREEN, LOW);
  digitalWrite(LED_A_RED,   HIGH);
  digitalWrite(LED_B_RED,   HIGH);
}

// --------------- Blynk helpers ---------------

void sendBlynkStatus(const String& msg, int flameVal, int smokeAVal, int smokeBVal) {
  Blynk.virtualWrite(V0, msg);        // Alert message
  Blynk.virtualWrite(V1, fireDetected ? 1 : 0);
  Blynk.virtualWrite(V2, smokeAVal);
  Blynk.virtualWrite(V3, smokeBVal);

  static bool lastFire = false;
  if (fireDetected && !lastFire) {
    // Configure an event named "fire_alert" in Blynk web dashboard
    Blynk.logEvent("fire_alert", msg);
  }
  lastFire = fireDetected;
}


