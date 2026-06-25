## ErgoGuard Chair – End‑to‑End Implementation (From Zero to Demo)

This file is your **single roadmap**: from buying parts to seeing live data on the web dashboard.  
Follow it in order next week and you will finish the whole project step by step.

---

## 1. What You Are Building

- **Smart chair** with:
  - 8 FSR sensors (seat + backrest)
  - IMU (MPU6050) for tilt/posture
  - MQ135 (air quality) + DHT22 (temperature, humidity)
  - 2 vibration motors + RGB LED strip for feedback
- **ESP32 firmware** sends posture + scores to a **Node.js backend**.
- **Web dashboard (React)** shows:
  - Real‑time posture visualization
  - Ergo‑stress, posture, asymmetry, sedentary scores
  - Alerts & recommendations
  - 24‑hour analytics

Code already exists in this folder; you just need to wire and run it.

---

## 2. Project Folder Structure (Where Each Code File Lives)

You do **not** need to create these files; they already exist. This is just to know where things are:

```text
ErgoGuard-Chair/
├── firmware/
│   └── ergoguard_chair.ino          ← ESP32 firmware (upload to board)
├── backend/
│   ├── server.js                    ← Node.js API + WebSocket + MongoDB
│   └── package.json                 ← Backend dependencies & scripts
├── frontend/
│   ├── package.json                 ← Frontend dependencies & scripts
│   └── src/
│       ├── App.js                   ← Main React dashboard
│       ├── App.css                  ← Global styles
│       └── components/
│           ├── PostureVisualizer.js ← Posture + pressure heatmap
│           ├── ScoreDashboard.js    ← Scores + charts
│           ├── AlertPanel.js        ← Alerts & recommendations
│           └── AnalyticsPanel.js    ← 24‑hour analytics
├── IMPLEMENTATION_GUIDE.md          ← Detailed generic guide
├── WIRING_DIAGRAM.md                ← ASCII wiring & placement diagrams
├── COMPONENTS_LIST.md               ← Shopping list for parts
└── FULL_IMPLEMENTATION_GUIDE.md     ← THIS FILE (end‑to‑end steps)
```

When I say “open a file”, open it from this exact path.

---

## 3. Before You Start Next Week – Things to Install on Your Laptop

Install these **once**:

1. **Arduino IDE** (for ESP32 firmware)
   - Download: `https://www.arduino.cc/en/software`
   - In Arduino IDE:
     - File → Preferences → “Additional Boards Manager URLs”:
       - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
     - Tools → Board → Boards Manager → search “ESP32” → Install

2. **Node.js (LTS)** – for backend & frontend
   - Download: `https://nodejs.org` (LTS version)
   - After install, in PowerShell:
     - `node --version`
     - `npm --version`

3. **MongoDB**
   - Easiest for a student project: **MongoDB Atlas (cloud)**
     - Go to `https://www.mongodb.com/cloud/atlas`
     - Create free account + free cluster
     - Create database user (username + password)
     - Get connection string (starts with `mongodb+srv://...`)
   - If you prefer local install, follow `IMPLEMENTATION_GUIDE.md` (Step 1).

4. **Code editor** (recommended)
   - VS Code: `https://code.visualstudio.com/`

You can stop here and continue next week with the steps below.

---

## 4. Hardware: Components, Wiring, and Physical Placement

### 4.1 Major Components (high‑level list)

See `COMPONENTS_LIST.md` for exact part names and quantities. Main items:

- 1× **ESP32 Dev Board**
- 8× **FSR sensors** (force sensitive resistors)
- 1× **MPU6050** IMU
- 1× **MQ135** gas sensor
- 1× **DHT22** temperature/humidity sensor
- 2× **Vibration motors** (DC coin or cylindrical)
- 1× **WS2812B RGB LED strip**
- Resistors: 10kΩ (for FSR pull‑downs), 1kΩ (transistor base), optional I²C pull‑up
- 2× NPN transistors (e.g., 2N2222) for motors + flyback diodes
- 5V 2A USB power adapter

