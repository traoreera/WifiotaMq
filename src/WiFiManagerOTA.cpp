// ============================================
// WiFiManagerOTA.cpp
// ============================================
#include "WiFiManagerOTA.h"
#include "WebPages.h"
#include "utilities.h"

// ✅ Fix: nom unifié — mqtt.h déclare "extern Logger logger"
extern Logger logger;

// ─────────────────────────────────────────────────────────
WiFiManagerOTA::WiFiManagerOTA(uint16_t port, const char *user, const char *pass)
    : server(port), otaUser(user), otaPass(pass)
{
    mqtt_config = {};
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::begin(String hostname, String apName, String apPassword)
{
    logger.info("╔═══════════════════════════════════╗");
    logger.info("║   WiFiManagerOTA Initialisation   ║");
    logger.info("╚═══════════════════════════════════╝");

    if (!connectToWiFi())
        startAccessPoint(apName, apPassword);

    setupRoutes();
    ElegantOTA.begin(&server, otaUser.c_str(), otaPass.c_str());
    server.begin();
    logger.info("Serveur web démarré");

    if (MDNS.begin(hostname.c_str()))
    {
        logger.info("mDNS actif: http://" + hostname + ".local");
        MDNS.addService("http", "tcp", 80);
    }

    logger.info("Accès — User: " + otaUser + "  Pass: " + otaPass);
    logger.info("═══════════════════════════════════");
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::loop()
{
    ElegantOTA.loop();

    // ✅ Fix: redémarrage différé — la réponse HTTP a le temps d'être envoyée
    if (pendingRestart && millis() >= restartScheduledAt)
    {
        logger.info("Redémarrage...");
        ESP.restart();
    }
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::scheduleRestart(unsigned long delayMs)
{
    pendingRestart = true;
    restartScheduledAt = millis() + delayMs;
    logger.info("Redémarrage programmé dans " + String(delayMs) + " ms");
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::handleWiFiReconnect()
{
    if (!wifi_connected)
        return;
    if (WiFi.status() == WL_CONNECTED)
        return;

    unsigned long now = millis();
    if (now - lastReconnectAttempt > 30000)
    {
        logger.info("Tentative de reconnexion WiFi...");
        lastReconnectAttempt = now;
        connectToWiFi(10, 500);
    }
}

// ─────────────────────────────────────────────────────────
bool WiFiManagerOTA::authenticate(AsyncWebServerRequest *request)
{
    if (!request->authenticate(otaUser.c_str(), otaPass.c_str()))
    {
        request->requestAuthentication();
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::loadConfig()
{
    prefs.begin("wifi_config", true);
    config.ssid = prefs.getString("ssid", "");
    config.password = prefs.getString("password", "");
    config.topic = prefs.getString("topic", "");
    config.user_id = prefs.getString("user_id", "");
    config.useStaticIP = prefs.getBool("useStaticIP", false);
    config.staticIP = prefs.getString("staticIP", "");
    config.subnet = prefs.getString("subnet", "255.255.255.0");
    config.gateway = prefs.getString("gateway", "");
    config.dns1 = prefs.getString("dns1", "8.8.8.8");
    config.dns2 = prefs.getString("dns2", "8.8.4.4");
    prefs.end();

    logger.info("Config WiFi — SSID: " + config.ssid +
                "  IP statique: " + String(config.useStaticIP ? "oui" : "non"));
}

// ─────────────────────────────────────────────────────────
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
        mqtt_config.port = 8883;

    logger.info("Config MQTT — " + mqtt_config.hostname + ":" + String(mqtt_config.port));
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::saveConfig()
{
    prefs.begin("wifi_config", false);
    prefs.putString("ssid", config.ssid);
    prefs.putString("password", config.password);
    prefs.putString("topic", config.topic);
    prefs.putString("user_id", config.user_id);
    prefs.putBool("useStaticIP", config.useStaticIP);
    prefs.putString("staticIP", config.staticIP);
    prefs.putString("subnet", config.subnet);
    prefs.putString("gateway", config.gateway);
    prefs.putString("dns1", config.dns1);
    prefs.putString("dns2", config.dns2);
    prefs.end();
    logger.info("Configuration WiFi sauvegardée");
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::saveMqttConfig()
{
    prefs.begin("mqtt_config", false);
    prefs.putString("hostname", mqtt_config.hostname);
    prefs.putInt("port", mqtt_config.port);
    prefs.putString("user", mqtt_config.user);
    prefs.putString("password", mqtt_config.password);
    prefs.putString("client", mqtt_config.client);
    prefs.end();
    logger.info("Configuration MQTT sauvegardée");
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::resetConfig()
{
    prefs.begin("wifi_config", false);
    prefs.clear();
    prefs.end();
    prefs.begin("mqtt_config", false);
    prefs.clear();
    prefs.end();
    logger.info("Configuration effacée");
}

// ─────────────────────────────────────────────────────────
bool WiFiManagerOTA::connectToWiFi(int maxAttempts, int delayMs)
{
    loadConfig();
    if (config.ssid.isEmpty() || config.password.isEmpty())
    {
        logger.error("Pas de configuration WiFi enregistrée");
        return false;
    }

    WiFi.mode(WIFI_STA);

    if (config.useStaticIP && !config.staticIP.isEmpty() && !config.gateway.isEmpty())
    {
        IPAddress ip, subnet, gateway, dns1, dns2;
        if (ip.fromString(config.staticIP) && subnet.fromString(config.subnet) &&
            gateway.fromString(config.gateway) && dns1.fromString(config.dns1) &&
            dns2.fromString(config.dns2))
        {
            WiFi.config(ip, gateway, subnet, dns1, dns2);
            logger.info("IP statique appliquée: " + config.staticIP);
        }
        else
        {
            logger.error("Adresses IP invalides — fallback DHCP");
        }
    }

    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    logger.info("Connexion à " + config.ssid + " ...");

    unsigned long deadline = millis() + (unsigned long)maxAttempts * delayMs;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline)
    {
        delay(delayMs);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        logger.info("Connecté ! IP: " + WiFi.localIP().toString() +
                    "  Signal: " + String(WiFi.RSSI()) + " dBm");
        wifi_connected = true;
        return true;
    }

    logger.error("Connexion WiFi échouée");
    wifi_connected = false;
    return false;
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::startAccessPoint(String apName, String password)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), password.c_str());
    logger.info("Point d'accès — SSID: " + apName +
                "  IP: " + WiFi.softAPIP().toString());
}

// ─────────────────────────────────────────────────────────
String WiFiManagerOTA::formatUptime()
{
    unsigned long s = millis() / 1000;
    unsigned long m = s / 60, h = m / 60, d = h / 24;

    if (d > 0)
        return String(d) + "j " + String(h % 24) + "h";
    if (h > 0)
        return String(h) + "h " + String(m % 60) + "m";
    if (m > 0)
        return String(m) + "m " + String(s % 60) + "s";
    return String(s) + "s";
}

// ─────────────────────────────────────────────────────────
String WiFiManagerOTA::pubTopic(String version)
{
    return config.topic + version + config.user_id;
}

String WiFiManagerOTA::cmdTopic(String version, String cmd)
{
    return config.topic + version + config.user_id + cmd;
}

// ─────────────────────────────────────────────────────────
WiFiManagerOTA::MQTTConfig WiFiManagerOTA::getMqttConfig()
{
    loadMqttConfig();
    return mqtt_config;
}

WiFiManagerOTA::WiFiConfigStruct WiFiManagerOTA::getWiFiConfig()
{
    loadConfig();
    return {config.ssid, config.password, config.useStaticIP,
            config.staticIP, config.subnet, config.gateway,
            config.dns1, config.dns2};
}

bool WiFiManagerOTA::hasValidConfig()
{
    MQTTConfig cfg = getMqttConfig();
    return cfg.hostname.length() > 0 && cfg.client.length() > 0 && cfg.port > 0;
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::handleConfigPage(AsyncWebServerRequest *request)
{
    String html = String(WebPages::CONFIG_HTML);

    // Scanner les réseaux
    int n = WiFi.scanNetworks();
    String networks;
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
            if (ssid == config.ssid)
                networks += " selected";
            networks += ">" + ssid + " (" + String(WiFi.RSSI(i)) + " dBm)";
            networks += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " [ouvert]" : " [chiffré]");
            networks += "</option>";
        }
    }

    // ✅ Fix: plus de bloc dupliqué
    html.replace("%NETWORKS%", networks);
    html.replace("%PASSWORD%", config.password);
    html.replace("%TOPIC%", config.topic);
    html.replace("%USER_ID%", config.user_id);
    html.replace("%USE_STATIC_IP%", config.useStaticIP ? "checked" : "");
    html.replace("%STATIC_IP%", config.staticIP);
    html.replace("%SUBNET%", config.subnet);
    html.replace("%GATEWAY%", config.gateway);
    html.replace("%DNS1%", config.dns1);
    html.replace("%DNS2%", config.dns2);

    request->send(200, "text/html", html);
    WiFi.scanDelete();
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::setupRoutes()
{
    // ── CSS partagé (mis en cache 24h) ───────────────────
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncWebServerResponse* resp = request->beginResponse_P(
            200, "text/css", WebPages::COMMON_CSS);
        resp->addHeader("Cache-Control", "public, max-age=86400");
        request->send(resp); });

    // ── Accueil ──────────────────────────────────────────
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;

        String page = String(WebPages::INDEX_HTML);
        page.replace("%SSID%",   WiFi.isConnected() ? config.ssid : "Non connecté");
        page.replace("%IP%",     WiFi.isConnected() ? WiFi.localIP().toString()
                                                     : WiFi.softAPIP().toString());
        page.replace("%RSSI%",   WiFi.isConnected() ? String(WiFi.RSSI()) : "N/A");
        page.replace("%UPTIME%", formatUptime());
        request->send(200, "text/html", page); });

    // ── Config WiFi ──────────────────────────────────────
    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;
        handleConfigPage(request); });

    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;

        if (!request->hasParam("ssid", true) || !request->hasParam("password", true))
        {
            request->send(400, "text/plain", "Paramètres manquants");
            return;
        }

        config.ssid        = request->getParam("ssid",        true)->value();
        config.password    = request->getParam("password",    true)->value();
        config.topic       = request->getParam("topic",       true)->value();
        config.user_id     = request->getParam("user_id",     true)->value();
        config.useStaticIP = request->hasParam("useStaticIP", true);
        config.staticIP    = request->getParam("staticIP",    true)->value();
        config.subnet      = request->getParam("subnet",      true)->value();
        config.gateway     = request->getParam("gateway",     true)->value();
        config.dns1        = request->getParam("dns1",        true)->value();
        config.dns2        = request->getParam("dns2",        true)->value();
        saveConfig();

        // ✅ Fix: réponse d'abord, redémarrage après
        request->send(200, "text/html",
            WebPages::successPage("Configuration WiFi enregistrée"));
        scheduleRestart(); });

    // ── Config MQTT ──────────────────────────────────────
    server.on("/mqtt", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;

        // ✅ Fix: pré-remplissage des valeurs MQTT existantes
        loadMqttConfig();
        String html = String(WebPages::MQTT_CONFIG_HTML);
        html.replace("%HOSTNAME%",   mqtt_config.hostname);
        html.replace("%PORT%",       String(mqtt_config.port));
        html.replace("%MQTT_USER%",  mqtt_config.user);
        html.replace("%MQTT_CLIENT%",mqtt_config.client);
        request->send(200, "text/html", html); });

    server.on("/saveMqtt", HTTP_POST, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;

        if (!request->hasParam("hostname", true) || !request->hasParam("port", true) ||
            !request->hasParam("user",     true) || !request->hasParam("password", true) ||
            !request->hasParam("client",   true))
        {
            request->send(400, "text/plain", "Paramètres manquants");
            return;
        }

        mqtt_config.hostname = request->getParam("hostname", true)->value();
        mqtt_config.port     = request->getParam("port",     true)->value().toInt();
        mqtt_config.user     = request->getParam("user",     true)->value();
        mqtt_config.password = request->getParam("password", true)->value();
        mqtt_config.client   = request->getParam("client",   true)->value();
        saveMqttConfig();

        request->send(200, "text/html",
            WebPages::successPage("Configuration MQTT enregistrée"));
        scheduleRestart(); });

    // ── Status JSON ──────────────────────────────────────
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;

        String json = "{";
        json += "\"ssid\":\""      + String(WiFi.SSID())             + "\",";
        json += "\"ip\":\""        + WiFi.localIP().toString()        + "\",";
        json += "\"rssi\":"        + String(WiFi.RSSI())              + ",";
        json += "\"uptime\":\""    + formatUptime()                   + "\",";
        json += "\"freeHeap\":"    + String(ESP.getFreeHeap())        + ",";
        json += "\"chipModel\":\"" + String(ESP.getChipModel())       + "\",";
        json += "\"cpuFreq\":"     + String(ESP.getCpuFreqMHz())      + ",";
        json += "\"mqttHost\":\"" + mqtt_config.hostname              + "\"";
        json += "}";
        request->send(200, "application/json", json); });

    // ── Reset ────────────────────────────────────────────
    server.on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;
        resetConfig();
        request->send(200, "text/html",
            WebPages::successPage("Configuration effacée"));
        scheduleRestart(); });

    // ── Reboot ───────────────────────────────────────────
    server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
        if (!authenticate(request)) return;
        request->send(200, "text/html",
            WebPages::successPage("Redémarrage en cours…"));
        scheduleRestart(); });
}

// ─────────────────────────────────────────────────────────
void WiFiManagerOTA::setLogger(bool active)
{
    logger.setLogger(active);
}