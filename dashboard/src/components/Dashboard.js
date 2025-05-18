import React, { useEffect, useState } from 'react';
import axios from 'axios';

function Dashboard({ user }) {
  const [detections, setDetections] = useState([]);

  useEffect(() => {
    axios.get('http://localhost:5000/detections').then(res => {
      setDetections(res.data);
    });
  }, []);

  return (
    <div>
      <h1>Welcome, {user.username} ({user.role})</h1>
      <h2>Detections:</h2>
      <ul>
        {detections.map(d => (
          <li key={d.id}>
            {d.timestamp} - {d.object_type} - <a href={d.image_url} target="_blank" rel="noreferrer">Image</a>
          </li>
        ))}
      </ul>
    </div>
  );
}

export default Dashboard;