### 4.2 Exact ESP32 Pin Connections

Full ASCII diagram is in `WIRING_DIAGRAM.md`. The **essential mapping** (matches the firmware code `ergoguard_chair.ino`):

- **FSR sensors (analog inputs)**  
  (each FSR has one side to 3.3V, the other to ESP32 GPIO with 10kΩ to GND)
  - Seat front‑left  → GPIO **36** (`FSR_SEAT_FL`)
  - Seat front‑right → GPIO **39** (`FSR_SEAT_FR`)
  - Seat back‑left   → GPIO **34** (`FSR_SEAT_BL`)
  - Seat back‑right  → GPIO **35** (`FSR_SEAT_BR`)
  - Backrest UL      → GPIO **32** (`FSR_BACK_UL`)
  - Backrest UR      → GPIO **33** (`FSR_BACK_UR`)
  - Backrest LL      → GPIO **25** (`FSR_BACK_LL`)
  - Backrest LR      → GPIO **26** (`FSR_BACK_LR`)

- **IMU (MPU6050 over I²C)**
  - SDA → GPIO **21**
  - SCL → GPIO **22**
  - VCC → 3.3V
  - GND → GND

- **Environment sensors**
  - MQ135 analog out → GPIO **27**, VCC → 5V, GND → GND
  - DHT22 data → GPIO **4**, VCC → 3.3V, GND → GND

- **Actuators**
  - Vibration motor 1:
    - Motor between +5V and transistor collector
    - Transistor emitter to GND
    - ESP32 GPIO **16** → 1kΩ → transistor base
  - Vibration motor 2:
    - Same as above, ESP32 GPIO **17** → 1kΩ → base
  - LED strip:
    - Data in → GPIO **2**
    - VCC → 5V
    - GND → common GND

Make sure **all grounds are common** (ESP32 GND, sensors, motors, LED).

### 4.3 Physical Placement on the Chair

Summarised from `WIRING_DIAGRAM.md`:

- **Seat cushion**
  - Place 4 FSRs at: front‑left (FL), front‑right (FR), back‑left (BL), back‑right (BR) under the foam.
  - Vibration motor 1: center of seat, under cushion.

- **Backrest**
  - 4 FSRs: UL, UR, LL, LR between padding and frame.
  - MPU6050: upper backrest, centered, firmly fixed (so tilt is accurate).
  - Vibration motor 2: lumbar region (lower back), under padding.
  - LED strip: top/side edge of the backrest where it is visible.

- **Control box**
  - Small enclosure attached under the seat or behind the backrest.
  - Holds ESP32, power wiring, and connectors to all sensors/actuators.

Build this **first** (even on cardboard) before worrying about the software.

---

## 5. Firmware (ESP32) – From Code to Board

### 5.1 Where the code is

- File: `firmware/ergoguard_chair.ino`  
  This file already contains:
  - Pin definitions (matching the wiring above)
  - Sensor reading code
  - Posture/asymmetry/sedentary/ergo‑stress score calculations
  - WiFi + HTTP code to send data to the backend at `/api/sensor-data`
  - Vibration + LED control for interventions

### 5.2 What you must edit

Open `firmware/ergoguard_chair.ino` and update **only these lines**:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverURL = "http://YOUR_SERVER_IP:3000/api/sensor-data";
```

- `YOUR_WIFI_SSID` / `YOUR_WIFI_PASSWORD`: your home/college WiFi (2.4 GHz).
- `YOUR_SERVER_IP`:
  - If backend runs **on the same laptop** on same WiFi:
    - In Windows, run `ipconfig` and copy your IPv4 address (e.g., `192.168.1.20`).
    - Use: `http://192.168.1.20:3000/api/sensor-data`.

### 5.3 Uploading the firmware

