# ErgoGuard Chair - Complete Components List

## 🛒 Shopping List (India - Amazon/Flipkart/Electronics Stores)

### ✅ Core Controller
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| ESP32 Development Board (ESP32-WROOM-32) | 1 | 300 | Search: "ESP32 development board" |
| USB Cable (Micro-USB) | 1 | 50 | For programming & power |

### ✅ Pressure Sensors
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| FSR 402 (Force Sensitive Resistor) | 8 | 800 | ₹100 each - Search: "FSR 402 pressure sensor" |
| 10kΩ Resistors (for pull-down) | 8 | 50 | ₹5-10 each |

### ✅ Motion Sensor
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| MPU6050 (6-axis IMU) | 1 | 150 | Search: "MPU6050 gyroscope accelerometer" |

### ✅ Environment Sensors (Optional but Recommended)
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| MQ135 Gas Sensor (CO2/Air Quality) | 1 | 100 | Cheaper alternative to SCD30 |
| DHT22 Temperature + Humidity | 1 | 200 | Search: "DHT22 sensor" |

### ✅ Actuators
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| Vibration Motor (Coin Type, 3V) | 2 | 100 | ₹50 each - Search: "coin vibration motor" |
| RGB LED Strip WS2812B (30 LEDs) | 1 | 300 | Search: "WS2812B RGB LED strip" |
| Transistor (2N2222) for motor control | 2 | 20 | ₹10 each |

### ✅ Power & Regulation
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| 5V 2A USB Power Adapter | 1 | 150 | Phone charger works |
| AMS1117 3.3V Regulator | 1 | 50 | For sensor power |
| Capacitors (100µF, 10µF) | 2 | 30 | For power filtering |

### ✅ Connectivity & Display (Optional)
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| OLED Display 128x64 (SSD1306) | 1 | 200 | Optional - for local display |

### ✅ Miscellaneous
| Component | Quantity | Price (₹) | Link/Notes |
|-----------|----------|-----------|------------|
| Jumper Wires (Male-Female, Male-Male) | 40 | 100 | ₹2-3 each |
| Breadboard (Half-size) | 1 | 100 | For prototyping |
| Perfboard/PCB (for final assembly) | 1 | 200 | Optional - for permanent build |
| Enclosure Box (Plastic) | 1 | 300 | Or 3D print / DIY cardboard |
| Velcro/Adhesive Strips | 1 pack | 100 | For mounting sensors |

### ✅ Software & Tools (Free)
- Arduino IDE (Free)
- Node.js (Free)
- MongoDB (Free - MongoDB Atlas cloud)
- React (Free)
- VS Code (Free)

---

## 📊 Total Cost Breakdown

| Category | Cost (₹) |
|----------|----------|
| Core Controller | 350 |
| Pressure Sensors | 850 |
| Motion Sensor | 150 |
| Environment Sensors | 300 |
| Actuators | 420 |
| Power & Regulation | 230 |
| Display (Optional) | 200 |
| Miscellaneous | 700 |
| **TOTAL** | **₹3,200** |

*With optional OLED display: ₹3,400*

---

## 🔌 Pin Connections (ESP32)

### FSR Sensors (8x Analog Inputs)
- FSR Seat Front-Left → GPIO 36 (ADC1_CH0)
- FSR Seat Front-Right → GPIO 39 (ADC1_CH3)
- FSR Seat Back-Left → GPIO 34 (ADC1_CH6)
- FSR Seat Back-Right → GPIO 35 (ADC1_CH7)
- FSR Backrest Upper-Left → GPIO 32 (ADC1_CH4)
- FSR Backrest Upper-Right → GPIO 33 (ADC1_CH5)
- FSR Backrest Lumbar-Left → GPIO 25 (DAC)
- FSR Backrest Lumbar-Right → GPIO 26 (DAC)

### MPU6050 (I2C)
- SDA → GPIO 21
- SCL → GPIO 22
- VCC → 3.3V
- GND → GND

### MQ135 (Analog)
- A0 → GPIO 27 (ADC2_CH7)
- VCC → 5V (via regulator)
- GND → GND

### DHT22 (Digital)
- Data → GPIO 4
- VCC → 3.3V
- GND → GND

### Vibration Motors
- Motor 1 (Seat) → GPIO 16 (via transistor)
- Motor 2 (Backrest) → GPIO 17 (via transistor)

### RGB LED Strip (WS2812B)
- Data → GPIO 2
- VCC → 5V
- GND → GND

### OLED Display (Optional)
- SDA → GPIO 21 (shared with MPU6050)
- SCL → GPIO 22 (shared with MPU6050)
- VCC → 3.3V
- GND → GND

---

## 📐 Sensor Placement Guide

### Seat Sensors (4x FSR)
```
        [Back-Left]    [Back-Right]
              |              |
              |              |
        [Front-Left]   [Front-Right]
```

### Backrest Sensors (4x FSR)
```
    [Upper-Left]    [Upper-Right]
           |              |
           |              |
    [Lumbar-Left]  [Lumbar-Right]
```

### IMU Placement
- Mount MPU6050 on backrest (center, upper portion)
- Ensure flat mounting surface

### Vibration Motors
- One under seat cushion (center)
- One in backrest (lumbar region)

### LED Strip
- Mount along backrest edge (visible from front)
- Or along seat front edge

---

## 🛠️ Assembly Tips

1. **FSR Wiring**: Each FSR needs a pull-down resistor (10kΩ) connected to GND
2. **Power Management**: Use separate 5V rail for motors/LEDs, 3.3V for sensors
3. **Mounting**: Use velcro/adhesive for easy removal/adjustment
4. **Cable Management**: Use cable ties to keep wires organized
5. **Testing**: Test each sensor individually before final assembly

---

## ⚠️ Safety Notes

- Ensure proper power isolation between 5V and 3.3V rails
- Use appropriate current-limiting resistors for LEDs
- Test vibration motors at low intensity first
- Ensure all connections are secure before powering on

