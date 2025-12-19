
// ============================================
// WiFiManagerOTA.cpp
// ============================================
#include "WiFiManagerOTA.h"
#include "WebPages.h"
#include "utilities.h"
Logger logs;

/**
 * Constructeur de la classe WiFiManagerOTA.
 *
 * @param port Numéro de port utilisé par le serveur web.
 * @param user Nom d'utilisateur pour l accès OTA.
 * @param pass Mot de passe pour l accès OTA.


/**
 * Initialise le gestionnaire WiFiManagerOTA.
 *
 * @param hostname Nom de domaine local pour l'accès mDNS.
 * @param apName Nom du point d accès WiFi.
 * @param apPassword Mot de passe du point d accès WiFi.
 */

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

/**
 * Boucle d'exécution de la classe WiFiManagerOTA.
 *
 * Cette fonction est appelée en boucle pour gérer les événements
 * liés au serveur web et au système d'accès OTA.
 */
void WiFiManagerOTA::loop()
{
    ElegantOTA.loop();
}

/**
 * Gère la reconnexion WiFi en cas de perte de connexion.
 * Cette fonction est appelée en boucle pour gérer les événements
 * liés au système de connexion WiFi.
 */
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

/**
 * Charge la configuration WiFi enregistrée dans les préférences.
 *
 * Cette fonction charge les paramètres de connexion WiFi (SSID, mot de passe,
 * topic MQTT et user ID) enregistrés dans les préférences du système.
 *
 * Les paramètres sont stockés dans le namespace "wifi_config".
 */
void WiFiManagerOTA::loadConfig()
{
    prefs.begin("wifi_config", true);
    config.ssid = prefs.getString("ssid", "");
    config.password = prefs.getString("password", "");
    config.topic = prefs.getString("topic", "");
    config.user_id = prefs.getString("user_id", "");
    config.useStaticIP = prefs.getBool("useStaticIP", false);   // Nouveau
    config.staticIP = prefs.getString("staticIP", "");          // Nouveau
    config.subnet = prefs.getString("subnet", "255.255.255.0"); // Nouveau (défaut /24)
    config.gateway = prefs.getString("gateway", "");            // Nouveau
    config.dns1 = prefs.getString("dns1", "8.8.8.8");           // Nouveau (Google DNS)
    config.dns2 = prefs.getString("dns2", "8.8.4.4");           // Nouveau (Google DNS secondaire)
    prefs.end();

    logs.info("Configuration WiFi chargée:");
    logs.info("  SSID: " + config.ssid);
    logs.info("  Topic: " + config.topic);
    logs.info("  User ID: " + config.user_id);
    logs.info(String("  IP Statique: ") + (config.useStaticIP ? "Activé" : "Désactivé"));
    if (config.useStaticIP)
    {
        logs.info("  IP: " + config.staticIP);
        logs.info("  Subnet: " + config.subnet);
        logs.info("  Gateway: " + config.gateway);
        logs.info("  DNS1: " + config.dns1);
        logs.info("  DNS2: " + config.dns2);
    }
}

/**
 * Charge la configuration MQTT enregistrée dans les préférences.
 *
 * Cette fonction charge les paramètres de connexion MQTT (hostname, port, utilisateur,
 * mot de passe et client ID) enregistrés dans les préférences du système.
 *
 * Les paramètres sont stockés dans le namespace "mqtt_config".
 */
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

/**
 * Sauvegarde la configuration WiFi actuelle dans les préférences.
 *
 * Cette fonction sauvegarde les paramètres de connexion WiFi (SSID, mot de passe, topic
 * et user ID) actuels dans les préférences du système.
 */
