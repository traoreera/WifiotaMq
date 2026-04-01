// ============================================
// WiFiManagerOTA.h
// ============================================
#ifndef WIFI_MANAGER_OTA_H
#define WIFI_MANAGER_OTA_H

#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Preferences.h>
#include "utilities.h"

extern bool wifi_connected;
extern Logger logger; // ✅ Fix: nom unifié avec mqtt.h

class WiFiManagerOTA
{
public:
    // ── Structures publiques ──────────────────────────────
    struct MQTTConfig
    {
        String hostname;
        int port = 8883;
        String user;
        String password;
        String client;
    };

    struct WiFiConfigStruct
    {
        String ssid;
        String password;
        bool useStaticIP = false;
        String staticIP;
        String subnet;
        String gateway;
        String dns1;
        String dns2;
    };

    // ── Cycle de vie ──────────────────────────────────────
    WiFiManagerOTA(uint16_t port = 80,
                   const char *user = "admin",
                   const char *pass = "admin123");

    void begin(String hostname, String apName, String apPassword);
    void loop();
    void handleWiFiReconnect();

    // ── Gestion de config ─────────────────────────────────
    void loadConfig();
    void loadMqttConfig();
    void saveConfig();
    void saveMqttConfig();
    void resetConfig();

    // ── Connexion WiFi ────────────────────────────────────
    bool connectToWiFi(int maxAttempts = 20, int delayMs = 500);
    void startAccessPoint(String apName, String password);

    // ── Topics MQTT ───────────────────────────────────────
    String pubTopic(String version);
    String cmdTopic(String version, String cmd);

    // ── Getters ───────────────────────────────────────────
    MQTTConfig getMqttConfig();
    WiFiConfigStruct getWiFiConfig();
    bool hasValidConfig();
    bool isConnected() const { return wifi_connected; }

    // ── Logger ────────────────────────────────────────────
    void setLogger(bool active = true);

private:
    // ── Config interne (contient topic et user_id en plus) ─
    struct WiFiConfig
    {
        String ssid, password, topic, user_id;
        bool useStaticIP = false;
        String staticIP, subnet, gateway, dns1, dns2;
    };

    Preferences prefs;
    AsyncWebServer server;
    WiFiConfig config;
    MQTTConfig mqtt_config;
    String otaUser;
    String otaPass;

    // ✅ Fix: redémarrage différé (évite delay() dans les handlers async)
    bool pendingRestart = false;
    unsigned long restartScheduledAt = 0;
    void scheduleRestart(unsigned long delayMs = 1200);

    unsigned long lastReconnectAttempt = 0;

    // ── Routes web ────────────────────────────────────────
    void setupRoutes();
    void handleConfigPage(AsyncWebServerRequest *request);

    // ── Helpers ───────────────────────────────────────────
    String formatUptime();
    bool authenticate(AsyncWebServerRequest *request); // helper d'auth
};

#endif