
// ============================================
// WiFiManagerOTA.cpp
// ============================================
#include "WiFiManagerOTA.h"
#include "WebPages.h"
#include "utilities.h"
Logger logs;

WiFiManagerOTA::WiFiManagerOTA(uint16_t port, const char *user, const char *pass)
    : server(port), otaUser(user), otaPass(pass), lastReconnectAttempt(0)
{
    mqtt_config = {.hostname = "", .port = 8883, .user = "", .password = "", .client = ""};
}

void WiFiManagerOTA::begin(String hostname, String apName, String apPassword)
{
    logs.info("╔═══════════════════════════════════╗");
    logs.info("║   WiFiManagerOTA Initialisation   ║");
    logs.info("╚═══════════════════════════════════╝");

    if (!connectToWiFi())
    {
        startAccessPoint(apName, apPassword);
    }

    setupRoutes();
    ElegantOTA.begin(&server, otaUser.c_str(), otaPass.c_str());
    server.begin();

    logs.info("Serveur web démarré");

    if (MDNS.begin(hostname.c_str()))
    {
        logs.info("mDNS actif: http://" + hostname + ".local");
        MDNS.addService("http", "tcp", 80);
    }

    logs.info("Accès:");
    logs.info("\t\tUser: " + otaUser);
    logs.info("\t\tPass: " + otaPass);
    logs.info("═══════════════════════════════════");
}

void WiFiManagerOTA::loop()
{
    ElegantOTA.loop();
}

void WiFiManagerOTA::handleWiFiReconnect()
{
    if (WiFi.status() != WL_CONNECTED && wifi_connected)
    {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 30000)
        {
            logs.info("Tentative de reconnexion WiFi...");
            lastReconnectAttempt = now;
            connectToWiFi(10, 500);
        }
    }
}

void WiFiManagerOTA::loadConfig()
{
    prefs.begin("wifi_config", true);
    config.ssid = prefs.getString("ssid", "");
    config.password = prefs.getString("password", "");
    config.topic = prefs.getString("topic", "");
    config.user_id = prefs.getString("user_id", "");
    prefs.end();

    logs.info("Configuration WiFi chargée:");
    logs.info("  SSID: " + config.ssid);
    logs.info("  Topic: " + config.topic);
    logs.info("  User ID: " + config.user_id);
}

void WiFiManagerOTA::loadMqttConfig()
{
    prefs.begin("mqtt_config", true);
    mqtt_config.hostname = prefs.getString("hostname", "");
    mqtt_config.port = prefs.getInt("port", 8883);
    mqtt_config.user = prefs.getString("user", "");
    mqtt_config.password = prefs.getString("password", "");
    mqtt_config.client = prefs.getString("client", "");
    prefs.end();

    if (mqtt_config.port < 1 || mqtt_config.port > 65535)
    {
        mqtt_config.port = 8883;
    }

    logs.info("Configuration MQTT chargée:");
    logs.info("  Hostname: " + mqtt_config.hostname);
    logs.info("  Port: " + String(mqtt_config.port));
    logs.info("  Client: " + mqtt_config.client);
}

void WiFiManagerOTA::saveConfig()
{
    prefs.begin("wifi_config", false);
    prefs.putString("ssid", config.ssid);
    prefs.putString("password", config.password);
    prefs.putString("topic", config.topic);
    prefs.putString("user_id", config.user_id);
    prefs.end();
    logs.info("Configuration WiFi sauvegardée");
}

void WiFiManagerOTA::saveMqttConfig()
{
    prefs.begin("mqtt_config", false);
    prefs.putString("hostname", mqtt_config.hostname);
    prefs.putInt("port", mqtt_config.port);
    prefs.putString("user", mqtt_config.user);
    prefs.putString("password", mqtt_config.password);
    prefs.putString("client", mqtt_config.client);
    prefs.end();
    logs.info("Configuration MQTT sauvegardée");
}

void WiFiManagerOTA::resetConfig()
{
    prefs.begin("wifi_config", false);
    prefs.clear();
    prefs.end();

    prefs.begin("mqtt_config", false);
    prefs.clear();
    prefs.end();

    logs.info("Configuration effacée");
}

bool WiFiManagerOTA::connectToWiFi(int maxAttempts, int delayMs)
{
    loadConfig();
    if (config.ssid == "" || config.password == "")
    {
        logs.error("Pas de configuration WiFi");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    logs.info("Connexion à " + config.ssid);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < (maxAttempts * delayMs))
    {
        logs.info(".");
        delay(delayMs);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        logs.info("Connecté !");
        logs.info("  IP: " + WiFi.localIP().toString());
        logs.info("  Signal: " + String(WiFi.RSSI()) + " dBm");
        wifi_connected = true;
        return true;
    }

    logs.critical("Connexion échouée");
    wifi_connected = false;
    return false;
}

void WiFiManagerOTA::startAccessPoint(String apName, String password)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), password.c_str());
    logs.info("Point d'accès démarré");
    logs.info("  SSID: " + apName);
    logs.info("  IP: " + WiFi.softAPIP().toString());
    logs.info("  Mot de passe: " + password);
}

String WiFiManagerOTA::formatUptime()
{
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;

    if (days > 0)
        return String(days) + "j " + String(hours % 24) + "h";
    if (hours > 0)
        return String(hours) + "h " + String(minutes % 60) + "m";
    if (minutes > 0)
        return String(minutes) + "m " + String(seconds % 60) + "s";
    return String(seconds) + "s";
}

