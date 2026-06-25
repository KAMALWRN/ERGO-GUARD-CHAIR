# SafePath – Intelligent Fire Evacuation System

## 1. Project Overview

**SafePath** is an ESP32‑based fire evacuation guidance system that:

- Detects **fire and smoke** in a corridor/hall.
- Chooses the **safest exit** (A or B) based on sensor data.
- Guides people using **green LEDs** for safe route and **red LEDs** for blocked exits.
- Displays clear messages on a **16x2 I2C LCD**.
- Sends **real‑time alerts** to a smartphone using **Blynk IoT**.

The reference layout is a simple hall with **two exits** at opposite ends; the logic can be extended to more complex buildings.

---

## 2. Components List

- **ESP32 DevKit** x1  
- **Flame sensor** x1 (analog or digital output module)  
- **Smoke sensors (MQ2 / similar)** x2  
- **Green LEDs** x2 (one per exit, or LED strips)  
- **Red LEDs** x2 (one per exit)  
- **16x2 LCD with I2C backpack (0x27)** x1  
- **Blynk IoT account** (cloud + mobile app)  
- Breadboard, jumper wires, USB cable  

---

## 3. Pin Mapping & Wiring

### 3.1 Sensors → ESP32

| Sensor           | Location                    | ESP32 Pin |
|------------------|-----------------------------|-----------|
| Flame sensor OUT | Hall / room center          | `GPIO 34` |
| Smoke sensor A   | Corridor near Exit A        | `GPIO 35` |
| Smoke sensor B   | Corridor near Exit B        | `GPIO 32` |

Each sensor module:  
- `VCC` → `3.3V` or `5V` (according to module spec)  
- `GND` → `GND`  
- `OUT` → analog pin above  

### 3.2 LEDs → ESP32

| LED Group            | Color | ESP32 Pin |
|----------------------|-------|-----------|
| Exit A Safe Path     | Green | `GPIO 14` |
| Exit A Blocked       | Red   | `GPIO 27` |
| Exit B Safe Path     | Green | `GPIO 26` |
| Exit B Blocked       | Red   | `GPIO 25` |

Each LED: microcontroller pin → 220Ω resistor → LED anode; LED cathode → `GND`.

### 3.3 LCD (I2C) → ESP32

| LCD Pin | ESP32 Pin |
|---------|-----------|
| SDA     | `GPIO 21` |
| SCL     | `GPIO 22` |
| VCC     | `3.3V` or `5V` (backpack‑dependent) |
| GND     | `GND`     |

---

## 4. Building Layout & Sensor Placement

```text
        [Exit A]
           |
      (SMOKE A)
         SMA
           |
   ===== Corridor A =====
           |
      Flame Sensor (F)
           |
   ===== Corridor B =====
           |
         SMB
      (SMOKE B)
           |
        [Exit B]
```

- **Flame sensor** F watches the hall center.  
- **Smoke A (SMA)** near Exit A’s corridor.  
- **Smoke B (SMB)** near Exit B’s corridor.  
- Green LEDs are placed along the **safe corridor** toward the chosen exit.  
- Red LEDs sit at **blocked exits** or in dangerous corridors.

---

## 5. Safe Path Decision Logic

We model a simple graph:

```text
Hall (H)
 ├─ Corridor A → Exit A
 └─ Corridor B → Exit B
```

Assumptions:

- Distances H→A and H→B are similar (equal weight).  
- **Smoke A > threshold** ⇒ Corridor A / Exit A is dangerous.  
- **Smoke B > threshold** ⇒ Corridor B / Exit B is dangerous.  
- **Flame sensor** adds global fire detection; does not directly pick exits but confirms fire presence.

Algorithm:

1. Read `flameVal`, `smokeAVal`, `smokeBVal` from ADC.  
2. `fireDetected = (flameVal > Tflame) OR (smokeAVal > TA) OR (smokeBVal > TB)`  
3. `exitABlocked = (smokeAVal > TA)`  
   `exitBBlocked = (smokeBVal > TB)`  

4. Decision tree:

   - If `!fireDetected`  
     → Normal state: all exits green, LCD: *"No Fire Detected"*  
   - Else if `exitABlocked && !exitBBlocked`  
     → Use Exit B (shortest safe path).  
   - Else if `!exitABlocked && exitBBlocked`  
     → Use Exit A.  
   - Else if `!exitABlocked && !exitBBlocked`  
     → Fire only near hall; both exits safe. For simplicity, choose Exit A as shortest.  
   - Else (`exitABlocked && exitBBlocked`)  
     → All exits blocked: show warning and turn both exits red.