void WiFiManagerOTA::saveConfig()
{
    prefs.begin("wifi_config", false);
    prefs.putString("ssid", config.ssid);
    prefs.putString("password", config.password);
    prefs.putString("topic", config.topic);
    prefs.putString("user_id", config.user_id);
    prefs.putBool("useStaticIP", config.useStaticIP); // Nouveau
    prefs.putString("staticIP", config.staticIP);     // Nouveau
    prefs.putString("subnet", config.subnet);         // Nouveau
    prefs.putString("gateway", config.gateway);       // Nouveau
    prefs.putString("dns1", config.dns1);             // Nouveau
    prefs.putString("dns2", config.dns2);             // Nouveau
    prefs.end();
    logs.info("Configuration WiFi sauvegardée");
}
/**
 * Sauvegarde la configuration MQTT actuelle dans les préférences.
 *
 * Cette fonction sauvegarde les paramètres de connexion MQTT (hostname, port, utilisateur,
 * mot de passe et client ID) actuels dans les préférences du système.
 */
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

/**
 * Efface la configuration WiFi et MQTT actuelle.
 *
 * Cette fonction efface les paramètres de connexion WiFi et MQTT actuels
 * dans les préférences du système.
 */
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

/**
 * Connexion à un réseau WiFi.
 *
 * Cette fonction charge la configuration WiFi enregistrée dans les préférences
 * et tente de se connecter au réseau WiFi spécifié.
 *
 * @param maxAttempts Nombre de tentatives de connexion maximum.
 * @param delayMs Délai entre chaque tentative de connexion (en ms).
 * @return true si la connexion est réussie, false sinon.
 */
bool WiFiManagerOTA::connectToWiFi(int maxAttempts, int delayMs)
{
    loadConfig();
    if (config.ssid == "" || config.password == "")
    {
        logs.error("Pas de configuration WiFi");
        return false;
    }

    WiFi.mode(WIFI_STA);
    if (config.useStaticIP && config.staticIP != "" && config.gateway != "")
    {
        IPAddress ip, subnet, gateway, dns1, dns2;
        if (ip.fromString(config.staticIP) && subnet.fromString(config.subnet) &&
            gateway.fromString(config.gateway) && dns1.fromString(config.dns1) && dns2.fromString(config.dns2))
        {
            WiFi.config(ip, gateway, subnet, dns1, dns2);
            logs.info("Configuration IP statique appliquée");
        }
        else
        {
            logs.error("Adresses IP statiques invalides, utilisation DHCP");
        }
    }
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
/**
 * Demarrage d'un point d'accès WiFi.
 *
 * Cette fonction demarre un point d'accès WiFi avec le nom et le mot de passe fournis.
 *
 * @param apName Nom du point d'accès WiFi.
 * @param password Mot de passe du point d'accès WiFi.
 */
void WiFiManagerOTA::startAccessPoint(String apName, String password)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), password.c_str());
    logs.info("Point d'accès démarré");
    logs.info("  SSID: " + apName);
    logs.info("  IP: " + WiFi.softAPIP().toString());
    logs.info("  Mot de passe: " + password);
}

/**
 * Formatte le temps écoulé depuis le démarrage du système en une chaîne de caractères lisible.
 *
 * La fonction renvoie une chaîne de caractères au format "Xj Yh Zm" ou "Xh Ym Zs" suivant le temps écoulé.
 *
 * @return une chaîne de caractères représentant le temps écoulé.
 */
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

/**
 * Génère le topic de publication pour une version de firmware.
 *
 * @param version Version de firmware (par exemple "v1.0.0").
 * @return Le topic de publication complet (par exemple "home/sensor/v1.0.0/device001").
 */
String WiFiManagerOTA::pubTopic(String version)
{
    String full = config.topic + version + config.user_id;
    logs.info("Topic publication: " + full);
    return full;
}

/**
 * Génère le topic de commande pour une version de firmware.
 *
 * @param version Version de firmware (par exemple "v1.0.0").
 * @param cmd Commande à envoyer (par exemple "restart").
 * @return Le topic de commande complet (par exemple "home/sensor/v1.0.0/device001/restart").
 */
String WiFiManagerOTA::cmdTopic(String version, String cmd)
{
    String full = config.topic + version + config.user_id + cmd;
    logs.info("Topic commande: " + full);
    return full;
}

