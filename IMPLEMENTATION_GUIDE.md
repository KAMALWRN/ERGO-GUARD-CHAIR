# ErgoGuard Chair - Complete Implementation Guide

## 📋 Quick Start Checklist

- [ ] Order components (see COMPONENTS_LIST.md)
- [ ] Set up development environment
- [ ] Assemble hardware
- [ ] Flash firmware to ESP32
- [ ] Set up backend server
- [ ] Set up frontend dashboard
- [ ] Test end-to-end
- [ ] Prepare demo

---

## 🛠️ STEP 1: Development Environment Setup

### Install Required Software

1. **Arduino IDE** (for ESP32 firmware)
   - Download: https://www.arduino.cc/en/software
   - Install ESP32 board support:
     - File → Preferences → Additional Board Manager URLs
     - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
     - Tools → Board → Boards Manager → Search "ESP32" → Install

2. **Node.js** (for backend)
   - Download: https://nodejs.org/ (LTS version)
   - Verify: `node --version` and `npm --version`

3. **MongoDB** (database)
   - Option A: MongoDB Atlas (cloud, free tier) - Recommended
     - Sign up: https://www.mongodb.com/cloud/atlas
     - Create free cluster
     - Get connection string
   - Option B: Local MongoDB
     - Download: https://www.mongodb.com/try/download/community

4. **VS Code** (optional but recommended)
   - Download: https://code.visualstudio.com/

### Install Arduino Libraries

Open Arduino IDE → Sketch → Include Library → Manage Libraries → Install:
- `MPU6050` by Electronic Cats
- `DHT sensor library` by Adafruit
- `Adafruit NeoPixel` by Adafruit
- `WiFi` (built-in)
- `HTTPClient` (built-in)

---

## 🔌 STEP 2: Hardware Assembly

### 2.1 Assemble Sensor Array

1. **FSR Sensors (8x)**
   - Connect each FSR to ESP32 analog pin
   - Add 10kΩ pull-down resistor to GND
   - Test each sensor individually

2. **MPU6050**
   - Connect VCC → 3.3V
   - Connect GND → GND
   - Connect SDA → GPIO 21
   - Connect SCL → GPIO 22

3. **Environment Sensors**
   - MQ135: A0 → GPIO 27, VCC → 5V, GND → GND
   - DHT22: Data → GPIO 4, VCC → 3.3V, GND → GND

4. **Actuators**
   - Vibration Motors: Connect via transistors to GPIO 16 & 17
   - LED Strip: Data → GPIO 2, VCC → 5V, GND → GND

### 2.2 Mount Sensors on Chair

- **Seat**: Place 4 FSR sensors under cushion (corners)
- **Backrest**: Place 4 FSR sensors between padding and frame
- **IMU**: Mount MPU6050 on upper backrest (center)
- **Motors**: Embed in seat and backrest padding
- **LED Strip**: Mount along backrest edge

### 2.3 Power Supply

- Connect 5V 2A USB adapter to ESP32 VIN
- Ensure proper grounding
- Test power supply before final assembly

---

## 💻 STEP 3: Firmware Setup

### 3.1 Configure WiFi

Edit `firmware/ergoguard_chair.ino`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverURL = "http://YOUR_SERVER_IP:3000/api/sensor-data";
```

### 3.2 Upload Firmware

1. Connect ESP32 via USB
2. Select board: Tools → Board → ESP32 Dev Module
3. Select port: Tools → Port → (your COM port)
4. Upload: Sketch → Upload
5. Open Serial Monitor (115200 baud) to see output

### 3.3 Test Sensors

- Check Serial Monitor for sensor readings
- Verify all sensors respond
- Check WiFi connection status

---

## 🖥️ STEP 4: Backend Setup

### 4.1 Install Dependencies

```bash
cd backend
npm install
```

### 4.2 Configure Environment

Create `backend/.env`:
```
MONGODB_URI=mongodb+srv://username:password@cluster.mongodb.net/ergoguard
PORT=3000
```

### 4.3 Start Server

```bash
npm start
# or for development with auto-reload:
npm run dev
```

Server should be running on `http://localhost:3000`

### 4.4 Test API

