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



class WiFiManagerOTA
{
public:
    struct MQTTConfig
    {
        String hostname;
        int port;
        String user;
        String password;
        String client;
    };

    struct WiFiConfigStruct
    {
        String ssid;
        String password;
        bool useStaticIP;  
        String staticIP;   
        String subnet;    
        String gateway;    
        String dns1;       
        String dns2;    
    };

    void setLogger(bool active =true);

    WiFiManagerOTA(uint16_t port = 80, const char *user = "admin", const char *pass = "admin123");

    // Lifecycle
    void begin(String hostname, String apName, String apPassword);
    void loop();
    void handleWiFiReconnect();

    // Config management
    void loadConfig();
    void loadMqttConfig();
    void saveConfig();
    void saveMqttConfig();
    void resetConfig();

    // WiFi connection
    bool connectToWiFi(int maxAttempts = 20, int delayMs = 500);
    void startAccessPoint(String apName, String password);

    // Topic helpers
    String pubTopic(String version);
    String cmdTopic(String version, String cmd);

    // Getters
    MQTTConfig getMqttConfig();
    WiFiConfigStruct getWiFiConfig();
    bool hasValidConfig();

private:
    struct WiFiConfig
    {
        String ssid;
        String password;
        String topic;
        String user_id;
        bool useStaticIP;  
        String staticIP;   
        String subnet;    
        String gateway;    
        String dns1;       
        String dns2;  
    };

    Preferences prefs;
    AsyncWebServer server;
    WiFiConfig config;
    MQTTConfig mqtt_config;
    String otaUser;
    String otaPass;
    unsigned long lastReconnectAttempt;

    // Web pages HTML
    void setupRoutes();
    void handleConfigPage(AsyncWebServerRequest *request);
    String formatUptime();

    // HTML templates
    const String &getIndexHtml();
    const String &getConfigHtml();
    const String &getMqttConfigHtml();
};

#endif