/**
 * Charge la configuration MQTT enregistrée dans les préférences.
 *
 * Cette fonction charge les paramètres de connexion MQTT (hostname, port, utilisateur,
 * mot de passe et client ID) enregistrés dans les préférences du système.
 *
 * Les paramètres sont stockés dans le namespace "mqtt_config".
 *
 * @return La configuration MQTT actuelle.
 */
WiFiManagerOTA::MQTTConfig WiFiManagerOTA::getMqttConfig()
{
    loadMqttConfig();
    return mqtt_config;
}

/**
 * Récupère la configuration WiFi actuelle.
 *
 * Cette fonction charge la configuration WiFi actuelle enregistrée dans les préférences
 * et la renvoie sous la forme d'une structure WiFiConfigStruct.
 *
 * @return La configuration WiFi actuelle.
 */
WiFiManagerOTA::WiFiConfigStruct WiFiManagerOTA::getWiFiConfig()
{
    loadConfig();
    WiFiConfigStruct wifi;
    wifi.ssid = config.ssid;
    wifi.password = config.password;
    wifi.useStaticIP = config.useStaticIP;
    wifi.staticIP = config.staticIP;
    wifi.subnet = config.subnet;
    wifi.gateway = config.gateway;
    wifi.dns1 = config.dns1;
    wifi.dns2 = config.dns2;
    return wifi;
}

/**
 * Vérifie si la configuration MQTT actuelle est valide.
 *
 * Une configuration MQTT est considérée comme valide si elle contient un hostname, un client ID
 * et un port non vides.
 *
 * @return true si la configuration MQTT actuelle est valide, false sinon.
 */
bool WiFiManagerOTA::hasValidConfig()
{
    MQTTConfig cfg = getMqttConfig();
    return (cfg.hostname.length() > 0 && cfg.client.length() > 0 && cfg.port > 0);
}

/**
 * Gère la page de configuration WiFi.
 *
 * Cette fonction est appelée lorsque l'utilisateur accède à la page de configuration WiFi.
 * Elle scanne les réseaux WiFi disponibles et génère une page HTML avec les paramètres actuels.
 *
 * @param request La requête HTTP reçue.
 */
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
    html.replace("%NETWORKS%", networks);
    html.replace("%PASSWORD%", config.password);
    html.replace("%TOPIC%", config.topic);
    html.replace("%USER_ID%", config.user_id);
    html.replace("%USE_STATIC_IP%", config.useStaticIP ? "checked" : ""); // Nouveau
    html.replace("%STATIC_IP%", config.staticIP);                         // Nouveau
    html.replace("%SUBNET%", config.subnet);                              // Nouveau
    html.replace("%GATEWAY%", config.gateway);                            // Nouveau
    html.replace("%DNS1%", config.dns1);                                  // Nouveau
    html.replace("%DNS2%", config.dns2);                                  // Nouveau

    request->send(200, "text/html", html);
    WiFi.scanDelete();
}

/**
 * Configure les routes de l'API OTA.
 *
 * Cette fonction configure les différentes routes de l'API OTA, notamment pour
 * la page d'accueil, la page de configuration WiFi, la page de configuration MQTT,
 * la page de status et la page de reboot.
 */
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
        config.useStaticIP = request->hasParam("useStaticIP", true);  // Nouveau : case cochée ?
        config.staticIP = request->getParam("staticIP", true)->value();  // Nouveau
        config.subnet = request->getParam("subnet", true)->value();      // Nouveau
        config.gateway = request->getParam("gateway", true)->value();    // Nouveau
        config.dns1 = request->getParam("dns1", true)->value();          // Nouveau
        config.dns2 = request->getParam("dns2", true)->value();          // Nouveau
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
    server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request)
              {
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

/**
 * Active ou désactive le logger des événements.
 *
 * @param active True pour activer le logger, false pour le désactiver.
 */
void WiFiManagerOTA::setLogger(bool active)
{
    logs.setLogger(active);
}