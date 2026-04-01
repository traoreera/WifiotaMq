// ============================================
// WebPages.h - HTML templates
// ============================================
#ifndef WEB_PAGES_H
#define WEB_PAGES_H

namespace WebPages
{
  // ── CSS partagé (servi via /style.css) ──────────────────
  // Avantage : ~4 KB de PROGMEM économisé, mis en cache par le navigateur
  const char COMMON_CSS[] PROGMEM = R"css(
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;
  background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);
  min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}
.container{background:white;border-radius:20px;
  box-shadow:0 20px 60px rgba(0,0,0,.3);max-width:500px;width:100%;padding:40px}
h1{color:#333;margin-bottom:10px;font-size:28px;text-align:center}
.subtitle{text-align:center;color:#666;margin-bottom:30px;font-size:14px}
.info-card{background:#f8f9fa;border-radius:10px;padding:20px;
  margin-bottom:25px;border-left:4px solid #667eea}
.info-item{display:flex;justify-content:space-between;
  padding:8px 0;border-bottom:1px solid #e0e0e0}
.info-item:last-child{border-bottom:none}
.info-label{color:#666;font-weight:500}
.info-value{color:#333;font-weight:600}
.nav-links{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}
.nav-link{display:flex;align-items:center;justify-content:center;padding:15px;
  background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;
  text-decoration:none;border-radius:10px;transition:all .3s ease;font-weight:500;
  box-shadow:0 4px 15px rgba(102,126,234,.4)}
.nav-link:hover{transform:translateY(-2px);box-shadow:0 6px 20px rgba(102,126,234,.6)}
.nav-link.danger{background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%)}
.emoji{margin-right:8px;font-size:18px}
.form-group{margin-bottom:20px}
label{display:block;margin-bottom:8px;color:#555;font-weight:500}
select,input[type=text],input[type=password],input[type=number]{
  width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;
  font-size:14px;transition:border-color .3s}
select:focus,input:focus{outline:none;border-color:#667eea}
button,input[type=submit]{width:100%;padding:15px;
  background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;
  border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;
  transition:transform .2s}
button:hover{transform:translateY(-2px)}
.back-link{display:block;text-align:center;margin-top:20px;
  color:#667eea;text-decoration:none}
  )css";

  // ── Page d'accueil ────────────────────────────────────
  const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>TSP-Manager</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <h1>&#127760; Tanga System Portal Manager</h1>
    <p class="subtitle">Gestion WiFi &amp; OTA</p>
    <div class="info-card">
      <div class="info-item">
        <span class="info-label">SSID</span>
        <span class="info-value">%SSID%</span>
      </div>
      <div class="info-item">
        <span class="info-label">&#127757; IP</span>
        <span class="info-value">%IP%</span>
      </div>
      <div class="info-item">
        <span class="info-label">&#128202; Signal</span>
        <span class="info-value">%RSSI% dBm</span>
      </div>
      <div class="info-item">
        <span class="info-label">&#9203; Uptime</span>
        <span class="info-value">%UPTIME%</span>
      </div>
    </div>
    <div class="nav-links">
      <a href="/config"  class="nav-link"><span class="emoji">&#9881;</span> WiFi</a>
      <a href="/mqtt"    class="nav-link"><span class="emoji">&#128225;</span> MQTT</a>
      <a href="/update"  class="nav-link"><span class="emoji">&#11014;</span> OTA</a>
      <a href="/reboot"  class="nav-link"><span class="emoji">&#128260;</span> Reboot</a>
      <a href="/reset"   class="nav-link danger"><span class="emoji">&#10060;</span> Reset</a>
      <a href="/status"  class="nav-link"><span class="emoji">&#128202;</span> Status</a>
    </div>
  </div>
</body>
</html>
  )rawliteral";

  // ── Configuration WiFi ────────────────────────────────
  const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>TSPM-Config WiFi</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <h1>&#9881; Configuration WiFi</h1>
    <form method="POST" action="/save">
      <div class="form-group">
        <label>R&eacute;seau WiFi</label>
        <select name="ssid" required>%NETWORKS%</select>
      </div>
      <div class="form-group">
        <label>&#128274; Mot de passe</label>
        <input type="password" name="password" value="%PASSWORD%" required>
      </div>
      <div class="form-group">
        <label>&#128172; Topic MQTT</label>
        <input type="text" name="topic" value="%TOPIC%" placeholder="home/sensor/" required>
      </div>
      <div class="form-group">
        <label>&#128100; User ID</label>
        <input type="text" name="user_id" value="%USER_ID%" placeholder="device001" required>
      </div>
      <div class="form-group">
        <label>
          <input type="checkbox" name="useStaticIP" %USE_STATIC_IP%> Utiliser IP statique
        </label>
      </div>
      <div class="form-group">
        <label>&#127760; IP statique</label>
        <input type="text" name="staticIP" value="%STATIC_IP%" placeholder="192.168.1.100"
          pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
      </div>
      <div class="form-group">
        <label>&#128207; Masque sous-r&eacute;seau</label>
        <input type="text" name="subnet" value="%SUBNET%" placeholder="255.255.255.0"
          pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
      </div>
      <div class="form-group">
        <label>&#128682; Passerelle</label>
        <input type="text" name="gateway" value="%GATEWAY%" placeholder="192.168.1.1"
          pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
      </div>
      <div class="form-group">
        <label>&#128269; DNS Primaire</label>
        <input type="text" name="dns1" value="%DNS1%" placeholder="8.8.8.8"
          pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
      </div>
      <div class="form-group">
        <label>&#128269; DNS Secondaire</label>
        <input type="text" name="dns2" value="%DNS2%" placeholder="8.8.4.4"
          pattern="^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$">
      </div>
      <button type="submit">&#128190; Enregistrer et Red&eacute;marrer</button>
    </form>
    <a href="/" class="back-link">&larr; Retour</a>
  </div>
</body>
</html>
  )rawliteral";

  // ── Configuration MQTT ────────────────────────────────
  // ✅ Fix: pré-remplit les valeurs existantes (%HOSTNAME%, %PORT%, etc.)
  const char MQTT_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>TSPM-Config MQTT</title>
  <link rel="stylesheet" href="/style.css">
</head>
<body>
  <div class="container">
    <h1>&#128225; Configuration MQTT</h1>
    <form method="POST" action="/saveMqtt">
      <div class="form-group">
        <label>&#127760; Hostname</label>
        <input type="text" name="hostname" value="%HOSTNAME%" placeholder="mqtt.example.com" required>
      </div>
      <div class="form-group">
        <label>&#128268; Port</label>
        <input type="number" name="port" value="%PORT%" min="1" max="65535" required>
      </div>
      <div class="form-group">
        <label>&#128100; Utilisateur</label>
        <input type="text" name="user" value="%MQTT_USER%" placeholder="mqtt_user" required>
      </div>
      <div class="form-group">
        <label>&#128274; Mot de passe</label>
        <input type="password" name="password" required>
      </div>
      <div class="form-group">
        <label>&#128241; Client ID</label>
        <input type="text" name="client" value="%MQTT_CLIENT%" placeholder="ESP32_Client" required>
      </div>
      <button type="submit">&#128190; Enregistrer et Red&eacute;marrer</button>
    </form>
    <a href="/" class="back-link">&larr; Retour</a>
  </div>
</body>
</html>
  )rawliteral";

  // ── Page de succès réutilisable ───────────────────────
  inline String successPage(const String &message)
  {
    return String(
               "<!DOCTYPE html><html><head>"
               "<meta http-equiv='refresh' content='3;url=/'>"
               "<meta charset='UTF-8'>"
               "<link rel='stylesheet' href='/style.css'>"
               "</head><body style='font-family:Arial;text-align:center;padding:50px'>"
               "<h2>&#9989; ") +
           message +
           "</h2><p>Retour dans 3 secondes&hellip;</p></body></html>";
  }
}

#endif