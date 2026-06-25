// SMART LANE-BASED TRAFFIC LIGHT SYSTEM
// Board: Arduino Uno
//
// 4-way junction, 2 IR sensors per lane (near + back).
// Lane with highest density (0-2) gets priority green.
// If no traffic, system cycles round-robin.

// IR sensor pins (active LOW = vehicle present)
const int IR_PINS[4][2] = {
  {2, 3},  // Lane 1: {near, back}
  {4, 5},  // Lane 2
  {6, 7},  // Lane 3
  {8, 9}   // Lane 4
};

// Traffic light pins: {RED, YELLOW, GREEN}
const int TL_PINS[4][3] = {
  {10, 11, 12}, // Lane 1
  {13, A0, A1}, // Lane 2
  {A2, A3, A4}, // Lane 3
  {A5, A6, A7}  // Lane 4 (adjust if board lacks A6/A7)
};

const unsigned long MIN_GREEN_TIME = 5000; // 5 seconds
const unsigned long CHECK_INTERVAL = 1000; // re-evaluate every 1s after min

int currentLane = 0; // 0..3
unsigned long lastSwitchTime = 0;

void setup() {
  Serial.begin(9600);

  // IR sensors as inputs with pull-ups
  for (int lane = 0; lane < 4; lane++) {
    pinMode(IR_PINS[lane][0], INPUT_PULLUP);
    pinMode(IR_PINS[lane][1], INPUT_PULLUP);
  }

  // Traffic light pins as outputs
  for (int lane = 0; lane < 4; lane++) {
    for (int c = 0; c < 3; c++) {
      pinMode(TL_PINS[lane][c], OUTPUT);
    }
  }

  setAllRed();
  setLaneGreen(currentLane);
  lastSwitchTime = millis();
}

void loop() {
  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  // Hold minimum green time before considering change
  if (now - lastSwitchTime < MIN_GREEN_TIME) {
    return;
  }

  if (now - lastCheck >= CHECK_INTERVAL) {
    lastCheck = now;

    int densities[4];
    readDensities(densities);

    int bestLane = chooseBestLane(densities);

    Serial.print("Densities: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(densities[i]);
      Serial.print(" ");
    }
    Serial.print(" | Current: ");
    Serial.print(currentLane + 1);
    Serial.print(" Best: ");
    Serial.println(bestLane + 1);

    // If no traffic anywhere, simple round-robin
    bool anyTraffic = false;
    for (int i = 0; i < 4; i++) if (densities[i] > 0) anyTraffic = true;

    if (!anyTraffic) {
      currentLane = (currentLane + 1) % 4;
      setAllRed();
      setLaneGreen(currentLane);
      lastSwitchTime = now;
    } else {
      // Change if current has 0 density or another lane is strictly better
      if (densities[currentLane] == 0 || bestLane != currentLane) {
        currentLane = bestLane;
        setAllRed();
        setLaneGreen(currentLane);
        lastSwitchTime = now;
      }
    }
  }
}

// -------- Logic helpers --------

void readDensities(int densities[4]) {
  for (int lane = 0; lane < 4; lane++) {
    int nearState = digitalRead(IR_PINS[lane][0]);
    int backState = digitalRead(IR_PINS[lane][1]);

    bool nearVeh = (nearState == LOW); // active LOW
    bool backVeh = (backState == LOW);

    int score = 0;
    if (nearVeh) score++;
    if (backVeh) score++;
    densities[lane] = score; // 0,1,2
  }
}

int chooseBestLane(int densities[4]) {
  int bestLane = 0;
  int bestScore = densities[0];

  for (int lane = 1; lane < 4; lane++) {
    if (densities[lane] > bestScore) {
      bestScore = densities[lane];
      bestLane = lane;
    }
  }
  return bestLane; // if tie, keep first
}

// -------- Output helpers --------

void setAllRed() {
  for (int lane = 0; lane < 4; lane++) {
    digitalWrite(TL_PINS[lane][0], HIGH); // RED
    digitalWrite(TL_PINS[lane][1], LOW);  // YELLOW
    digitalWrite(TL_PINS[lane][2], LOW);  // GREEN
  }
}

void setLaneGreen(int lane) {
  for (int i = 0; i < 4; i++) {
    if (i == lane) {
      digitalWrite(TL_PINS[i][0], LOW);   // RED off
      digitalWrite(TL_PINS[i][1], LOW);   // YELLOW off
      digitalWrite(TL_PINS[i][2], HIGH);  // GREEN on
    } else {
      digitalWrite(TL_PINS[i][0], HIGH);  // RED on
      digitalWrite(TL_PINS[i][1], LOW);
      digitalWrite(TL_PINS[i][2], LOW);
    }
  }
}


