<<<<<<< HEAD
## ErgoGuard Chair (4‑FSR Version) 🪑
### Context-Aware Posture + Sedentary Risk Chair for IT Workers

ErgoGuard Chair is a smart chair system that uses **4 pressure sensors**, an **IMU (MPU6050)**, **2 vibration motors**, and **red/green LEDs** to:

- **Detect posture quality** (upright vs slouching)
- **Detect leaning / weight imbalance** (front vs back, upper vs lower back)
- **Monitor sedentary behavior** (how long you sit without micro-movements)
- **Intervene in real time** (vibration + LED feedback)
- **Show live analytics** on a web dashboard

This README is your **A–Z build guide**: hardware, wiring, firmware, backend, frontend, and demo.

---

## 🧱 Hardware Overview (What you actually have)

- **Controller**
  - ESP32 DevKit (ESP32‑WROOM)

- **Sensors**
  - 4x FSR (Force Sensitive Resistors)
    - Seat Front
    - Seat Back
    - Backrest Upper
    - Backrest Lower
  - 1x MPU6050 (6‑axis IMU: accelerometer + gyroscope)
  - 1x 16‑channel analog MUX (e.g., CD74HC4067) for the FSRs

- **Actuators**
  - 2x coin‑type vibration motors (seat + back)
  - 1x Red LED (alert)
  - 1x Green LED (good posture)

- **Passive components**
  - 4x 10kΩ resistors (FSR pull‑downs)
  - Several 330Ω resistors (LED and transistor base resistors)
  - 2x 2N2222 NPN transistors (for motors)
  - Jumper wires, breadboard(s)

- **Power**
  - 5V via ESP32 USB (from PC or 5V adapter)

---

## 🔌 Wiring Summary

**High‑level:**

- All 4 FSRs → 16‑channel MUX → ESP32 **GPIO 34** (single analog in)
- MUX channel select (S0–S3) from ESP32 digital GPIOs
- MPU6050 via I²C (GPIO 21, 22)
- 2 vibration motors driven via 2N2222 NPN transistors (controlled by GPIO 25 & 26)
- Red/Green LEDs on GPIO 14 & 27 with 330Ω resistors
- **All grounds common**

**Pin mapping (matches firmware):**

- `MUX_SIG` → GPIO **34**
- `MUX_S0` → GPIO **23**
- `MUX_S1` → GPIO **19**
- `MUX_S2` → GPIO **18**
- `MUX_S3` → GPIO **5**

- `LED_GREEN` → GPIO **27**
- `LED_RED` → GPIO **14**

- `VIB_MOTOR_1` (seat) → GPIO **25**
- `VIB_MOTOR_2` (back) → GPIO **26**

- `SDA` → GPIO **21**
- `SCL` → GPIO **22**

For a detailed ASCII diagram and FSR/MUX connection pattern, see `WIRING_DIAGRAM.md`.

---

## 🧠 Firmware (ESP32)

Firmware file: `firmware/ergoguard_chair/ergoguard_chair.ino`

### What the firmware does

- Reads 4 FSR values via the MUX
- Reads IMU values (pitch/roll + movement)
- Runs a **1‑minute auto‑calibration** when powered on
- Calculates:
  - **Posture Score** (based on pitch and backrest pressure)
  - **Asymmetry Score** (front vs back of seat, upper vs lower back)
  - **Sedentary Score** (time since last movement, sitting duration)
  - **Ergo‑Stress Score** (weighted combination)
- Drives:
  - **Green/Red LEDs** (good vs bad posture)
  - **Motors** (vibration for warnings/critical scores)
- Sends JSON data via HTTP POST to the backend at `/api/sensor-data`

### One‑time setup steps

1. **Install tools**
   - Install Arduino IDE
   - Install ESP32 board support (Boards Manager → ESP32 by Espressif)

2. **Libraries to install (Arduino Library Manager)**
   - `MPU6050` (Adafruit/compatible, the one used in the sketch)
   - `WiFi` (built‑in with ESP32 core)
   - `HTTPClient` (built‑in with ESP32 core)

3. **Configure WiFi + backend URL**

   In `ergoguard_chair.ino`:

  
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* serverURL = "http://YOUR_SERVER_IP:3000/api/sensor-data";
      - Replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD`
   - Replace `YOUR_SERVER_IP` with the machine running the Node backend
     - On the same PC: often `http://192.168.x.x:3000/api/sensor-data`

4. **Compile and upload**
   - Board: **ESP32 Dev Module**
   - Port: your ESP32 COM port
   - Upload the sketch
   - Open Serial Monitor @ 115200 baud → you should see:
     - WiFi connection logs
     - Calibration messages
     - Live scores and FSR readings

---

## 🌐 Backend (Node.js + MongoDB)

Folder: `backend/`

### What it does

- Receives POSTs from ESP32 at `/api/sensor-data`
- Stores readings in MongoDB
- Provides REST APIs for latest data and analytics
- Pushes real‑time updates to the frontend via WebSocket

### Setup

