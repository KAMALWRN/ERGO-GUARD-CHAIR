/*
 * ErgoGuard Chair - Web Dashboard
 * React Frontend
 */

import React, { useState, useEffect } from 'react';
import axios from 'axios';
import './App.css';
import PostureVisualizer from './components/PostureVisualizer';
import ScoreDashboard from './components/ScoreDashboard';
import AlertPanel from './components/AlertPanel';
import AnalyticsPanel from './components/AnalyticsPanel';

// Auto-detect backend host so the dashboard works even when opened from another device.
// Optional overrides (create `frontend/.env`):
//   REACT_APP_BACKEND_HOST=192.168.1.8
//   REACT_APP_BACKEND_PORT=3000
const BACKEND_HOST = process.env.REACT_APP_BACKEND_HOST || window.location.hostname;
const BACKEND_PORT = process.env.REACT_APP_BACKEND_PORT || '3000';
const HTTP_PROTO = window.location.protocol === 'https:' ? 'https' : 'http';
const WS_PROTO = window.location.protocol === 'https:' ? 'wss' : 'ws';

const API_BASE_URL = `${HTTP_PROTO}://${BACKEND_HOST}:${BACKEND_PORT}/api`;
const WS_URL = `${WS_PROTO}://${BACKEND_HOST}:${BACKEND_PORT}`;

function App() {
  const [sensorData, setSensorData] = useState(null);
  const [analytics, setAnalytics] = useState(null);
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    // Fetch latest data
    fetchLatestData();
    
    // Fetch analytics
    fetchAnalytics();
    
    // Set up WebSocket connection
    const ws = new WebSocket(WS_URL);
    
    ws.onopen = () => {
      console.log('WebSocket connected');
      setIsConnected(true);
    };
    
    ws.onmessage = (event) => {
      const message = JSON.parse(event.data);
      if (message.type === 'sensor_update') {
        setSensorData(message.data);
      }
    };
    
    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      setIsConnected(false);
    };
    
    ws.onclose = () => {
      console.log('WebSocket disconnected');
      setIsConnected(false);
    };
    
    // Poll for updates every 5 seconds (fallback)
    const interval = setInterval(() => {
      fetchLatestData();
      fetchAnalytics();
    }, 5000);
    
    return () => {
      ws.close();
      clearInterval(interval);
    };
  }, []);

  const fetchLatestData = async () => {
    try {
      const response = await axios.get(`${API_BASE_URL}/latest`);
      setSensorData(response.data);
    } catch (error) {
      console.error('Error fetching latest data:', error);
    }
  };

  const fetchAnalytics = async () => {
    try {
      const response = await axios.get(`${API_BASE_URL}/analytics?hours=24`);
      setAnalytics(response.data);
    } catch (error) {
      console.error('Error fetching analytics:', error);
    }
  };

  return (
    <div className="App">
      <header className="App-header">
        <h1>ErgoGuard Chair Dashboard</h1>
        <div className="connection-status">
          <span className={isConnected ? 'connected' : 'disconnected'}>
            {isConnected ? '● Connected' : '○ Disconnected'}
          </span>
        </div>
      </header>

      <main className="dashboard">
        <div className="dashboard-grid">
          {/* Left Column */}
          <div className="left-column">
            <PostureVisualizer data={sensorData} />
            <AlertPanel data={sensorData} />
          </div>

          {/* Right Column */}
          <div className="right-column">
            <ScoreDashboard data={sensorData} analytics={analytics} />
            <AnalyticsPanel analytics={analytics} />
          </div>
        </div>
      </main>

      <footer className="App-footer">
        <div className="App-footer-inner">
          Project developed by <strong>KAMALESHWARAN T R</strong> - <strong>KANNAN B</strong> - <strong>KARTHICK K</strong>.
        </div>
      </footer>
    </div>
  );
}

export default App;

