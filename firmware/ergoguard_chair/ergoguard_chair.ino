/*
 * ErgoGuard Chair - ESP32 Firmware (Simplified for 4 FSR + MUX)
 * Version A (4-sensor configuration)
 *
 * Sensors:
 * - 4x FSR pressure sensors:
 *     - Seat Front
 *     - Seat Back
 *     - Backrest Upper
 *     - Backrest Lower
 * - MPU6050 IMU (motion detection)
 *
 * Actuators:
 * - 2x Vibration motors (seat + back)
 * - 2x Single-color LEDs (GREEN = good, RED = alert)
 *
 * NOTE: Environment sensors (MQ135, DHT22) and RGB LED strip have been removed
 *       to match the available hardware.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>  // Use Adafruit version
#include <Adafruit_Sensor.h>   // Required for getEvent

// ==================== PIN DEFINITIONS ====================
// 16-channel Analog MUX (e.g., CD74HC4067)
#define MUX_SIG 34   // Analog input from MUX to ESP32 (ADC1)
#define MUX_S0 23
#define MUX_S1 19
#define MUX_S2 18
#define MUX_S3 5

// MUX channel mapping for 4 FSRs
#define CH_SEAT_FRONT_LEFT  0
#define CH_SEAT_FRONT_RIGHT 4  // MUX channel 4
#define CH_SEAT_BACK    1  // MUX channel 1
#define CH_BACK_UPPER   2  // MUX channel 2
#define CH_BACK_LOWER   3  // MUX channel 3

// Demo sensitivity boost: your "human sitting" values are ~300, finger press ~1000.
// Multiply ADC readings so sitting shows strongly in UI and scoring.
const float FSR_GAIN = 4.0f;

// I2C (MPU6050)
#define SDA_PIN 21
#define SCL_PIN 22

// Actuators
#define VIB_MOTOR_1 25  // Seat motor (via 2N2222 + 330Ω)
#define VIB_MOTOR_2 26  // Backrest motor (via 2N2222 + 330Ω)

// Status LEDs (with 330Ω series resistors)
#define LED_GREEN 27    // Good posture
#define LED_RED   14    // Warning / Critical

// ==================== SENSOR OBJECTS ====================
Adafruit_MPU6050 mpu;

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";
const char* serverURL = "http://192.168.250.92:3000/api/sensor-data";

// ==================== SENSOR DATA STRUCTURE ====================
struct SensorData {
  // FSR Readings (0-4095 on ESP32 ADC)
  int fsr_seat_front_left;
  int fsr_seat_front_right;
  int fsr_seat_front;   // averaged value
  int fsr_seat_back;
  int fsr_back_upper;
  int fsr_back_lower;

  // IMU Data
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float pitch, roll;

  // Calculated Metrics
  float posture_score;
  float asymmetry_score;
  float sedentary_score;
  float ergo_stress_score;

  unsigned long timestamp;
};

SensorData sensorData;

// ==================== CALIBRATION & STATE ====================
bool isCalibrated = false;
unsigned long calibrationStartTime = 0;
const unsigned long CALIBRATION_DURATION = 10000; // 10 seconds (demo-friendly)

// Baseline values (set during calibration)
float baseline_pressure[5] = {0, 0, 0, 0,0};
float baseline_pitch = 0;
float baseline_roll = 0;

// Personal max pressure learned during calibration (demo feature)
int max_pressure[5] = {1, 1, 1, 1, 1};

// Sitting / sedentary state
unsigned long sittingStartTime = 0;
unsigned long lastMovementTime = 0;
bool isCurrentlySitting = false;

// High-level status (for dashboard)
String statusMessage = "Booting";

// Posture classification (for demo logic)
enum PostureState {
  POSTURE_NOT_SITTING,
  POSTURE_UPRIGHT,
  POSTURE_CURVED,
  POSTURE_SLOUCH
};

PostureState currentPostureState = POSTURE_NOT_SITTING;
unsigned long badPostureStartTime = 0;
bool wasBadPostureLastFrame = false;
bool redBlinkState = false;
unsigned long lastBlinkToggleTime = 0;

// ==================== INTERVENTION STATE ====================
enum InterventionLevel {
  INTERVENTION_NONE,
  INTERVENTION_LOW,
  INTERVENTION_MEDIUM,
  INTERVENTION_HIGH
};

InterventionLevel currentIntervention = INTERVENTION_NONE;
unsigned long lastInterventionTime = 0;
const unsigned long INTERVENTION_COOLDOWN = 1000; // 30 seconds

// Forward declaration
void sendDataToServer();

// ==================== MUX HELPER ====================
int readMuxChannel(uint8_t channel) {
  // Set address lines S0-S3
  digitalWrite(MUX_S0, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX_S1, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX_S2, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(MUX_S3, (channel & 0x08) ? HIGH : LOW);

  delayMicroseconds(50); // small settling time
  return analogRead(MUX_SIG);
}

int scaleFSR(int raw) {
  int scaled = (int)(raw * FSR_GAIN);
  if (scaled < 0) return 0;
  if (scaled > 4095) return 4095;
  return scaled;
}

int toPct(int value, int maxVal) {
  if (maxVal <= 0) return 0;
  int pct = (int)((value * 100.0f) / maxVal);
  if (pct < 0) return 0;
  if (pct > 100) return 100;
  return pct;
}

// ==================== LED & VIBRATION HELPERS ====================
void setStatusLED(bool greenOn, bool redOn) {
  digitalWrite(LED_GREEN, greenOn ? HIGH : LOW);
  digitalWrite(LED_RED,   redOn   ? HIGH : LOW);
}

void setVibration(bool on) {
  digitalWrite(VIB_MOTOR_1, on ? HIGH : LOW);
  digitalWrite(VIB_MOTOR_2, on ? HIGH : LOW);
}

void sendStatusOnlyUpdate() {
  // Keep timestamp fresh so dashboard sees this as newest status.
  sensorData.timestamp = millis();
  if (WiFi.status() == WL_CONNECTED) {
    sendDataToServer();
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ErgoGuard Chair (4-FSR) - Initializing...");
  statusMessage = "Booting...";

  // MUX control pins
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
  //pinMode(MUX_SIG, INPUT);

  // Actuators
  pinMode(VIB_MOTOR_1, OUTPUT);
  pinMode(VIB_MOTOR_2, OUTPUT);
  digitalWrite(VIB_MOTOR_1, LOW);
  digitalWrite(VIB_MOTOR_2, LOW);

  // LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  setStatusLED(false, false);

  // Initialize I2C + MPU6050
  Wire.begin(SDA_PIN, SCL_PIN);
  if (mpu.begin()) {
    Serial.println("MPU6050 initialized");
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  } else {
    Serial.println("MPU6050 initialization failed!");
  }

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  statusMessage = "Connecting to WiFi...";
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    statusMessage = "WiFi connected";
    sendStatusOnlyUpdate();
  } else {
    Serial.println("\nWiFi connection failed (offline mode).");
    statusMessage = "WiFi failed (offline mode)";
  }

  // Start calibration
  Serial.println("Starting calibration... Sit normally for 10 seconds.");
  statusMessage = "Calibrating - sit normally for 10 sec";
  sendStatusOnlyUpdate();
  calibrationStartTime = millis();
  sittingStartTime = millis();
  lastMovementTime = millis();

  // Yellow-ish = both LEDs ON during calibration
  setStatusLED(true, true);
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();

  // Calibration phase
  if (!isCalibrated) {
    if (currentTime - calibrationStartTime < CALIBRATION_DURATION) {
      calibrateSensors();
      // Push calibration state so dashboard can display startup status messages.
      if (WiFi.status() == WL_CONNECTED) {
        static unsigned long lastCalibrationStatusPush = 0;
        if (currentTime - lastCalibrationStatusPush >= 1000) {
          sendDataToServer();
          lastCalibrationStatusPush = currentTime;
        }
      }
      delay(100);
      return;
    } else {
      finishCalibration();
      isCalibrated = true;
      Serial.println("Calibration complete!");
      statusMessage = "Calibration complete - monitoring posture";
      sendStatusOnlyUpdate();
    }
  }

  // Read all sensors
  readFSRSensors();
  readIMU();

  // Calculate metrics
  calculatePostureScore();
  calculateAsymmetryScore();
  calculateSedentaryScore();

  // Calculate final Ergo-Stress Score
  calculateErgoStressScore();

  // Update intervention
  updateIntervention();

  // Send data to server
  if (WiFi.status() == WL_CONNECTED) {
    sendDataToServer();
  }

  // Print debug info
  printDebugInfo();

  delay(100); // 10 Hz update rate
}

// ==================== SENSOR READING FUNCTIONS ====================
void readFSRSensors() {

  // read both front sensors
  sensorData.fsr_seat_front_left  = scaleFSR(readMuxChannel(CH_SEAT_FRONT_LEFT));
  sensorData.fsr_seat_front_right = scaleFSR(readMuxChannel(CH_SEAT_FRONT_RIGHT));

  // combine them (use MAX so crossed legs still detected)
  sensorData.fsr_seat_front =
    max(sensorData.fsr_seat_front_left, sensorData.fsr_seat_front_right);

  // read other sensors
  sensorData.fsr_seat_back  = scaleFSR(readMuxChannel(CH_SEAT_BACK));
  sensorData.fsr_back_upper = scaleFSR(readMuxChannel(CH_BACK_UPPER));
  sensorData.fsr_back_lower = scaleFSR(readMuxChannel(CH_BACK_LOWER));
}
void readIMU() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  sensorData.accel_x = accel.acceleration.x;
  sensorData.accel_y = accel.acceleration.y;
  sensorData.accel_z = accel.acceleration.z;

  sensorData.gyro_x = gyro.gyro.x;
  sensorData.gyro_y = gyro.gyro.y;
  sensorData.gyro_z = gyro.gyro.z;

  // Calculate pitch and roll (in degrees)
  sensorData.pitch = atan2(accel.acceleration.y, accel.acceleration.z) * 180.0 / PI;
  sensorData.roll  = atan2(accel.acceleration.x,
                           sqrt(accel.acceleration.y * accel.acceleration.y +
                                accel.acceleration.z * accel.acceleration.z)) * 180.0 / PI;
}

// ==================== CALIBRATION FUNCTIONS ====================
void calibrateSensors() {
  static int sampleCount = 0;
  sampleCount++;

  readFSRSensors();
  readIMU();

  // Moving average for baseline pressure
  baseline_pressure[0] = (baseline_pressure[0] * (sampleCount - 1) + sensorData.fsr_seat_front) / sampleCount;
  baseline_pressure[1] = (baseline_pressure[1] * (sampleCount - 1) + sensorData.fsr_seat_back) / sampleCount;
  baseline_pressure[2] = (baseline_pressure[2] * (sampleCount - 1) + sensorData.fsr_back_upper) / sampleCount;
  baseline_pressure[3] = (baseline_pressure[3] * (sampleCount - 1) + sensorData.fsr_back_lower) / sampleCount;

  // Track personal max pressure during calibration (per-sensor)
  max_pressure[0] = max(max_pressure[0], sensorData.fsr_seat_front_left);
  max_pressure[1] = max(max_pressure[1], sensorData.fsr_seat_front_right);
  max_pressure[2] = max(max_pressure[2], sensorData.fsr_seat_back);
  max_pressure[3] = max(max_pressure[3], sensorData.fsr_back_upper);
  max_pressure[4] = max(max_pressure[4], sensorData.fsr_back_lower);

  baseline_pitch = (baseline_pitch * (sampleCount - 1) + sensorData.pitch) / sampleCount;
  baseline_roll  = (baseline_roll  * (sampleCount - 1) + sensorData.roll)  / sampleCount;
}

void finishCalibration() {
  Serial.println("Baseline values (FSR):");
  Serial.print("Seat Front (combined): "); Serial.println(baseline_pressure[0]);
  Serial.print("Seat Back           : "); Serial.println(baseline_pressure[1]);
  Serial.print("Back Upper          : "); Serial.println(baseline_pressure[2]);
  Serial.print("Back Lower          : "); Serial.println(baseline_pressure[3]);
  Serial.println("Max values learned (per sensor):");
  Serial.print("Front L max: "); Serial.println(max_pressure[0]);
  Serial.print("Front R max: "); Serial.println(max_pressure[1]);
  Serial.print("Seat Back max: "); Serial.println(max_pressure[2]);
  Serial.print("Back Upper max: "); Serial.println(max_pressure[3]);
  Serial.print("Back Lower max: "); Serial.println(max_pressure[4]);
  Serial.print("Baseline Pitch: "); Serial.println(baseline_pitch);
  Serial.print("Baseline Roll : "); Serial.println(baseline_roll);
}

// ==================== SCORE CALCULATION FUNCTIONS ====================
void calculatePostureScore() {
  float score = 100.0;

  // Thresholds (tune these based on your actual readings)
  const float TH = 15; // minimum pressure to consider "active"

  bool frontOn = sensorData.fsr_seat_front   > TH;
  bool backOn  = sensorData.fsr_seat_back    > TH;
  bool upperOn = sensorData.fsr_back_upper   > TH;
  bool lowerOn = sensorData.fsr_back_lower   > TH;

  // Total pressures for quick "not sitting" detection
  float seatTotal = sensorData.fsr_seat_front + sensorData.fsr_seat_back;
  float backTotal = sensorData.fsr_back_upper + sensorData.fsr_back_lower;

  // 1) Not sitting: no sensor has pressure
  if (!frontOn && !backOn && !upperOn && !lowerOn) {
    currentPostureState = POSTURE_NOT_SITTING;
    score = 100.0f;
    sensorData.posture_score = score;
    wasBadPostureLastFrame = false;
    badPostureStartTime = millis();
    return;
  }

  // 2) Sitting upright: all four sensors active
  if (frontOn && backOn && upperOn && lowerOn) {
    currentPostureState = POSTURE_UPRIGHT;
    score = 100.0f;
  }
  // 3) Curved (hunched forward in upper spine): upper + front active, lower/back mostly off
  else if (upperOn && frontOn && !backOn && !lowerOn) {
    currentPostureState = POSTURE_CURVED;
    score = 60.0f;
    statusMessage = "Sit up straight - align your back with the backrest";
  }
  // 4) Slouching forward:
  //    - lower + back + front are active (3 sensors)
  //    - OR only front + back active (2 seat sensors) with little backrest contact
  else if ( (lowerOn && backOn && frontOn) ||
            (frontOn && backOn && !upperOn && !lowerOn) ) {
    currentPostureState = POSTURE_SLOUCH;
    score = 30.0f;
  }
  // 5) Any other mixed pattern: treat as slouching (for demo) but slightly better than worst
  else {
    currentPostureState = POSTURE_SLOUCH;
    score = 40.0f;
  }

  // Track how long posture has been "bad" (curved or slouching)
  bool isBad = (currentPostureState == POSTURE_CURVED || currentPostureState == POSTURE_SLOUCH);
  unsigned long now = millis();
  if (isBad) {
    if (!wasBadPostureLastFrame) {
      badPostureStartTime = now;
    }
    wasBadPostureLastFrame = true;
  } else {
    wasBadPostureLastFrame = false;
    badPostureStartTime = now;
  }

  sensorData.posture_score = constrain(score, 0.0f, 100.0f);
}

void calculateAsymmetryScore() {
  float score = 100.0;

  // Seat front vs back (leaning forward/backward)
  float seatFront  = sensorData.fsr_seat_front;
  float seatBack   = sensorData.fsr_seat_back;
  float seatTotal  = max(1.0f, seatFront + seatBack);
  float seatDiff   = fabs(seatFront - seatBack) / seatTotal; // 0..1

  if (seatDiff > 0.4f) {
    score -= 10;
  } else if (seatDiff > 0.25f) {
    score -= 5;
  }

  // Back upper vs lower (hunching vs upright)
  float backUpper  = sensorData.fsr_back_upper;
  float backLower  = sensorData.fsr_back_lower;
  float backTotal  = max(1.0f, backUpper + backLower);
  float backDiff   = fabs(backUpper - backLower) / backTotal;

  if (backDiff > 0.4f) {
    score -= 10;
  } else if (backDiff > 0.25f) {
    score -= 5;
  }

  sensorData.asymmetry_score = constrain(score, 0.0f, 100.0f);
}

void calculateSedentaryScore() {
  float score = 100.0;

  // Determine if user is sitting based on seat pressure
  float seatTotal = sensorData.fsr_seat_front + sensorData.fsr_seat_back;
  bool sittingNow = seatTotal > 80; // simple threshold, adjust for your chair

  unsigned long now = millis();

  if (sittingNow && !isCurrentlySitting) {
    // Just sat down
    sittingStartTime = now;
    isCurrentlySitting = true;
  } else if (!sittingNow && isCurrentlySitting) {
    // Just stood up → reset sedentary timers
    isCurrentlySitting = false;
    sittingStartTime = now;
    lastMovementTime = now;
  }

  // For demo accuracy, treat "still sitting" as sedentary without depending on IMU motion
  unsigned long sittingDuration   = isCurrentlySitting ? (now - sittingStartTime) / 1000 : 0; // seconds
  // unsigned long timeSinceMovement = (now - lastMovementTime) / 1000;

  // If not currently sitting, sedentary is perfect
  if (!isCurrentlySitting) {
    sensorData.sedentary_score = 100.0f;
    return;
  }

  // While sitting but for a short time, keep sedentary high so it doesn't drop
  if (sittingDuration <= 60) { // first 60 seconds: full score
    sensorData.sedentary_score = 100.0f;
    return;
  }

  // After 1–2 minutes, gently reduce sedentary score for demo
  if (sittingDuration > 180) {            // > 3 minutes
    score -= 30;
    statusMessage = "Sitting >3 min (demo) - Please take a break now";
  } else if (sittingDuration > 120) {     // > 2 minutes
    score -= 20;
    statusMessage = "Sitting >2 min (demo) - Consider a short break";
  }

  sensorData.sedentary_score = constrain(score, 0.0f, 100.0f);
}

void calculateErgoStressScore() {
  float baseScore = 100.0;

  // Weighted penalties from individual scores
  float posturePenalty   = (100.0f - sensorData.posture_score)   * 0.5f;
  float asymmetryPenalty = (100.0f - sensorData.asymmetry_score) * 0.2f;
  float sedentaryPenalty = (100.0f - sensorData.sedentary_score) * 0.3f;

  sensorData.ergo_stress_score = baseScore - posturePenalty - asymmetryPenalty - sedentaryPenalty;
  sensorData.ergo_stress_score = constrain(sensorData.ergo_stress_score, 0.0f, 100.0f);

  sensorData.timestamp = millis();
}

// ==================== INTERVENTION FUNCTIONS ====================
void updateIntervention() {
  unsigned long currentTime = millis();

  // Determine intervention level based on score
  if (sensorData.ergo_stress_score < 50.0f) {
    currentIntervention = INTERVENTION_HIGH;
  } else if (sensorData.ergo_stress_score < 70.0f) {
    currentIntervention = INTERVENTION_MEDIUM;
  } else {
    currentIntervention = INTERVENTION_LOW;
  }

  // If posture is bad for a while, blink RED LED for demo clarity (NOT tied to cooldown)
  const unsigned long BAD_POSTURE_BLINK_DELAY = 8000; // 8 seconds of bad posture before blinking
  const unsigned long BLINK_PERIOD_MS = 300;          // visible blink rate
  bool isBad = (currentPostureState == POSTURE_CURVED || currentPostureState == POSTURE_SLOUCH);
  bool longBad = isBad && (currentTime - badPostureStartTime > BAD_POSTURE_BLINK_DELAY);

  if (longBad) {
    // Blink red continuously while long-bad posture is present
    if (currentTime - lastBlinkToggleTime >= BLINK_PERIOD_MS) {
      redBlinkState = !redBlinkState;
      lastBlinkToggleTime = currentTime;
    }
    setStatusLED(false, redBlinkState);
    setVibration(true);
    statusMessage = "Bad posture sustained - blinking red!";
    return; // skip normal intervention logic while blinking
  } else {
    // reset blink state when posture recovers
    redBlinkState = false;
    lastBlinkToggleTime = currentTime;
  }

  // Apply normal interventions (with cooldown)
  if (currentTime - lastInterventionTime > INTERVENTION_COOLDOWN) {
    switch (currentIntervention) {
      case INTERVENTION_HIGH:
        // RED on, GREEN off, vibration ON
        setStatusLED(false, true);
        setVibration(true);
        Serial.println("ALERT: Critical - Stand up immediately!");
        break;

      case INTERVENTION_MEDIUM:
        // Both LEDs on (yellow-ish), vibration short pulse
        setStatusLED(true, true);
        setVibration(true);
        delay(300);
        setVibration(false);
        Serial.println("Warning: Adjust posture - Take micro-break");
        break;

      case INTERVENTION_LOW:
      default:
        // GREEN on, RED off, no vibration
        setStatusLED(true, false);
        setVibration(false);
        break;
    }

    lastInterventionTime = currentTime;
  }
}

// ==================== COMMUNICATION FUNCTIONS ====================
void sendDataToServer() {
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  String jsonPayload = "{";
  jsonPayload += "\"fsr_seat_front_left\":" + String(sensorData.fsr_seat_front_left) + ",";
  jsonPayload += "\"fsr_seat_front_right\":" + String(sensorData.fsr_seat_front_right) + ",";
  jsonPayload += "\"fsr_seat_front\":" + String(sensorData.fsr_seat_front) + ",";
  jsonPayload += "\"fsr_seat_back\":"  + String(sensorData.fsr_seat_back)  + ",";
  jsonPayload += "\"fsr_back_upper\":" + String(sensorData.fsr_back_upper) + ",";
  jsonPayload += "\"fsr_back_lower\":" + String(sensorData.fsr_back_lower) + ",";
  // Percent of personal max (learned during calibration)
  jsonPayload += "\"fsr_seat_front_left_pct\":" + String(toPct(sensorData.fsr_seat_front_left, max_pressure[0])) + ",";
  jsonPayload += "\"fsr_seat_front_right_pct\":" + String(toPct(sensorData.fsr_seat_front_right, max_pressure[1])) + ",";
  jsonPayload += "\"fsr_seat_back_pct\":" + String(toPct(sensorData.fsr_seat_back, max_pressure[2])) + ",";
  jsonPayload += "\"fsr_back_upper_pct\":" + String(toPct(sensorData.fsr_back_upper, max_pressure[3])) + ",";
  jsonPayload += "\"fsr_back_lower_pct\":" + String(toPct(sensorData.fsr_back_lower, max_pressure[4])) + ",";
  // expose max values for debugging
  jsonPayload += "\"fsr_max_front_left\":" + String(max_pressure[0]) + ",";
  jsonPayload += "\"fsr_max_front_right\":" + String(max_pressure[1]) + ",";
  jsonPayload += "\"fsr_max_seat_back\":" + String(max_pressure[2]) + ",";
  jsonPayload += "\"fsr_max_back_upper\":" + String(max_pressure[3]) + ",";
  jsonPayload += "\"fsr_max_back_lower\":" + String(max_pressure[4]) + ",";
  jsonPayload += "\"pitch\":"          + String(sensorData.pitch)          + ",";
  jsonPayload += "\"roll\":"           + String(sensorData.roll)           + ",";
  jsonPayload += "\"posture_score\":"  + String(sensorData.posture_score)  + ",";
  jsonPayload += "\"asymmetry_score\":"+ String(sensorData.asymmetry_score)+ ",";
  jsonPayload += "\"sedentary_score\":"+ String(sensorData.sedentary_score)+ ",";
  jsonPayload += "\"ergo_stress_score\":"+ String(sensorData.ergo_stress_score) + ",";
  // extra fields for dashboard UX
  jsonPayload += "\"is_sitting\":"     + String(isCurrentlySitting ? 1 : 0) + ",";
  jsonPayload += "\"sitting_duration\":"+ String(isCurrentlySitting ? ((millis() - sittingStartTime) / 1000) : 0) + ",";
  jsonPayload += "\"status_message\":\"" + statusMessage + "\",";
  jsonPayload += "\"timestamp\":"      + String(sensorData.timestamp);
  jsonPayload += "}";

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

// ==================== DEBUG FUNCTIONS ====================
void printDebugInfo() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) { // Print every second
    Serial.println("=== Sensor Data (4-FSR) ===");
    Serial.print("Front L: "); Serial.print(sensorData.fsr_seat_front_left);
    Serial.print(" | Front R: "); Serial.print(sensorData.fsr_seat_front_right);
    Serial.print(" | Seat Front: "); Serial.print(sensorData.fsr_seat_front);
    Serial.print(" | Seat Back: "); Serial.println(sensorData.fsr_seat_back);
    Serial.print("Back Upper: "); Serial.print(sensorData.fsr_back_upper);
    Serial.print(" | Back Lower: "); Serial.println(sensorData.fsr_back_lower);
    Serial.print("Pitch: "); Serial.print(sensorData.pitch);
    Serial.print(" | Roll: "); Serial.println(sensorData.roll);
    Serial.print("Posture: "); Serial.print(sensorData.posture_score);
    Serial.print(" | Asymmetry: "); Serial.print(sensorData.asymmetry_score);
    Serial.print(" | Sedentary: "); Serial.println(sensorData.sedentary_score);
    Serial.print("Ergo-Stress Score: "); Serial.println(sensorData.ergo_stress_score);
    Serial.println("===========================");
    lastPrint = millis();
  }
}