Open browser: `http://localhost:3000/health`
Should return: `{"status":"ok","timestamp":"..."}`

---

## 🎨 STEP 5: Frontend Setup

### 5.1 Install Dependencies

```bash
cd frontend
npm install
```

### 5.2 Configure API URL

Edit `frontend/src/App.js`:
```javascript
const API_BASE_URL = 'http://YOUR_SERVER_IP:3000/api';
const WS_URL = 'ws://YOUR_SERVER_IP:3000';
```

### 5.3 Start Development Server

```bash
npm start
```

Dashboard should open at `http://localhost:3000` (or different port if 3000 is taken)

---

## 🧪 STEP 6: Testing

### 6.1 Hardware Test

1. **FSR Sensors**: Press each sensor → Check Serial Monitor for readings
2. **IMU**: Tilt chair → Check pitch/roll values
3. **Motors**: Trigger intervention → Motors should vibrate
4. **LED**: Check LED color changes with score

### 6.2 Integration Test

1. **Data Flow**: ESP32 → Backend → Frontend
   - Check Serial Monitor: "HTTP Response code: 200"
   - Check Backend logs: "Data received"
   - Check Frontend: Real-time updates

2. **WebSocket**: Check browser console for WebSocket connection

3. **Database**: Check MongoDB for saved sensor readings

### 6.3 Calibration Test

1. Sit normally for 2 minutes
2. Check Serial Monitor for "Calibration complete!"
3. Move around → Check if scores update correctly

---

## 🎬 STEP 7: Demo Preparation

### 7.1 Demo Script

See `PROJECT_BLUEPRINT.md` → Demo Script section

### 7.2 Visual Setup

- **Chair**: Clean, sensors hidden, LED visible
- **Laptop/Tablet**: Dashboard open, full screen
- **Backup**: Have Serial Monitor open as backup

### 7.3 Practice Points

1. **Good Posture Demo**: Sit correctly → Show green score
2. **Posture Deviation**: Slouch → Show score drop + intervention
3. **Asymmetry**: Lean to side → Show asymmetry alert
4. **Sedentary Risk**: Sit still → Show break suggestion
5. **Analytics**: Show daily trends, posture breakdown

---

## 🐛 Troubleshooting

### ESP32 Not Connecting

- Check USB cable (data cable, not charge-only)
- Install ESP32 drivers (CP2102 or CH340)
- Try different USB port

### WiFi Connection Failed

- Check SSID/password
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check router settings

### Sensors Not Reading

- Check wiring connections
- Verify pin numbers match code
- Test with multimeter

### Backend Not Receiving Data

- Check server URL in firmware
- Check firewall settings
- Verify MongoDB connection

### Frontend Not Updating

- Check API URL in frontend code
- Check browser console for errors
- Verify WebSocket connection

---

## 📊 Performance Optimization

### Reduce Update Frequency

In `ergoguard_chair.ino`:
```cpp
delay(100); // Change to 500 for slower updates
```

### Reduce Database Writes

In `backend/server.js`, add sampling:
```javascript
// Only save every 10th reading
if (Math.random() < 0.1) {
  await reading.save();
}
```

### Optimize Frontend

- Use React.memo for components
- Debounce chart updates
- Limit historical data points

---

## 🚀 Deployment (Optional)

### Backend Deployment

- **Heroku**: Easy deployment
- **AWS EC2**: More control
- **DigitalOcean**: Simple VPS

### Frontend Deployment

- **Netlify**: Free hosting
- **Vercel**: Free hosting
- **GitHub Pages**: Free hosting

### Database

- **MongoDB Atlas**: Free tier (512MB)
- Keep connection string secure

---

## ✅ Final Checklist

- [ ] All sensors working
- [ ] WiFi connected
- [ ] Backend receiving data
- [ ] Frontend displaying data
- [ ] Calibration complete
- [ ] Interventions working
- [ ] Demo script practiced
- [ ] Documentation ready

---

## 📞 Support

If you encounter issues:
1. Check Serial Monitor for ESP32 errors
2. Check browser console for frontend errors
3. Check backend logs for server errors
4. Review wiring diagram
5. Verify all connections

---

**Good luck with your project! 🎉**