1. **Install dependencies**

  
   cd backend
   npm install
   2. **Configure MongoDB**

   - Create a `.env` file inside `backend/`:

    
     MONGODB_URI=mongodb://localhost:27017/ergoguard
        - Or point to any MongoDB instance you have.

3. **Run the backend**

  
   npm start
      - Server runs on `http://localhost:3000`
   - WebSocket also uses port 3000

### Important data model

In `server.js` the **sensor reading schema** is:

const SensorReadingSchema = new mongoose.Schema({
  deviceId: { type: String, default: 'chair-001' },
  // 4-FSR configuration
  fsr_seat_front: Number,
  fsr_seat_back: Number,
  fsr_back_upper: Number,
  fsr_back_lower: Number,
  // IMU
  pitch: Number,
  roll: Number,
  // Scores
  posture_score: Number,
  asymmetry_score: Number,
  sedentary_score: Number,
  ergo_stress_score: Number,
  timestamp: Number,
  createdAt: { type: Date, default: Date.now }
});APIs you use:

- `POST /api/sensor-data` – ESP32 sends readings here
- `GET /api/latest` – frontend gets latest reading
- `GET /api/analytics?hours=24` – stats for dashboard

---

## 💻 Frontend (React Dashboard)

Folder: `frontend/`

### What it shows

- **PostureVisualizer**: visual heatmap using 4 FSRs
  - Seat Front/Back
  - Back Upper/Lower
- **ScoreDashboard**: current Ergo‑Stress + component scores
- **AlertPanel**: traffic‑light style alerts + recommendations
- **AnalyticsPanel**: 24‑hour stats (average score, interventions, posture breakdown)

### Setup

1. **Install dependencies**

  
   cd frontend
   npm install
   2. **Run development server**

  
   npm start
      - Opens at `http://localhost:3001` or similar (depending on your setup)
   - It calls the backend at `http://localhost:3000` by default (see `App.js` if you want to change).

3. **Real‑time updates**

   - A WebSocket connects to `ws://localhost:3000`
   - When backend receives new `/api/sensor-data`, it broadcasts a `sensor_update` event
   - React state updates live (no manual refresh needed)

---

## 🔁 End‑to‑End Flow

1. You power the ESP32 → it connects to WiFi → **1‑minute calibration**
2. You sit normally during calibration → firmware stores baseline FSR + pitch/roll
3. After calibration:
   - Every 100 ms:
     - Read FSRs via MUX
     - Read IMU
     - Compute scores
   - Every loop:
     - Adjust LEDs + motors based on **Ergo‑Stress Score**
     - Send JSON to backend (if WiFi OK)
4. Backend stores data and sends it via WebSocket
5. Frontend React dashboard shows:
   - Live posture visualization
   - Live scores and alerts
   - 24‑hour summary analytics

---

## 🎬 Demo Script (Judges / Presentation)

1. **Good posture (30s)**
   - Sit upright, back fully touching backrest
   - Green LED ON, vibrations OFF
   - Ergo‑Stress ~80–100 on dashboard

2. **Slouch forward (30s)**
   - Move shoulders forward; reduce back FSR reading
   - Posture score drops, Ergo‑Stress falls <70
   - Red LED/“Warning” on dashboard, vibration pulses

3. **Lean pattern (30s)**
   - Move weight to seat front or back and change back upper/lower pressure
   - Asymmetry score drops
   - Alert tells you to re‑balance

4. **Sedentary demo**
   - Sit still for a while (or simulate by adjusting timers)
   - Sedentary score drops, alerts suggest standing/micro‑breaks

---

## 🐛 Troubleshooting (Common Issues)

- **ESP32 not detected in Arduino IDE**
  - Try a different USB cable
  - Install CP210x/CH340 drivers depending on your board

- **No WiFi connection**
  - Check SSID/password
  - Ensure 2.4 GHz network, not 5 GHz only

- **FSR reads constant 0 or 4095**
  - Check FSR orientation (one side 3.3V, other side MUX+resistor to GND)
  - Confirm MUX powered at 3.3V and SIG connected to GPIO34

- **No movement readings**
  - Check MPU6050 wiring (3.3V, GND, SDA=21, SCL=22)
  - Verify library initialization in Serial Monitor

- **Dashboard empty**
  - Ensure backend (`npm start` in `backend/`) is running
  - Check that ESP32 `serverURL` matches backend IP/port
  - Open browser dev tools → Network/WebSocket tab for errors

---

## 📄 License & Notes

- This build is **for educational / prototype use**.
- If you plan to commercialize, use `PATENT_DOCUMENTATION.md` as a starting point for novelty and claims.

**Built with ❤️ to protect your spine while you code.**
=======
# ERGO-GUARD-CHAIR
ErgoGuard Chair is an AI and IoT-enabled smart ergonomic chair that monitors posture, sitting duration, and environmental conditions using intelligent sensors. It provides real-time posture correction through vibration alerts and displays health insights on a web dashboard, helping reduce fatigue, improve comfort, and promote healthier living.
>>>>>>> cb20d8cf073afa97c2da48f00c26c9399389c1b0
