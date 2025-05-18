import React, { useState } from 'react';
import axios from 'axios';

function Login({ onLogin }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    try {
      const res = await axios.post('http://localhost:5000/login', { username, password });
      if (res.data.message === 'Login successful') {
        onLogin({ username: res.data.username, role: res.data.role });
      } else {
        setError('Login failed');
      }
    } catch {
      setError('Invalid credentials');
    }
  };

  return (
    <form onSubmit={handleSubmit}>
      <input placeholder="Username" value={username} onChange={e => setUsername(e.target.value)} />
      <input type="password" placeholder="Password" value={password} onChange={e => setPassword(e.target.value)} />
      <button type="submit">Login</button>
      {error && <p style={{color:'red'}}>{error}</p>}
    </form>
  );
}

export default Login;
