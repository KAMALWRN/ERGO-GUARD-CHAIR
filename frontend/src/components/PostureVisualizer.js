import React from 'react';
import './PostureVisualizer.css';

const PostureVisualizer = ({ data }) => {
  if (!data) {
    return (
      <div className="posture-visualizer">
        <h2>Posture Visualization</h2>
        <div className="loading">Waiting for data...</div>
      </div>
    );
  }

  // Use % of personal max (learned during calibration) for visualization.
  // This makes intensity consistent for different body weights.
  const seatPressurePct = {
    frontLeft: data.fsr_seat_front_left_pct ?? 0,
    frontRight: data.fsr_seat_front_right_pct ?? 0,
    back: data.fsr_seat_back_pct ?? 0,
  };

  const backrestPressurePct = {
    upper: data.fsr_back_upper_pct ?? 0,
    lower: data.fsr_back_lower_pct ?? 0,
  };

  const opacityFromPct = (pct) => {
    // keep cells visible even at low % (UI clarity)
    const clamped = Math.max(0, Math.min(100, pct));
    const minOpacity = 0.12;
    return minOpacity + (clamped / 100) * (1 - minOpacity);
  };

  // Determine posture status
  const getPostureStatus = () => {
    const score = data.posture_score || 0;
    if (score >= 80) return { text: 'Good', color: '#4CAF50' };
    if (score >= 60) return { text: 'Fair', color: '#FF9800' };
    return { text: 'Poor', color: '#F44336' };
  };

  const postureStatus = getPostureStatus();

  return (
    <div className="posture-visualizer">
      <h2>Posture Visualization</h2>
      
      <div className="posture-status">
        <div className="status-indicator" style={{ backgroundColor: postureStatus.color }}>
          {postureStatus.text}
        </div>
        <div className="score-display">
          Score: {data.posture_score?.toFixed(1) || 0}
        </div>
      </div>

      <div className="chair-diagram">
        {/* Backrest (Upper / Lower) */}
        <div className="backrest">
          <div className="pressure-grid">
            <div
              className="pressure-cell"
              style={{ opacity: opacityFromPct(backrestPressurePct.upper) }}
            >
              Upper
            </div>
            <div
              className="pressure-cell"
              style={{ opacity: opacityFromPct(backrestPressurePct.lower) }}
            >
              Lower
            </div>
          </div>
        </div>

        {/* Seat (Front L / Front R / Back) */}
        <div className="seat">
          <div className="pressure-grid pressure-grid-seat">
            <div
              className="pressure-cell"
              style={{ opacity: opacityFromPct(seatPressurePct.frontLeft) }}
            >
              Front L
            </div>
            <div
              className="pressure-cell"
              style={{ opacity: opacityFromPct(seatPressurePct.frontRight) }}
            >
              Front R
            </div>
            <div
              className="pressure-cell"
              style={{ opacity: opacityFromPct(seatPressurePct.back) }}
            >
              Back
            </div>
          </div>
        </div>
      </div>

      <div className="posture-metrics">
        <div className="metric">
          <label>Pitch:</label>
          <span>{data.pitch?.toFixed(1) || 0}°</span>
        </div>
        <div className="metric">
          <label>Roll:</label>
          <span>{data.roll?.toFixed(1) || 0}°</span>
        </div>
      </div>
    </div>
  );
};

export default PostureVisualizer;