1. Connect ESP32 to your laptop via USB.
2. Open Arduino IDE → open `ergoguard_chair.ino`.
3. Tools → Board → `ESP32 Dev Module` (or matching board).
4. Tools → Port → select the COM port of the ESP32.
5. Click **Upload**.
6. After upload: Tools → Serial Monitor (115200 baud) to see logs:
   - “ErgoGuard Chair – Initializing…”
   - WiFi connection status
   - Sensor readings and scores

You can fully test all sensors and scores in Serial Monitor **even before** the backend/frontend.

---

## 6. Backend (Node.js + MongoDB + WebSocket)

The backend:
- Receives POST data from ESP32 at `/api/sensor-data`
- Saves readings in MongoDB
- Broadcasts live updates to the dashboard via WebSocket
- Serves analytics data (24‑hour stats, posture breakdown, etc.)

### 6.1 Backend code files

- `backend/server.js` – main API + WebSocket + DB logic
- `backend/package.json` – lists dependencies & scripts

You **do not** need to modify the logic; only the **MongoDB connection** if you use Atlas.

### 6.2 Install backend dependencies

Open PowerShell:

```bash
cd "D:\projects\ErgoGuard-Chair\backend"
npm install
```

This installs: `express`, `mongoose`, `cors`, `ws`, etc.

### 6.3 Configure MongoDB connection

Create a file `backend/.env` with:

```env
MONGODB_URI=mongodb+srv://YOUR_USER:YOUR_PASSWORD@YOUR_CLUSTER.mongodb.net/ergoguard
PORT=3000
```

- Replace `YOUR_USER`, `YOUR_PASSWORD`, `YOUR_CLUSTER` with your Atlas details.
- If using **local MongoDB**, you can instead use:

```env
MONGODB_URI=mongodb://localhost:27017/ergoguard
PORT=3000
```

### 6.4 Start the backend server

In PowerShell:

```bash
cd "D:\projects\ErgoGuard-Chair\backend"
npm start
```

You should see:
- `ErgoGuard Backend Server running on port 3000`
- `WebSocket server ready`
- If MongoDB is okay: `MongoDB connected`

### 6.5 Test that backend is alive

In your browser, open:

- `http://localhost:3000/health`

You should get JSON like:

```json
{ "status": "ok", "timestamp": "2026-02-24T..." }
```

Once this works, the backend is ready to receive data from the ESP32.

---

## 7. Frontend (React Web Dashboard)

The frontend:
- Connects to the backend’s **REST API** at `/api`
- Connects to the backend’s **WebSocket** for live updates
- Renders:
  - `PostureVisualizer`
  - `ScoreDashboard`
  - `AlertPanel`
  - `AnalyticsPanel`

### 7.1 Frontend code files

Main files you will use:

- `frontend/src/App.js` – sets up WebSocket + REST calls and **connects all components**
- `frontend/src/components/PostureVisualizer.js`
- `frontend/src/components/ScoreDashboard.js`
- `frontend/src/components/AlertPanel.js`
- `frontend/src/components/AnalyticsPanel.js`

These files are already implemented in this project (you don’t need to rewrite them).

`App.js` already points to the backend on your own machine:

```js
const API_BASE_URL = 'http://localhost:3000/api';
const WS_URL = 'ws://localhost:3000';
```

If the backend runs on **another machine**, change `localhost` to that machine’s IP.

### 7.2 Install frontend dependencies

In a **new** PowerShell window:

```bash
cd "D:\projects\ErgoGuard-Chair\frontend"
npm install
```

This installs: `react`, `axios`, `chart.js`, `react-chartjs-2`, etc.

### 7.3 Start the React dev server

Because the backend is already using port 3000, run React on **port 3001**:

```bash
cd "D:\projects\ErgoGuard-Chair\frontend"
set PORT=3001 && npm start
```

This will open the dashboard at:

- `http://localhost:3001`

You’ll see:
- Title: “ErgoGuard Chair Dashboard”
- Connection indicator (Connected / Disconnected)
- Empty panels waiting for data (if backend / ESP32 not yet sending).

---

## 8. How Everything Connects (Data Flow)