String WiFiManagerOTA::pubTopic(String version)
{
    String full = config.topic + version + config.user_id;
    logs.info("Topic publication: " + full);
    return full;
}

String WiFiManagerOTA::cmdTopic(String version, String cmd)
{
    String full = config.topic + version + config.user_id + cmd;
    logs.info("Topic commande: " + full);
    return full;
}

WiFiManagerOTA::MQTTConfig WiFiManagerOTA::getMqttConfig()
{
    loadMqttConfig();
    return mqtt_config;
}

WiFiManagerOTA::WiFiConfigStruct WiFiManagerOTA::getWiFiConfig()
{
    loadConfig();
    WiFiConfigStruct wifi;
    wifi.ssid = config.ssid;
    wifi.password = config.password;
    return wifi;
}

bool WiFiManagerOTA::hasValidConfig()
{
    MQTTConfig cfg = getMqttConfig();
    return (cfg.hostname.length() > 0 && cfg.client.length() > 0 && cfg.port > 0);
}

void WiFiManagerOTA::handleConfigPage(AsyncWebServerRequest *request)
{
    String html = String(WebPages::CONFIG_HTML);

    int n = WiFi.scanNetworks();
    String networks = "";
    String currentSSID = config.ssid;

    if (n == 0)
    {
        networks = "<option value=''>Aucun réseau trouvé</option>";
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            String ssid = WiFi.SSID(i);
            networks += "<option value='" + ssid + "'";
            if (ssid == currentSSID)
                networks += " selected";
            networks += ">" + ssid + " (" + String(WiFi.RSSI(i)) + " dBm)";
            networks += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " 🔓" : " 🔒");
            networks += "</option>";
        }
    }

    html.replace("%NETWORKS%", networks);
    html.replace("%PASSWORD%", config.password);
    html.replace("%TOPIC%", config.topic);
    html.replace("%USER_ID%", config.user_id);

    request->send(200, "text/html", html);
    WiFi.scanDelete();
}

void WiFiManagerOTA::setupRoutes()
{
    // Home page
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    
    String page = String(WebPages::INDEX_HTML);
    page.replace("%SSID%", WiFi.isConnected() ? config.ssid : "Non connecté");
    page.replace("%IP%", WiFi.isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
    page.replace("%RSSI%", WiFi.isConnected() ? String(WiFi.RSSI()) : "N/A");
    page.replace("%UPTIME%", formatUptime());
    
    request->send(200, "text/html", page); });

    // WiFi config page
    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    handleConfigPage(request); });

    // Save WiFi config
    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      config.ssid = request->getParam("ssid", true)->value();
      config.password = request->getParam("password", true)->value();
      config.topic = request->getParam("topic", true)->value();
      config.user_id = request->getParam("user_id", true)->value();
      saveConfig();
      
      request->send(200, "text/html", 
        "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/'>"
        "<meta charset='UTF-8'></head><body style='font-family:Arial;text-align:center;padding:50px;'>"
        "<h2>✅ Configuration enregistrée</h2><p>Redémarrage dans 3 secondes...</p></body></html>");
      
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "⚠️ Paramètres manquants");
    } });

    // MQTT config page
    server.on("/mqtt", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    request->send(200, "text/html", WebPages::MQTT_CONFIG_HTML); });

    // Save MQTT config
    server.on("/saveMqtt", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    
    if (request->hasParam("hostname", true) && request->hasParam("port", true) &&
        request->hasParam("user", true) && request->hasParam("password", true) &&
        request->hasParam("client", true)) {
      
      mqtt_config.hostname = request->getParam("hostname", true)->value();
      mqtt_config.port = request->getParam("port", true)->value().toInt();
      mqtt_config.user = request->getParam("user", true)->value();
      mqtt_config.password = request->getParam("password", true)->value();
      mqtt_config.client = request->getParam("client", true)->value();
      saveMqttConfig();
      
      request->send(200, "text/html",
        "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/'>"
        "<meta charset='UTF-8'></head><body style='font-family:Arial;text-align:center;padding:50px;'>"
        "<h2>✅ Configuration MQTT enregistrée</h2><p>Redémarrage dans 3 secondes...</p></body></html>");
      
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "⚠️ Paramètres manquants");
    } });

    // Status page
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    
    String json = "{";
    json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + formatUptime() + "\",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz());
    json += "}";
    
    request->send(200, "application/json", json); });

    // Reset config
    server.on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
      return request->requestAuthentication();
    }
    
    resetConfig();
    request->send(200, "text/html",
        "<!DOCTYPE html><html><head><meta charset='UTF-8'></head>"
        "<body style='font-family:Arial;text-align:center;padding:50px;'>"
        "<h2>⚠️ Configuration effacée</h2><p>Redémarrage...</p></body></html>");
    
    delay(1000);
    ESP.restart(); });

    // Reboot
    server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request){
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str())) {
        return request->requestAuthentication();
    }
    
    request->send(200, "text/html",
        "<!DOCTYPE html><html><head><meta charset='UTF-8'></head>"
        "<body style='font-family:Arial;text-align:center;padding:50px;'>"
        "<h2>🔄 Redémarrage en cours...</h2></body></html>");
    
    delay(1000);
    ESP.restart(); });
}

void WiFiManagerOTA::setLogger(bool active){
    logs.settLogger(active);
}