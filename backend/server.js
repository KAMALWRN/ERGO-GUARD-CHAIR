/*
 * ErgoGuard Chair - Backend Server
 * Node.js + Express + MongoDB + WebSocket
 */

const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const http = require('http');
const WebSocket = require('ws');
require('dotenv').config();

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Middleware
app.use(cors());
app.use(express.json());

// MongoDB Connection
const MONGODB_URI = process.env.MONGODB_URI || 'mongodb://localhost:27017/ErgoGuard_Chair';
mongoose.connect(MONGODB_URI, {
  useNewUrlParser: true,
  useUnifiedTopology: true,
});

mongoose.connection.on('connected', () => {
  console.log('MongoDB connected');
});

mongoose.connection.on('error', (err) => {
  console.error('MongoDB connection error:', err);
});

// ==================== DATABASE SCHEMAS ====================
const SensorReadingSchema = new mongoose.Schema({
  deviceId: { type: String, default: 'chair-001' },
  // 4-FSR configuration
  // FSR configuration
  fsr_seat_front_left: Number,
  fsr_seat_front_right: Number,
  fsr_seat_front: Number,
  fsr_seat_back: Number,
  fsr_back_upper: Number,
  fsr_back_lower: Number,
  // FSR % of personal max (learned during calibration)
  fsr_seat_front_left_pct: Number,
  fsr_seat_front_right_pct: Number,
  fsr_seat_back_pct: Number,
  fsr_back_upper_pct: Number,
  fsr_back_lower_pct: Number,
  // learned max values (debug)
  fsr_max_front_left: Number,
  fsr_max_front_right: Number,
  fsr_max_seat_back: Number,
  fsr_max_back_upper: Number,
  fsr_max_back_lower: Number,
  // IMU
  pitch: Number,
  roll: Number,
  // Scores
  posture_score: Number,
  asymmetry_score: Number,
  sedentary_score: Number,
  ergo_stress_score: Number,
  // Extra UX fields from firmware
  is_sitting: Number,
  sitting_duration: Number, // seconds
  status_message: String,
  timestamp: Number,
  createdAt: { type: Date, default: Date.now }
});

const UserSessionSchema = new mongoose.Schema({
  deviceId: String,
  startTime: Date,
  endTime: Date,
  totalSittingTime: Number, // seconds
  avgErgoScore: Number,
  interventions: Number,
  createdAt: { type: Date, default: Date.now }
});

const SensorReading = mongoose.model('SensorReading', SensorReadingSchema);
const UserSession = mongoose.model('UserSession', UserSessionSchema);

// ==================== WEBSOCKET BROADCAST ====================
function broadcastToClients(data) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
}

wss.on('connection', (ws) => {
  console.log('WebSocket client connected');
  
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
  });
});

// ==================== API ROUTES ====================

// POST /api/sensor-data - Receive data from ESP32
app.post('/api/sensor-data', async (req, res) => {
  try {
    const sensorData = req.body;
    
    // Save to database
    const reading = new SensorReading(sensorData);
    await reading.save();
    
    // Broadcast to WebSocket clients
    broadcastToClients({
      type: 'sensor_update',
      data: sensorData
    });
    
    res.json({ success: true, message: 'Data received' });
  } catch (error) {
    console.error('Error saving sensor data:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// GET /api/latest - Get latest sensor reading
app.get('/api/latest', async (req, res) => {
  try {
    const latest = await SensorReading.findOne()
      .sort({ createdAt: -1 })
      .limit(1);
    
    res.json(latest);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// GET /api/history - Get historical data
app.get('/api/history', async (req, res) => {
  try {
    const { hours = 1, limit = 100 } = req.query;
    const startTime = new Date(Date.now() - hours * 60 * 60 * 1000);
    
    const history = await SensorReading.find({
      createdAt: { $gte: startTime }
    })
      .sort({ createdAt: -1 })
      .limit(parseInt(limit));
    
    res.json(history);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// GET /api/analytics - Get analytics data
app.get('/api/analytics', async (req, res) => {
  try {
    const { hours = 24 } = req.query;
    const startTime = new Date(Date.now() - hours * 60 * 60 * 1000);
    
    const readings = await SensorReading.find({
      createdAt: { $gte: startTime }
    });
    
    // Calculate statistics
    const scores = readings.map(r => r.ergo_stress_score);
    const avgScore = scores.reduce((a, b) => a + b, 0) / scores.length;
    const minScore = Math.min(...scores);
    const maxScore = Math.max(...scores);
    
    // Count interventions (score < 70)
    const interventions = readings.filter(r => r.ergo_stress_score < 70).length;
    
    // Posture breakdown
    const postureBreakdown = {
      good: readings.filter(r => r.posture_score >= 80).length,
      fair: readings.filter(r => r.posture_score >= 60 && r.posture_score < 80).length,
      poor: readings.filter(r => r.posture_score < 60).length
    };
    
    res.json({
      avgErgoScore: avgScore,
      minScore,
      maxScore,
      totalReadings: readings.length,
      interventions,
      postureBreakdown,
      timeRange: `${hours} hours`
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// GET /api/sessions - Get user sessions
app.get('/api/sessions', async (req, res) => {
  try {
    const sessions = await UserSession.find()
      .sort({ createdAt: -1 })
      .limit(10);
    
    res.json(sessions);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date() });
});

// ==================== SERVER START ====================
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`ErgoGuard Backend Server running on port ${PORT}`);
  console.log(`WebSocket server ready`);
});
