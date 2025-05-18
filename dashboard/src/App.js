import React, { useState } from 'react';
import Login from './components/Login';
import Dashboard from './components/Dashboard';

function App() {
  const [user, setUser] = useState(null);

  if (!user) {
    return <Login onLogin={setUser} />;
  }
  return <Dashboard user={user} />;
}

export default App;
