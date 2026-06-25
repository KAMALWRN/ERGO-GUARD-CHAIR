import React from 'react';
import { Pie } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  ArcElement,
  Tooltip,
  Legend
} from 'chart.js';
import './AnalyticsPanel.css';

ChartJS.register(ArcElement, Tooltip, Legend);

const AnalyticsPanel = ({ analytics }) => {
  if (!analytics) {
    return (
      <div className="analytics-panel">
        <h2>Analytics</h2>
        <div className="loading">Loading analytics...</div>
      </div>
    );
  }

  const postureData = {
    labels: ['Good', 'Fair', 'Poor'],
    datasets: [
      {
        data: [
          analytics.postureBreakdown?.good || 0,
          analytics.postureBreakdown?.fair || 0,
          analytics.postureBreakdown?.poor || 0,
        ],
        backgroundColor: ['#4CAF50', '#FF9800', '#F44336'],
      },
    ],
  };

  return (
    <div className="analytics-panel">
      <h2>24-Hour Analytics</h2>
      
      <div className="stats-grid">
        <div className="stat-card">
          <label>Average Score</label>
          <div className="stat-value">{analytics.avgErgoScore?.toFixed(1) || 0}</div>
        </div>
        
        <div className="stat-card">
          <label>Min Score</label>
          <div className="stat-value">{analytics.minScore?.toFixed(1) || 0}</div>
        </div>
        
        <div className="stat-card">
          <label>Max Score</label>
          <div className="stat-value">{analytics.maxScore?.toFixed(1) || 0}</div>
        </div>
        
        <div className="stat-card">
          <label>Interventions</label>
          <div className="stat-value">{analytics.interventions || 0}</div>
        </div>
      </div>

      <div className="posture-chart">
        <h3>Posture Breakdown</h3>
        <Pie data={postureData} />
      </div>

      <div className="summary">
        <p>
          <strong>Total Readings:</strong> {analytics.totalReadings || 0}
        </p>
        <p>
          <strong>Time Range:</strong> {analytics.timeRange || '24 hours'}
        </p>
      </div>
    </div>
  );
};

export default AnalyticsPanel;

