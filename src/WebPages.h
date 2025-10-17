// ============================================
// WebPages.h - HTML templates
// ============================================
#ifndef WEB_PAGES_H
#define WEB_PAGES_H

namespace WebPages {
  const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 - Accueil</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container { 
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px;
    }
    h1 { color: #333; margin-bottom: 10px; font-size: 28px; text-align: center; }
    .subtitle { text-align: center; color: #666; margin-bottom: 30px; font-size: 14px; }
    .info-card {
      background: #f8f9fa;
      border-radius: 10px;
      padding: 20px;
      margin-bottom: 25px;
      border-left: 4px solid #667eea;
    }
    .info-item {
      display: flex;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid #e0e0e0;
    }
    .info-item:last-child { border-bottom: none; }
    .info-label { color: #666; font-weight: 500; }
    .info-value { color: #333; font-weight: 600; }
    .nav-links {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 12px;
    }
    .nav-link {
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      text-decoration: none;
      border-radius: 10px;
      transition: all 0.3s ease;
      font-weight: 500;
      box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    }
    .nav-link:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
    }
    .nav-link.danger {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
    }
    .emoji { margin-right: 8px; font-size: 18px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🌐 ESP32 Manager</h1>
    <p class="subtitle">Gestion WiFi & OTA</p>
    <div class="info-card">
      <div class="info-item">
        <span class="info-label">SSID</span>
        <span class="info-value">%SSID%</span>
      </div>
      <div class="info-item">
        <span class="info-label">🌍 IP</span>
        <span class="info-value">%IP%</span>
      </div>
      <div class="info-item">
        <span class="info-label">📊 Signal</span>
        <span class="info-value">%RSSI% dBm</span>
      </div>
      <div class="info-item">
        <span class="info-label">⏱️ Uptime</span>
        <span class="info-value">%UPTIME%</span>
      </div>
    </div>
    <div class="nav-links">
      <a href="/config" class="nav-link"><span class="emoji">⚙️</span> WiFi</a>
      <a href="/mqtt" class="nav-link"><span class="emoji">📡</span> MQTT</a>
      <a href="/update" class="nav-link"><span class="emoji">⬆️</span> OTA</a>
      <a href="/reboot" class="nav-link"><span class="emoji">🔄</span> Reboot</a>
      <a href="/reset" class="nav-link danger"><span class="emoji">❌</span> Reset</a>
      <a href="/status" class="nav-link"><span class="emoji">📊</span> Status</a>
    </div>
  </div>
</body>
</html>
  )rawliteral";

  const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configuration WiFi</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container { 
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px;
    }
    h1 { color: #333; margin-bottom: 30px; text-align: center; }
    .form-group { margin-bottom: 20px; }
    label { display: block; margin-bottom: 8px; color: #555; font-weight: 500; }
    select, input { 
      width: 100%;
      padding: 12px;
      border: 2px solid #e0e0e0;
      border-radius: 8px;
      font-size: 14px;
      transition: border-color 0.3s;
    }
    select:focus, input:focus {
      outline: none;
      border-color: #667eea;
    }
    button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s;
    }
    button:hover { transform: translateY(-2px); }
    .back-link { display: block; text-align: center; margin-top: 20px; color: #667eea; text-decoration: none; }
  </style>
</head>
<body>
  <div class="container">
    <h1>⚙️ Configuration WiFi</h1>
    <form method="POST" action="/save">
      <div class="form-group">
        <label for="ssid">Réseau WiFi</label>
        <select name="ssid" id="ssid" required>
          %NETWORKS%
        </select>
      </div>
      <div class="form-group">
        <label for="password">🔒 Mot de passe</label>
        <input type="password" name="password" id="password" value="%PASSWORD%" required>
      </div>
      <div class="form-group">
        <label for="topic">📬 Topic MQTT</label>
        <input type="text" name="topic" id="topic" value="%TOPIC%" placeholder="home/sensor/" required>
      </div>
      <div class="form-group">
        <label for="user_id">👤 User ID</label>
        <input type="text" name="user_id" id="user_id" value="%USER_ID%" placeholder="device001" required>
      </div>
      <button type="submit">💾 Enregistrer et Redémarrer</button>
    </form>
    <a href="/" class="back-link">← Retour</a>
  </div>
</body>
</html>
  )rawliteral";

  const char MQTT_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configuration MQTT</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container { 
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px;
    }
    h1 { color: #333; margin-bottom: 30px; text-align: center; }
    .form-group { margin-bottom: 20px; }
    label { display: block; margin-bottom: 8px; color: #555; font-weight: 500; }
    input { 
      width: 100%;
      padding: 12px;
      border: 2px solid #e0e0e0;
      border-radius: 8px;
      font-size: 14px;
      transition: border-color 0.3s;
    }
    input:focus {
      outline: none;
      border-color: #667eea;
    }
    button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s;
    }
    button:hover { transform: translateY(-2px); }
    .back-link { display: block; text-align: center; margin-top: 20px; color: #667eea; text-decoration: none; }
  </style>
</head>
<body>
  <div class="container">
    <h1>📡 Configuration MQTT</h1>
    <form method="POST" action="/saveMqtt">
      <div class="form-group">
        <label for="hostname">🌐 Hostname</label>
        <input type="text" name="hostname" id="hostname" placeholder="mqtt.example.com" required>
      </div>
      <div class="form-group">
        <label for="port">🔌 Port</label>
        <input type="number" name="port" id="port" value="8883" min="1" max="65535" required>
      </div>
      <div class="form-group">
        <label for="user">👤 Utilisateur</label>
        <input type="text" name="user" id="user" placeholder="mqtt_user" required>
      </div>
      <div class="form-group">
        <label for="password">🔒 Mot de passe</label>
        <input type="password" name="password" id="password" required>
      </div>
      <div class="form-group">
        <label for="client">📱 Client ID</label>
        <input type="text" name="client" id="client" placeholder="ESP32_Client" required>
      </div>
      <button type="submit">💾 Enregistrer et Redémarrer</button>
    </form>
    <a href="/" class="back-link">← Retour</a>
  </div>
</body>
</html>
  )rawliteral";
}

#endif