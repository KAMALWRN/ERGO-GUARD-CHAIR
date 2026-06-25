import React from 'react';
import { Line } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from 'chart.js';
import './ScoreDashboard.css';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const ScoreDashboard = ({ data, analytics }) => {
  const getScoreColor = (score) => {
    if (score >= 80) return '#4CAF50';
    if (score >= 60) return '#FF9800';
    return '#F44336';
  };

  const getScoreLabel = (score) => {
    if (score >= 80) return 'Excellent';
    if (score >= 60) return 'Good';
    if (score >= 40) return 'Fair';
    return 'Poor';
  };

  // Mock historical data (replace with real API call)
  const historicalData = {
    labels: Array.from({ length: 20 }, (_, i) => `${i * 5}m ago`).reverse(),
    datasets: [
      {
        label: 'Ergo-Stress Score',
        data: Array.from({ length: 20 }, () => Math.random() * 40 + 50),
        borderColor: '#2196F3',
        backgroundColor: 'rgba(33, 150, 243, 0.1)',
        tension: 0.4,
      },
    ],
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      y: {
        beginAtZero: true,
        max: 100,
      },
    },
  };

  return (
    <div className="score-dashboard">
      <h2>Ergo-Stress Score</h2>
      
      {data && (
        <div className="current-score">
          <div 
            className="score-circle" 
            style={{ borderColor: getScoreColor(data.ergo_stress_score || 0) }}
          >
            <div className="score-value">
              {data.ergo_stress_score?.toFixed(0) || 0}
            </div>
            <div className="score-label">
              {getScoreLabel(data.ergo_stress_score || 0)}
            </div>
          </div>
        </div>
      )}

      <div className="score-breakdown">
        <div className="score-item">
          <label>Posture:</label>
          <div className="score-bar">
            <div 
              className="score-fill" 
              style={{ 
                width: `${data?.posture_score || 0}%`,
                backgroundColor: getScoreColor(data?.posture_score || 0)
              }}
            />
            <span>{data?.posture_score?.toFixed(0) || 0}</span>
          </div>
        </div>
        
        <div className="score-item">
          <label>Asymmetry:</label>
          <div className="score-bar">
            <div 
              className="score-fill" 
              style={{ 
                width: `${data?.asymmetry_score || 0}%`,
                backgroundColor: getScoreColor(data?.asymmetry_score || 0)
              }}
            />
            <span>{data?.asymmetry_score?.toFixed(0) || 0}</span>
          </div>
        </div>
        
        <div className="score-item">
          <label>Sedentary:</label>
          <div className="score-bar">
            <div 
              className="score-fill" 
              style={{ 
                width: `${data?.sedentary_score || 0}%`,
                backgroundColor: getScoreColor(data?.sedentary_score || 0)
              }}
            />
            <span>{data?.sedentary_score?.toFixed(0) || 0}</span>
          </div>
        </div>
      </div>

      <div className="score-chart">
        <Line data={historicalData} options={chartOptions} />
      </div>

      {analytics && (
        <div className="analytics-summary">
          <div className="stat">
            <label>Avg Score (24h):</label>
            <span>{analytics.avgErgoScore?.toFixed(1) || 0}</span>
          </div>
          <div className="stat">
            <label>Interventions:</label>
            <span>{analytics.interventions || 0}</span>
          </div>
        </div>
      )}
    </div>
  );
};

export default ScoreDashboard;

