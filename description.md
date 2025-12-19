# GEMINI.md

## Project Overview

This project is an Arduino library for the ESP32 platform named `WifiotaMq`. Its main purpose is to simplify the management of WiFi connectivity, MQTT communication, and Over-the-Air (OTA) firmware updates. It provides a web interface for easy configuration of WiFi and MQTT credentials. The library also includes a rich set of utility classes for common tasks such as logging, task scheduling, and statistics.

**Key Technologies:**

*   Arduino
*   ESP32
*   MQTT
*   AsyncWebServer

**Architecture:**

The library is structured around a main class, `WiFiManagerOTA`, which handles the web server, WiFi connection, and OTA updates. An `MQTTController` class manages the MQTT connection, publishing, and subscribing. A collection of utility classes provides additional functionality.

## Building and Running

The project is an Arduino library and is intended to be used within an Arduino project. The `README.md` file provides a quick start guide for using the library with PlatformIO.

**Key steps to use the library:**

1.  **Include the library:** Add the library to your Arduino project.
2.  **Instantiate `WiFiManagerOTA`:** Create an instance of the `WiFiManagerOTA` class.
3.  **Instantiate `MQTTController`:** Create an instance of the `MQTTController` class.
4.  **Call `begin()` and `loop()`:** Call the `begin()` and `loop()` methods of the `WiFiManagerOTA` and `MQTTController` objects in your `setup()` and `loop()` functions respectively.

**Example:**

```cpp
#include "WiFiManagerOTA.h"
#include "MQTT.h"

bool wifi_connected = false;
WiFiManagerOTA wifiManager(80, "admin", "admin123");
MQTTController* mqttController = nullptr;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = String((char*)payload).substring(0, length);
    if (msg == "status") {
        mqttController->publish("{\"status\":\"ok\"}");
    }
}

void setup() {
    Serial.begin(115200);
    wifiManager.begin("esp32-device", "ESP32-Config", "12345678");

    if (wifi_connected) {
        auto cfg = wifiManager.getMqttConfig();
        mqttController = new MQTTController(
            cfg.hostname.c_str(), cfg.port,
            cfg.user.c_str(), cfg.password.c_str()
        );
        mqttController->setClientId(cfg.client);
        mqttController->setPublishTopic(wifiManager.pubTopic("v1/"));
        mqttController->setSubscribeTopic(wifiManager.cmdTopic("v1/", "/cmd"));
        mqttController->begin();
    }
}

void loop() {
    wifiManager.loop();
    wifiManager.handleWiFiReconnect();
    if (wifi_connected && mqttController) {
        mqttController->loop();
    }
    delay(10);
}
```

## Development Conventions

*   The code is well-documented with comments in both English and French.
*   The library uses a set of utility classes to provide a modular and reusable codebase.
*   The library provides a web interface for configuration, which is a user-friendly approach.
*   The library uses the `Preferences` library to store configuration, which is a good practice for persistent storage.
*   The library uses the `PubSubClient` library for MQTT communication, which is a popular and well-tested library.
*   The library uses `ElegantOTA` for OTA updates, which simplifies the process of updating the firmware.
