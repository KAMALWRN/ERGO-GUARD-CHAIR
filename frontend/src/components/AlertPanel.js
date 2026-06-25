import React from 'react';
import './AlertPanel.css';

const AlertPanel = ({ data }) => {
  if (!data) {
    return (
      <div className="alert-panel">
        <h2>Alerts & Interventions</h2>
        <div className="loading">Waiting for data...</div>
      </div>
    );
  }

  const getAlertLevel = (score) => {
    if (score < 50) return { level: 'critical', message: 'Critical - Stand up immediately!', color: '#F44336' };
    if (score < 70) return { level: 'warning', message: 'Warning - Adjust posture or take a break', color: '#FF9800' };
    return { level: 'good', message: 'Good posture maintained', color: '#4CAF50' };
  };

  const alert = getAlertLevel(data.ergo_stress_score || 0);

  const getRecommendations = () => {
    const recommendations = [];

    // High-level status from firmware (WiFi, calibration, long sitting, etc.)
    if (data.status_message) {
      recommendations.push(data.status_message);
    }

    if (data.posture_score < 60) {
      recommendations.push('Sit up straight and keep your back in contact with the backrest');
    }

    if (data.asymmetry_score < 70) {
      recommendations.push('Balance your weight between seat front/back and upper/lower back');
    }

    if (data.sedentary_score < 60) {
      recommendations.push('Stand up or walk for 2–3 minutes to reset your posture');
    }

    // Explicit break reminder (short demo timing: minutes instead of hours)
    if ((data.sitting_duration || 0) >= 120) {
      recommendations.push('You have been sitting for more than 2 minutes (demo) – please take a break now.');
    } else if ((data.sitting_duration || 0) >= 60) {
      recommendations.push('You have been sitting for more than 1 minute (demo) – consider a short micro-break.');
    }

    if (recommendations.length === 0) {
      recommendations.push('Keep up the good work!');
    }

    return recommendations;
  };

  return (
    <div className="alert-panel">
      <h2>Alerts & Interventions</h2>
      
      <div className="alert-banner" style={{ backgroundColor: alert.color }}>
        <div className="alert-icon">
          {alert.level === 'critical' && '⚠️'}
          {alert.level === 'warning' && '⚡'}
          {alert.level === 'good' && '✓'}
        </div>
        <div className="alert-message">{alert.message}</div>
      </div>

      <div className="recommendations">
        <h3>Recommendations</h3>
        <ul>
          {getRecommendations().map((rec, index) => (
            <li key={index}>{rec}</li>
          ))}
        </ul>
      </div>

      {/* Environment section removed for 4-sensor configuration (no MQ135/DHT) */}
    </div>
  );
};

export default AlertPanel;

