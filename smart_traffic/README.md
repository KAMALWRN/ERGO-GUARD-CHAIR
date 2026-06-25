# Smart Lane-Based Traffic Light System

## 1. Project Overview

This project implements a **smart 4‑way traffic light controller** on an Arduino Uno.  
Each lane has **two IR vehicle sensors**, and the system:

- Measures **vehicle density** per lane (0, 1, or 2 cars detected).
- Gives **priority green** to the lane with the highest density.
- **Skips** lanes that have no vehicles waiting.
- After a minimum green time, **re-evaluates** all lanes and switches green to the next most congested lane.

This design is ideal for **lab prototypes**, **student demos**, and basic **IoT/embedded presentations**.

---

## 2. Components List

- **Arduino Uno** x1  
- **IR Sensors** x8 (digital reflective / obstacle sensors, active LOW)  
- **Traffic Light Modules** x4 (each with RED, YELLOW, GREEN, GND)  
- Breadboard or custom PCB  
- Jumper wires  
- USB cable / 5V power supply  

---

## 3. Pin Mapping & Wiring

### 3.1 IR Sensors → Arduino

| Lane | Sensor | Function              | Arduino Pin |
|------|--------|-----------------------|-------------|
| 1    | IR1    | Near stop line        | `D2`        |
| 1    | IR2    | Further back in lane  | `D3`        |
| 2    | IR3    | Near stop line        | `D4`        |
| 2    | IR4    | Further back in lane  | `D5`        |
| 3    | IR5    | Near stop line        | `D6`        |
| 3    | IR6    | Further back in lane  | `D7`        |
| 4    | IR7    | Near stop line        | `D8`        |
| 4    | IR8    | Further back in lane  | `D9`        |

All IR sensors:  
- `VCC` → `5V`  
- `GND` → `GND`  
- `OUT` → corresponding Arduino digital pin above  

> **Note**: Most IR modules are **active LOW** (output LOW when object is detected).

### 3.2 Traffic Light Modules → Arduino

Each traffic light module has `RED`, `YELLOW`, `GREEN`, `GND`.  

| Lane | RED   | YELLOW | GREEN | Arduino Pins      |
|------|-------|--------|-------|-------------------|
| 1    | R1    | Y1     | G1    | `D10`, `D11`, `D12` |
| 2    | R2    | Y2     | G2    | `D13`, `A0`, `A1` |
| 3    | R3    | Y3     | G3    | `A2`, `A3`, `A4` |
| 4    | R4    | Y4     | G4    | `A5`, `A6`, `A7`* |

\*If your Uno variant lacks A6/A7, move Y4/G4 to any free digital pins and update the firmware pin table accordingly.

All light module GND pins → Arduino `GND`.

---

## 4. Connection Diagram (ASCII)

```text
                  Lane 1 (North)
             IR1           IR2
              |            |
 D2 <-------- OUT        OUT --------> D3

        [Traffic Light 1]
   D10 -> RED    D11 -> YELLOW
   D12 -> GREEN  GND -> GND

           |             |
           |             |
   Lane 2  |             |   Lane 3
 (West)    |             |   (East)
  IR3/IR4  |             |   IR5/IR6
   ... TL2 pins      TL3 pins ...

                  Lane 4 (South)
             IR7           IR8
              |            |
 D8 <-------- OUT        OUT --------> D9

        [Traffic Light 4]
   A5 -> RED    A6 -> YELLOW
   A7 -> GREEN  GND -> GND
```

---

## 5. Road Layout & Sensor Placement

```text
                    ↑  LANE 1 (N)
                 IR2   IR1
                  |    |
            ---------------------  Stop line
                 |           |
   LANE 2 (W)    |           |    LANE 3 (E)
   IR3,IR4  ---> |   Hall    |  <--- IR5,IR6
                 |           |
            ---------------------  Stop line
                  |    |
                 IR8  IR7
                    ↓  LANE 4 (S)
```

Each lane:  
- **Near sensor** (IRx): right at the stop line.  
- **Back sensor**: a little behind, to estimate queue length.  

---

## 6. Algorithm (Lane Priority Logic)

1. **Sensor Reading**  
   - For each lane `i` (0..3):  
     - `nearState = digitalRead(IR_near[i])`  
     - `backState = digitalRead(IR_back[i])`  
     - Convert to booleans `nearVeh`, `backVeh` (`LOW` = vehicle present).  

2. **Density Score per Lane**  
   - `density[i] = (nearVeh ? 1 : 0) + (backVeh ? 1 : 0)`  
   - `density` ∈ {0, 1, 2}. Higher = more vehicles.  

3. **Best Lane Selection**  
   - If **all densities are 0** → no traffic: use **round‑robin** (cyclic) switching.  
   - Else:
     - Find lane `bestLane` with **maximum density**.  
     - If tie, keep the first lane found (round‑robin behaviour over time).  

4. **Green Timing**  
   - Maintain `currentLane` and `lastSwitchTime`.  
   - For at least `MIN_GREEN_TIME` (5s):
     - Keep `currentLane` green, ignore new sensor data.  
   - After 5s, each `CHECK_INTERVAL` (1s):
     - If `density[currentLane] == 0` **or** `bestLane != currentLane`:  
       → switch to `bestLane` and reset `lastSwitchTime`.  

5. **Light Control**  
   - `setAllRed()` turns on red in all four directions.  
   - `setLaneGreen(currentLane)` turns that lane GREEN and others RED.  

This keeps the junction **responsive to real-time traffic density**, while respecting a minimum green time to prevent flickering and accidents.

---

## 7. Source Code

The complete, well‑commented Arduino sketch is in:

- `smart_traffic/firmware/smart_traffic.ino`

You can open this file directly in the Arduino IDE and upload it to the Uno.

---

## 8. Step‑by‑Step Working

1. **Power Up**
   - Arduino starts, all IR inputs are configured with internal pull‑ups.
   - All traffic lights are initially set to **RED**.

2. **Initial Lane Selection**
   - `currentLane` defaults to lane 1 (index 0).
   - Lane 1 gets **GREEN**; others remain RED.

3. **Measurement Loop**
   - After 5s of minimum green time, the controller:  
     - Reads all 8 IR sensors.  
     - Computes density scores for all lanes.  
     - Logs values to Serial for debugging.  

4. **Priority Decision**
   - If **no cars** on any lane → controller switches green to the **next lane** in a round‑robin fashion.  
   - If there is traffic:
     - Lane with **most cars** gets next green.  
     - If the current lane still has vehicles and density is **not lower** than others, it can **keep green**.  
     - If current lane becomes empty while other lanes have cars, the green is **reassigned**.  

5. **Signal Update**
   - `setAllRed()` ensures other lanes are red.  
   - `setLaneGreen(bestLane)` sets chosen lane to green.  
   - `lastSwitchTime` is updated to enforce the next 5s minimum.  

6. **Continuous Operation**
   - System keeps looping, always respecting:  
     - **Safety:** no rapid flicker thanks to MIN_GREEN_TIME.  
     - **Fairness:** lanes with more cars get priority.  
     - **Efficiency:** empty lanes don’t waste green time.  

This documentation plus the sketch gives you a **complete project folder** similar in style to the ErgoGuard Chair firmware + docs.  


