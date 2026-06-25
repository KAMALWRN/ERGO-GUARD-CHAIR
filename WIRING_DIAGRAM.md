# ErgoGuard Chair - Wiring Diagram (4 FSR + MUX Version)

## 🔌 Complete Circuit Diagram

                 ┌─────────────────────┐
                 │   ESP32 DEV BOARD   │
                 │                     │
                 │  GPIO 34 (ADC) ─────┼──► MUX SIG (common analog out)
                 │                     │
                 │  GPIO 23 ───────────┼──► MUX S0
                 │  GPIO 19 ───────────┼──► MUX S1
                 │  GPIO 18 ───────────┼──► MUX S2
                 │  GPIO 5  ───────────┼──► MUX S3
                 │                     │
                 │  GPIO 21 (SDA) ─────┼──► MPU6050 SDA
                 │  GPIO 22 (SCL) ─────┼──► MPU6050 SCL
                 │                     │
                 │  GPIO 25 ───────────┼──► Base (2N2222 #1) via 330Ω (Motor 1 - seat)
                 │  GPIO 26 ───────────┼──► Base (2N2222 #2) via 330Ω (Motor 2 - back)
                 │                     │
                 │  GPIO 27 ───────────┼──► Green LED anode via 330Ω
                 │  GPIO 14 ───────────┼──► Red LED anode via 330Ω
                 │                     │
                 │  3.3V ──────────────┼──► MUX VCC, MPU6050 VCC, FSR high side
                 │  5V/VIN ────────────┼──► Vibration motors +
                 │  GND ───────────────┼──► MUX GND, MPU6050 GND,
                 │                        FSR resistor GND, LED cathodes,
                 │                        2N2222 emitters
                 └─────────────────────┘
                              │
                              │ USB Power
                              │
                    ┌─────────▼─────────┐
                    │  5V 2A USB Adapter │
                    └───────────────────┘---

## 🌀 FSR + MUX Wiring (4 Sensors)

Each FSR is a voltage divider with a **10kΩ resistor** to GND, and all four go into the MUX.

**FSR connection (for each of the 4 sensors):**

- FSR pin 1 → **3.3V**
- FSR pin 2 → **MUX CHx** **and** → **10kΩ resistor → GND**

**MUX channel mapping:**

- **CH0** → Seat Front FSR  
- **CH1** → Seat Back FSR  
- **CH2** → Backrest Upper FSR  
- **CH3** → Backrest Lower FSR  

MUX power and control lines:

- MUX VCC → **3.3V**
- MUX GND → **GND**
- MUX SIG → **GPIO 34**
- MUX S0 → **GPIO 23**
- MUX S1 → **GPIO 19**
- MUX S2 → **GPIO 18**
- MUX S3 → **GPIO 5**

---

## ⚙️ Vibration Motor + 2N2222 Wiring (2 Motors)

For each motor (Motor 1 = seat, Motor 2 = backrest):

- ESP32 GPIO (25 or 26) → **330Ω** → 2N2222 **base**
- 2N2222 **emitter** → **GND**
- 2N2222 **collector** → motor negative (–)
- Motor positive (+) → **5V/VIN**

So the current path is: **5V → Motor → 2N2222 → GND**.

*Optional (recommended if you have diodes):*  
Add a **1N4007** across each motor, cathode to +5V, anode to transistor collector, for flyback protection.

---

## 💡 Red / Green LED Wiring (Posture Status)

- **Green LED**
  - GPIO 27 → 330Ω → LED anode (+)
  - LED cathode (–) → GND

- **Red LED**
  - GPIO 14 → 330Ω → LED anode (+)
  - LED cathode (–) → GND

Behavior:

- Calibration → both ON  
- Good posture → Green ON, Red OFF  
- Warning / Critical → Red ON (and vibration), Green OFF  

---

## 📡 MPU6050 I2C Wiring

- VCC → **3.3V**
- GND → **GND**
- SDA → **GPIO 21**
- SCL → **GPIO 22**

*(Optional: add 4.7kΩ pull‑ups from SDA/SCL to 3.3V if you have them.)*

---

## 📍 Physical Layout on Chair (4 FSR)

                 BACKREST (side view)
        ┌─────────────────────────────┐
        │      [Back Upper FSR]      │  (between shoulder blades)
        │             │              │
        │          [MPU6050]         │  (center top)
        │             │              │
        │      [Back Lower FSR]      │  (lumbar)
        │      [Vib Motor 2]         │
        │ [Red / Green LEDs on top]  │
        └─────────────────────────────┘
                    │
                    │
        ┌───────────┴───────────┐
        │         SEAT          │
        │  [Seat Back FSR]      │  (under back of thighs)
        │  [Seat Front FSR]     │  (under front of thighs)
        │      [Vib Motor 1]    │  (center of seat)
        └───────────────────────┘---

## 🧰 Control Box & Power

- Mount the ESP32, MUX, and wiring on a small breadboard or in a small box attached to the chair frame.
- Power ESP32 via USB (from PC or a 5V wall adapter).
- Use ESP32 **5V/VIN** pin to feed the **motors**.
- Use ESP32 **3.3V** pin to feed:
  - MUX VCC
  - MPU6050 VCC
  - High side of all 4 FSRs

**All grounds must be common:**

- ESP32 GND  
- MUX GND  
- FSR resistor GND ends  
- MPU6050 GND  
- 2N2222 emitters  
- Motor ground side  
- LED cathodes  

---

## ✅ Testing Checklist (4 FSR Build)

- [ ] With no one sitting, all 4 FSR readings are low (near zero).  
- [ ] Press each area (seat front, seat back, back upper, back lower) → only that sensor’s reading increases.  
- [ ] MPU6050 returns changing pitch/roll when you tilt the chair.  
- [ ] Vibration motors spin when the firmware triggers high/medium intervention.  
- [ ] Red/Green LEDs change as posture/Ergo‑Stress score changes.  
- [ ] ESP32 connects to WiFi and the backend receives `/api/sensor-data` from the firmware.  