5. LEDs and LCD:

- **Use Exit A:**  
  - Exit A Green ON, A Red OFF; Exit B Red ON, B Green OFF.  
  - LCD: `"Fire near Exit B"` / `"Use Exit A (GREEN)"`.  
- **Use Exit B:** mirrored behaviour.  
- **All safe:** both exits Green, message `"No Fire Detected"`.  
- **All blocked:** both Reds ON, LCD: `"ALL EXITS BLOCKED"` & `"Wait for rescue"`.  

This effectively chooses the **shortest safe path** among the two exits by **filtering out unsafe paths** using smoke thresholds.

---

## 6. Blynk Configuration

1. **Create Template** in Blynk Console  
   - Template name: `SafePath`  
   - Hardware: `ESP32`  
   - Connection: `WiFi`  
   - Copy `BLYNK_TEMPLATE_ID` and `BLYNK_TEMPLATE_NAME`.  

2. **Create Device** from this template  
   - Copy the **Device Auth Token** (`BLYNK_AUTH_TOKEN`).  

3. **Add Datastreams**

| Datastream | Type   | Pin | Purpose         |
|------------|--------|-----|-----------------|
| `V0`       | String | V0  | Alert message   |
| `V1`       | Int    | V1  | Fire present (0/1) |
| `V2`       | Int    | V2  | Smoke A value   |
| `V3`       | Int    | V3  | Smoke B value   |

4. **Events / Notifications**

- Create an Event named `fire_alert`.  
- Enable push notifications when the event is logged.  

5. **Mobile App Widgets**

- Label/Value display bound to **V0** (alert text).  
- LED or Gauge widget bound to **V1** (fire status).  
- Value widgets for **V2** and **V3** (smoke levels).  
- Notification widget (for push alerts).  

6. **Credentials in Code**

Update in `safepath/firmware/safepath_esp32.ino`:

```cpp
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "SafePath"
#define BLYNK_AUTH_TOKEN    "YOUR_DEVICE_AUTH_TOKEN"

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";
```

---

## 7. Source Code

The complete ESP32 + Blynk sketch is in:

- `safepath/firmware/safepath_esp32.ino`

It is fully commented and ready to compile in the Arduino IDE (after installing **Blynk** and **LiquidCrystal_I2C** libraries).

---

## 8. Step‑by‑Step Operation

1. **Power On & WiFi/Blynk Connect**
   - LCD: `"SafePath System"` → `"Connecting WiFi"` → `"SafePath Ready"` / `"Monitoring..."`.  
   - ESP32 connects to WiFi and Blynk cloud.  

2. **Normal Monitoring State**
   - Reads flame + smoke sensors every 0.5s.  
   - When all values are below thresholds:
     - Both exits show **Green LEDs**.  
     - LCD: `"No Fire Detected"` / `"All exits normal"`.  
     - Blynk V0 shows `"No fire detected"`.  

3. **Fire Detected**
   - If readings exceed thresholds:
     - `fireDetected = true`.  
     - Blynk event `fire_alert` is logged (first time), sending a push notification.  

4. **Exit Evaluation**
   - Using `exitABlocked` and `exitBBlocked` from smoke sensors, SafePath decides:
     - **Use Exit A** (safe, shortest).  
     - **Use Exit B**.  
     - **All exits blocked**.  
   - The corresponding LED pattern and LCD messages are set.  

5. **User Guidance**
   - People in the building:
     - Look at the **LCD message** (`"Use Exit A (GREEN)"` etc.).  
     - Follow the **Green LEDs** along the corridor of the recommended exit.  
     - Avoid exits where only **Red LEDs** are lit.  

6. **Remote Monitoring**
   - Safety officer sees:  
     - Live alert text in **Blynk app**.  
     - Real-time smoke levels for both corridors.  
     - Whether fire is active (V1).  

7. **After Fire Clears**
   - When sensors drop below thresholds:
     - System automatically returns to **Normal** state.  
     - LEDs reset to both exits Green.  
     - LCD/Blynk messages update to `"No Fire Detected"`.  

This folder (`safepath/`) now mirrors the structure of your **ErgoGuard Chair** project, containing a clear README and firmware sketch for direct use in demos or reports.


