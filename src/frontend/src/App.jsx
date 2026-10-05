import React, { useState, useEffect } from 'react';

function App() {
  const [sysStatus, setSysStatus] = useState('checking');
  const [profile, setProfile] = useState(null);
  const [authData, setAuthData] = useState(null);
  
  // State untuk form modifikasi tema
  const [formName, setFormName] = useState('');
  const [formTheme, setFormTheme] = useState('dark');
  const [formColor, setFormColor] = useState('#007bff');
  const [saveStatus, setSaveStatus] = useState('');

  const API_BASE = "http://" + "192.168.1.22" + ":8080" + "/api";

  const loadData = () => {
    Promise.all([
      fetch(`${API_BASE}/core-auth`).then(res => res.json()),
      fetch(`${API_BASE}/company-profile`).then(res => res.json())
    ])
    .then(([authRes, profileRes]) => {
      setAuthData(authRes);
      if (profileRes.status === 'success') {
        setProfile(profileRes);
        setFormName(profileRes.company_name);
        setFormTheme(profileRes.config?.theme || 'dark');
        setFormColor(profileRes.config?.primary_color || '#007bff');
      }
      setSysStatus('ready');
    })
    .catch(() => setSysStatus('connection_error'));
  };

  useEffect(() => {
    fetch(`${API_BASE}/core-setup`)
      .then(res => res.json())
      .then(setupRes => {
        if (setupRes.status === 'setup_required') {
          setSysStatus('setup');
        } else {
          loadData();
        }
      })
      .catch(() => setSysStatus('connection_error'));
  }, []);

  const handleSaveTheme = (e) => {
    e.preventDefault();
    setSaveStatus('Menyimpan ke SQLite...');
    
    fetch(`${API_BASE}/company-profile`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        company_name: formName,
        theme: formTheme,
        primary_color: formColor
      })
    })
    .then(res => res.json())
    .then(data => {
      if (data.status === 'success') {
        setSaveStatus('Berhasil disimpan!');
        loadData(); // Segarkan layout visual React secara instant
      } else {
        setSaveStatus('Gagal menyimpan: ' + data.message);
      }
    })
    .catch(() => setSaveStatus('Galat jaringan komunikasi POST.'));
  };

  if (sysStatus === 'checking') return <div style={{padding: '20px'}}>Memuat Ekosistem...</div>;
  if (sysStatus === 'connection_error') return <div style={{padding: '20px', color: 'red'}}>Galat Hubungan Server Backend.</div>;

  const isDarkMode = profile?.config?.theme === 'dark';
  const primaryColor = profile?.config?.primary_color || '#007bff';

  return (
    <div style={{ 
      minHeight: '100vh', 
      backgroundColor: isDarkMode ? '#121212' : '#f8f9fa', 
      color: isDarkMode ? '#e0e0e0' : '#212529',
      fontFamily: 'sans-serif',
      transition: 'all 0.3s ease'
    }}>
      <header style={{ display: 'flex', justifyContent: 'space-between', padding: '15px 30px', background: 'rgba(0,0,0,0.05)', borderBottom: `3px solid ${primaryColor}` }}>
        <div style={{ fontSize: '22px', fontWeight: 'bold', color: primaryColor }}>{profile?.company_name}</div>
        <div style={{ background: primaryColor, color: 'white', padding: '6px 16px', borderRadius: '20px', fontSize: '14px' }}>
          👤 {authData?.google_email}
        </div>
      </header>

      <main style={{ maxWidth: '800px', margin: '0 auto', padding: '40px 20px' }}>
        <div style={{ textAlign: 'center', marginBottom: '30px' }}>
          <h1>Selamat Datang di Portal</h1>
          <p style={{ color: '#777' }}>Tampilan ini dikendalikan penuh secara modular oleh biner Qt C++ kontainer server.</p>
        </div>

        {/* CUSTOMIZER PANEL FORM */}
        <div style={{ 
          background: isDarkMode ? '#1e1e1e' : 'white', 
          padding: '25px', 
          borderRadius: '10px',
          boxShadow: '0 4px 15px rgba(0,0,0,0.1)',
          color: isDarkMode ? '#fff' : '#333'
        }}>
          <h3 style={{ color: primaryColor, marginTop: 0, borderBottom: '1px solid #444', paddingBottom: '10px' }}>
            Azzammar Modular Customizer Live:
          </h3>
          <form onSubmit={handleSaveTheme} style={{ display: 'flex', flexDirection: 'column', gap: '15px', marginTop: '15px' }}>
            <label>
              Nama Perusahaan: <br/>
              <input type="text" value={formName} onChange={e => setFormName(e.target.value)} style={{ width: '100%', padding: '8px', marginTop: '5px', borderRadius: '4px', border: '1px solid #ccc' }} />
            </label>
            <label>
              Mode Visual Tema: <br/>
              <select value={formTheme} onChange={e => setFormTheme(e.target.value)} style={{ width: '100%', padding: '8px', marginTop: '5px', borderRadius: '4px' }}>
                <option value="light">Light Mode (Terang)</option>
                <option value="dark">Dark Mode (Gelap)</option>
              </select>
            </label>
            <label>
              Warna Utama Akses Aksen (Primary Color): <br/>
              <input type="color" value={formColor} onChange={e => setFormColor(e.target.value)} style={{ width: '100%', height: '40px', marginTop: '5px', border: 'none', cursor: 'pointer'}} />
            </label>
            <button type="submit" style={{ backgroundColor: primaryColor, color: 'white', border: 'none', padding: '12px', borderRadius: '6px', cursor: 'pointer', fontWeight: 'bold' }}>
              Simpan Konfigurasi ke SQLite Kontainer
            </button>
            {saveStatus && <p style={{ textAlign: 'center', fontWeight: 'bold', color: primaryColor }}>{saveStatus}</p>}
          </form>
        </div>
      </main>
    </div>
  );
}

export default App;