1. **ESP32 firmware** (`ergoguard_chair.ino`)
   - Reads all sensors, computes scores.
   - Every loop, sends JSON to:
     - `http://YOUR_SERVER_IP:3000/api/sensor-data`

2. **Backend** (`backend/server.js`)
   - `POST /api/sensor-data`:
     - Saves the reading into MongoDB.
     - Broadcasts a WebSocket message to all connected dashboards:
       - `{ type: "sensor_update", data: <sensorData> }`
   - `GET /api/latest`:
     - Returns the latest reading from the database.
   - `GET /api/analytics?hours=24`:
     - Returns statistics for the last 24 hours.

3. **Frontend** (`frontend/src/App.js`)
   - On page load:
     - Calls `GET /api/latest` → `sensorData` state.
     - Calls `GET /api/analytics?hours=24` → `analytics` state.
     - Opens WebSocket to `ws://localhost:3000`:
       - On `sensor_update`, updates `sensorData` in real time.
   - Renders child components with these props:
     - `<PostureVisualizer data={sensorData} />`
     - `<AlertPanel data={sensorData} />`
     - `<ScoreDashboard data={sensorData} analytics={analytics} />`
     - `<AnalyticsPanel analytics={analytics} />`

All components are already **connected** through `App.js`. You just need to run the three layers:
- Firmware (ESP32)
- Backend (Node.js)
- Frontend (React)

---

## 9. Step‑By‑Step Order to Follow Next Week

Use this as your **checklist**:

1. **Day 1–2: Hardware**
   - Buy components (see `COMPONENTS_LIST.md`).
   - Wire ESP32 + sensors + actuators (follow `WIRING_DIAGRAM.md` and section 4 above).
   - Mount roughly on chair (or cardboard prototype).

2. **Day 3: Firmware**
   - Install Arduino IDE + ESP32 support.
   - Edit WiFi + `serverURL` in `ergoguard_chair.ino`.
   - Upload to ESP32, open Serial Monitor, verify:
     - Sensors output numbers.
     - Scores are printed.
     - WiFi connects successfully.

3. **Day 4: Backend**
   - Install Node.js (if not already done).
   - In `backend/`: `npm install`.
   - Create `.env` with `MONGODB_URI` + `PORT=3000`.
   - Run `npm start`, verify `http://localhost:3000/health`.

4. **Day 5: Frontend**
   - In `frontend/`: `npm install`.
   - Run `set PORT=3001 && npm start`.
   - Open `http://localhost:3001` and ensure the dashboard loads.

5. **Day 6: End‑to‑End Test**
   - Turn on ESP32 and confirm:
     - Serial Monitor shows “HTTP Response code: 200”.
   - On backend console:
     - You see “ErgoGuard Backend Server running…” and log msgs when data arrives.
   - On dashboard (`localhost:3001`):
     - Posture visualization, scores, alerts, analytics update in real time.

6. **Day 7: Demo Rehearsal**
   - Practice the demo flow from `README.md` and `IMPLEMENTATION_GUIDE.md`:
     - Good posture → green scores, calm LED.
     - Slouch/lean → scores drop, alerts + LED color change + vibration.
     - Show analytics page.

---

## 10. If You Get Stuck

Check in this order:

1. **Firmware**
   - Serial Monitor: is WiFi connected? Do you see “HTTP Response code: 200”?
2. **Backend**
   - `http://localhost:3000/health` works?
   - Any errors printed in the terminal (MongoDB, port in use, etc.)?
3. **Frontend**
   - Browser console (F12 → Console) for any errors.
   - WebSocket connection: open Network → WS tab, see if it connects to `ws://localhost:3000`.
4. **Database**
   - Check MongoDB Atlas / local Mongo to see if `SensorReading` documents are being created.

For more detailed troubleshooting, you can also check the **Troubleshooting** section in `IMPLEMENTATION_GUIDE.md`.

---

**You now have a single, complete file to follow from first connection to final web dashboard.**  
Next week, just open `FULL_IMPLEMENTATION_GUIDE.md` and go through the sections in order.


