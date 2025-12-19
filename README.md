# WifiotaMq

[![Arduino](https://img.shields.io/badge/Arduino-Library-00979D?logo=arduino)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/Platform-ESP32-E7352C?logo=espressif)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-green.svg)](library.properties)

A comprehensive Arduino library for ESP32 that provides WiFi management, MQTT connectivity, and OTA (Over-The-Air) firmware updates through an intuitive web interface.

## ✨ Features

- 🌐 **WiFi Management**
  - Web-based configuration interface
  - Automatic Access Point fallback mode
  - WiFi network scanning and selection
  - Static IP configuration support
  - mDNS support (e.g., `http://esp32ota.local`)

- 📡 **MQTT Controller**
  - Secure (SSL/TLS) and insecure connections
  - Automatic reconnection with exponential backoff
  - Dynamic topic configuration
  - Publish/Subscribe support
  - Custom callback handling

- 🔄 **OTA Updates**
  - Web-based firmware updates via ElegantOTA
  - Password-protected access
  - Easy firmware deployment

- 🔐 **Security**
  - Web interface authentication
  - SSL/TLS support for MQTT
  - Configurable credentials

- 🛠️ **Utility Classes** (12 included)
  - Logger with multiple levels
  - Statistics tracker
  - Task scheduler
  - Software watchdog
  - Circular buffer
  - Low-pass filter
  - Change detector
  - LED manager with patterns
  - Config manager (NVS storage)
  - Time formatter
  - Serial commander
  - Buzzer controller

## 📋 Table of Contents

- [Installation](#installation)
- [Dependencies](#dependencies)
- [Quick Start](#quick-start)
- [Usage Examples](#usage-examples)
- [API Documentation](#api-documentation)
- [Utility Classes](#utility-classes)
- [Web Interface](#web-interface)
- [MQTT Topics](#mqtt-topics)
- [Security](#security)
- [Troubleshooting](#troubleshooting)
- [Advanced Configuration](#advanced-configuration)
- [Contributing](#contributing)
- [License](#license)

## 🚀 Installation

### Arduino IDE

1. Download this repository as a ZIP file
2. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### PlatformIO

Add to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    https://github.com/traoreera/WifiotaMq.git
    me-no-dev/ESPAsyncWebServer
    me-no-dev/AsyncTCP
    ayushsharma82/ElegantOTA
    knolleary/PubSubClient
    bblanchon/ArduinoJson
monitor_speed = 115200
```

## 📦 Dependencies

This library requires the following dependencies:

- **ESPAsyncWebServer** - Async web server
- **AsyncTCP** - Async TCP library for ESP32
- **ElegantOTA** - OTA update library
- **PubSubClient** - MQTT client
- **ArduinoJson** - JSON parsing (optional, for utilities)
- **Preferences** - ESP32 NVS storage (built-in)

## ⚡ Quick Start

### Basic WiFi + OTA Example

```cpp
#include "WiFiManagerOTA.h"

bool wifi_connected = false;
WiFiManagerOTA wifiManager(80, "admin", "admin123");

void setup() {
    Serial.begin(115200);
    
    // Start WiFi manager with:
    // - mDNS hostname: "esp32ota" (access via http://esp32ota.local)
    // - AP name: "ESP32-Config"
    // - AP password: "12345678"
    wifiManager.begin("esp32ota", "ESP32-Config", "12345678");
}

void loop() {
    wifiManager.loop();
    wifiManager.handleWiFiReconnect();
    delay(10);
}
```

### First Time Setup

1. **Power on your ESP32** - It will create a WiFi Access Point
2. **Connect to the AP**:
   - SSID: `ESP32-Config`
   - Password: `12345678`
3. **Open your browser** and navigate to: `http://192.168.4.1`
4. **Login** with default credentials:
   - Username: `admin`
   - Password: `admin123`
5. **Configure WiFi**:
   - Click on "⚙️ WiFi" or "Config"
   - Select your WiFi network
   - Enter password
   - Save configuration
6. **ESP32 will restart** and connect to your WiFi network
7. **Access via mDNS**: `http://esp32ota.local` (or use the IP address shown in serial monitor)

## 📚 Usage Examples

### Example 1: Simple WiFi + OTA

```cpp
#include "WiFiManagerOTA.h"
#include "utilities.h"

bool wifi_connected = false;
WiFiManagerOTA server(80, "admin", "admin123");
Logger logger;

void setup() {
    logger.setLevel(logger.INFO);
    logger.isEnabled = true;
    
    logger.info("╔══════════════════════════════════════════╗");
    logger.info("║   Simple OTA Server Initialization      ║");
    logger.info("╚══════════════════════════════════════════╝");
    
    server.begin("esp32ota", "ESP32-Config", "12345678");
}

void loop() {
    server.loop();
    server.handleWiFiReconnect();
}
```

### Example 2: WiFi + MQTT (Insecure)

```cpp
#include "mqtt.h"
#include "WiFiManagerOTA.h"
#include "utilities.h"

Logger logger;
bool wifi_connected = false;
WiFiManagerOTA server(80, "admin", "admin123");
MQTTController* mqttController = nullptr;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    logger.info("Message received: " + message);
    
    // Handle commands
    if (message == "status") {
        String status = "{\"status\":\"online\",\"uptime\":" + String(millis()) + "}";
        mqttController->publish(status);
    }
}

void setup() {
    logger.setLevel(logger.INFO);
    logger.isEnabled = true;
    
    String clientId = "esp32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    server.begin("esp32ota", "ESP32-Config", "12345678");
    
    if (wifi_connected) {
        auto cfg = server.getMqttConfig();
        mqttController = new MQTTController(
            cfg.hostname.c_str(),
            cfg.port,
            cfg.user.c_str(),
            cfg.password.c_str()
        );
        
        mqttController->setClientId(clientId);
        mqttController->setPublishTopic(server.pubTopic("v1/"));
        mqttController->setSubscribeTopic(server.cmdTopic("v1/", "/cmd"));
        mqttController->isSecure = false;  // Insecure connection
        mqttController->begin();
    }
}

void loop() {
    server.loop();
    
    if (wifi_connected && mqttController) {
        mqttController->loop();
        
        // Publish sensor data every 60 seconds
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 60000) {
            String json = "{";
            json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
            json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
            json += "\"freeHeap\":" + String(ESP.getFreeHeap());
            json += "}";
            
            mqttController->publish(json);
            lastPublish = millis();
        }
    }
    
    server.handleWiFiReconnect();
}
```

### Example 3: WiFi + MQTT (Secure with SSL/TLS)

```cpp
#include "mqtt.h"
#include "WiFiManagerOTA.h"
#include "utilities.h"

Logger logger;
bool wifi_connected = false;
WiFiManagerOTA server(80, "admin", "admin123");
MQTTController* mqttController = nullptr;

// Root CA Certificate (replace with your broker's certificate)
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
... (your certificate here) ...
-----END CERTIFICATE-----
)EOF";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    logger.info("Message received: " + message);
}

void setup() {
    logger.setLevel(logger.INFO);
    logger.isEnabled = true;
    
    String clientId = "esp32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    server.begin("esp32ota", "ESP32-Config", "12345678");
    
    if (wifi_connected) {
        auto cfg = server.getMqttConfig();
        mqttController = new MQTTController(
            cfg.hostname.c_str(),
            cfg.port,
            cfg.user.c_str(),
            cfg.password.c_str()
        );
        
        mqttController->setClientId(clientId);
        mqttController->setPublishTopic(server.pubTopic("v1/"));
        mqttController->setSubscribeTopic(server.cmdTopic("v1/", "/cmd"));
        
        // Enable secure connection
        mqttController->isSecure = true;
        mqttController->setSecure(root_ca);
        
        mqttController->begin();
    }
}

void loop() {
    server.loop();
    
    if (wifi_connected && mqttController) {
        mqttController->loop();
    }
    
    server.handleWiFiReconnect();
}
```

## 📖 API Documentation

### WiFiManagerOTA Class

#### Constructor

```cpp
WiFiManagerOTA(uint16_t port = 80, const char* user = "admin", const char* pass = "admin123")
```

Creates a WiFiManagerOTA instance.

**Parameters:**
- `port` - Web server port (default: 80)
- `user` - Web interface username (default: "admin")
- `pass` - Web interface password (default: "admin123")

#### Methods

##### `void begin(String hostname, String apName, String apPassword)`

Initializes the WiFi manager.

**Parameters:**
- `hostname` - mDNS hostname (e.g., "esp32ota" → http://esp32ota.local)
- `apName` - Access Point SSID when in AP mode
- `apPassword` - Access Point password (min 8 characters)

##### `void loop()`

Must be called in the main loop to handle web server requests.

##### `void handleWiFiReconnect()`

Handles automatic WiFi reconnection. Call this in your main loop.

##### `MQTTConfig getMqttConfig()`

Returns the MQTT configuration stored in NVS.

**Returns:** `MQTTConfig` struct with hostname, port, user, password, and client ID.

##### `WiFiConfigStruct getWiFiConfig()`

Returns the WiFi configuration.

**Returns:** `WiFiConfigStruct` with SSID, password, and static IP settings.

##### `String pubTopic(String version)`

Generates the publish topic based on configuration.

**Parameters:**
- `version` - Version prefix (e.g., "v1/")

**Returns:** Complete publish topic (e.g., "home/sensor/v1/device001")

##### `String cmdTopic(String version, String cmd)`

Generates the command/subscribe topic.

**Parameters:**
- `version` - Version prefix (e.g., "v1/")
- `cmd` - Command suffix (e.g., "/cmd")

**Returns:** Complete command topic (e.g., "home/sensor/v1/device001/cmd")

##### `void loadConfig()` / `void saveConfig()`

Load/save WiFi configuration from/to NVS.

##### `void loadMqttConfig()` / `void saveMqttConfig()`

Load/save MQTT configuration from/to NVS.

##### `void resetConfig()`

Resets all configuration to defaults.

##### `bool hasValidConfig()`

Checks if valid WiFi configuration exists.

**Returns:** `true` if WiFi credentials are configured.

### MQTTController Class

#### Constructor

```cpp
MQTTController(const char* mqtt_server, int mqtt_port, const char* mqtt_user, const char* mqtt_password)
```

Creates an MQTT controller instance.

#### Methods

##### `void begin()`

Initializes MQTT connection. Call after setting topics and client ID.

##### `void loop()`

Handles MQTT connection and reconnection. Must be called in main loop.

##### `bool publish(const char* topic, const String& message)`

Publishes a message to a specific topic.

**Returns:** `true` if successful.

##### `bool publish(const String& message)`

Publishes to the configured publish topic.

**Returns:** `true` if successful.

##### `void setPublishTopic(const String& topic)`

Sets the default publish topic.

##### `void setSubscribeTopic(const String& topic)`

Sets the subscribe topic and subscribes if connected.

##### `void setClientId(const String& id)`

Sets the MQTT client ID.

##### `void setSecure(const char* caCert)`

Sets the CA certificate for SSL/TLS connection.

#### Properties

##### `bool isSecure`

Set to `true` for SSL/TLS connection, `false` for insecure (default: `false`).

## 🛠️ Utility Classes

The library includes 12 utility classes for common tasks:

### 1. Logger

Multi-level logging system with DEBUG, INFO, WARNING, ERROR, and CRITICAL levels.

```cpp
Logger logger;
logger.setLevel(logger.INFO);
logger.isEnabled = true;

logger.debug("Debug message");
logger.info("Info message");
logger.warning("Warning message");
logger.error("Error message");
logger.critical("Critical message");
```

### 2. Statistics

Track min, max, average, and standard deviation.

```cpp
Statistics stats;
stats.addValue(25.5);
stats.addValue(26.3);
stats.addValue(24.8);

Serial.println("Min: " + String(stats.getMin()));
Serial.println("Max: " + String(stats.getMax()));
Serial.println("Avg: " + String(stats.getAverage()));
Serial.println("StdDev: " + String(stats.getStdDev()));
Serial.println(stats.toJSON());
```

### 3. TaskScheduler

Schedule periodic tasks without blocking.

```cpp
TaskScheduler scheduler;

void readSensor() {
    Serial.println("Reading sensor...");
}

void setup() {
    scheduler.addTask("sensor", 5000, readSensor);  // Every 5 seconds
}

void loop() {
    scheduler.run();
}
```

### 4. SoftwareWatchdog

Monitor system health and auto-restart on timeout.

```cpp
SoftwareWatchdog watchdog("main", 60000);  // 60 second timeout

void setup() {
    watchdog.enable();
}

void loop() {
    watchdog.feed();  // Reset watchdog
    // Your code here
    watchdog.check();  // Will restart if timeout exceeded
}
```

### 5. CircularBuffer

FIFO queue with fixed size.

```cpp
CircularBuffer<String, 10> buffer;

buffer.push("Message 1");
buffer.push("Message 2");

String msg;
if (buffer.pop(msg)) {
    Serial.println(msg);
}
```

### 6. LowPassFilter

Smooth noisy sensor readings.

```cpp
LowPassFilter filter(0.1);  // Smoothing factor

float rawValue = analogRead(A0);
float smoothed = filter.filter(rawValue);
```

### 7. ChangeDetector

Detect significant changes with hysteresis.

```cpp
ChangeDetector detector(1.0);  // Threshold of 1.0

float newValue = readSensor();
if (detector.hasChanged(newValue)) {
    Serial.println("Value changed significantly!");
}
```

### 8. LEDManager

Control LED with various patterns.

```cpp
LEDManager led(LED_BUILTIN);

led.setPattern("heartbeat");  // Options: off, on, blink_slow, blink_fast, pulse, heartbeat

void loop() {
    led.update();
}
```

### 9. ConfigManager

Store configuration in NVS flash.

```cpp
ConfigManager config("myapp");

config.saveString("ssid", "MyWiFi");
config.saveInt("interval", 5000);
config.saveBool("enabled", true);

String ssid = config.loadString("ssid");
int interval = config.loadInt("interval");
bool enabled = config.loadBool("enabled");
```

### 10. TimeFormatter

Format time and data sizes.

```cpp
String uptime = TimeFormatter::formatUptime(millis());
String memory = TimeFormatter::formatBytes(ESP.getFreeHeap());
String signal = TimeFormatter::formatRSSI(WiFi.RSSI());
```

### 11. SerialCommander

Handle serial commands.

```cpp
void handleCommand(String cmd, String args) {
    if (cmd == "status") {
        Serial.println("System OK");
    } else if (cmd == "restart") {
        ESP.restart();
    }
}

SerialCommander commander(handleCommand);

void loop() {
    commander.process();
}
```

### 12. Buzzer

Control buzzer with patterns.

```cpp
Buzzer buzzer(BUZZER_PIN);

void setup() {
    buzzer.begin();
    buzzer.setPattern("alert");  // Options: off, alert, warning
}

void loop() {
    buzzer.update();
}
```

## 🌐 Web Interface

### Accessing the Interface

1. **Via mDNS**: `http://[hostname].local` (e.g., `http://esp32ota.local`)
2. **Via IP**: `http://[ESP32_IP_ADDRESS]`
3. **AP Mode**: `http://192.168.4.1`

### Available Pages

- **Home** (`/`) - System status and information
- **WiFi Config** (`/config`) - Configure WiFi settings
- **MQTT Config** (`/mqtt`) - Configure MQTT broker settings
- **OTA Update** (`/update`) - Upload new firmware
- **Status JSON** (`/status`) - JSON status endpoint
- **Reset** (`/reset`) - Reset configuration

### Configuration Options

#### WiFi Configuration
- SSID selection from scan
- Password
- Static IP settings (optional)
- Gateway and DNS configuration

#### MQTT Configuration
- Broker hostname/IP
- Port (1883 for insecure, 8883 for SSL/TLS)
- Username and password
- Client ID
- Topic prefix
- User ID (for topic generation)

## 📡 MQTT Topics

### Topic Structure

The library uses a flexible topic structure:

```
[topic_prefix]/[version]/[user_id][suffix]
```

**Example:**
- Topic Prefix: `home/sensor`
- User ID: `device001`
- Version: `v1/`

**Results in:**
- Publish Topic: `home/sensor/v1/device001`
- Command Topic: `home/sensor/v1/device001/cmd`

### Using Topics in Code

```cpp
// Set topics using helper methods
mqttController->setPublishTopic(server.pubTopic("v1/"));
mqttController->setSubscribeTopic(server.cmdTopic("v1/", "/cmd"));

// Or set manually
mqttController->setPublishTopic("home/sensor/v1/device001");
mqttController->setSubscribeTopic("home/sensor/v1/device001/cmd");
```

### Publishing Messages

```cpp
// Publish to default topic
mqttController->publish("{\"temperature\":25.5}");

// Publish to specific topic
mqttController->publish("custom/topic", "{\"data\":\"value\"}");
```

## 🔐 Security

### Changing Default Credentials

**⚠️ IMPORTANT:** Always change default credentials in production!

```cpp
// Change web interface credentials
WiFiManagerOTA server(80, "your_username", "YourSecurePassword123!");
```

### Enabling SSL/TLS for MQTT

1. **Obtain your broker's CA certificate**
2. **Add it to your code:**

```cpp
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
[Your CA Certificate Here]
-----END CERTIFICATE-----
)EOF";
```

3. **Enable secure connection:**

```cpp
mqttController->isSecure = true;
mqttController->setSecure(root_ca);
```

### Security Best Practices

✅ **DO:**
- Change default web interface credentials
- Use SSL/TLS for MQTT in production
- Use strong passwords (min 12 characters)
- Keep firmware updated
- Use unique client IDs

❌ **DON'T:**
- Use default credentials in production
- Use `setInsecure()` in production
- Expose web interface to public internet without VPN
- Hardcode sensitive credentials (use web config instead)

## 🐛 Troubleshooting

### ESP32 Won't Connect to WiFi

**Symptoms:** Stays in AP mode

**Solutions:**
1. Verify SSID and password are correct
2. Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check signal strength (RSSI should be > -80 dBm)
4. Reset configuration via web interface: `http://[IP]/reset`
5. Check serial monitor for error messages

### MQTT Connection Fails

**Symptoms:** No messages published/received

**Solutions:**
1. Verify broker is accessible (ping the hostname/IP)
2. Check port number (1883 for insecure, 8883 for SSL/TLS)
3. Verify username and password
4. Check MQTT state in serial monitor:
   - `-4`: Connection timeout
   - `-3`: Connection lost
   - `-2`: Connect failed
   - `-1`: Disconnected
   - `0`: Connected
5. For SSL/TLS: Verify certificate is correct
6. Check firewall rules on broker

### OTA Update Fails

**Symptoms:** Upload fails or ESP32 crashes

**Solutions:**
1. Ensure stable WiFi connection
2. Check firmware size (must fit in partition)
3. Use correct partition scheme in `platformio.ini`:
   ```ini
   board_build.partitions = default.csv
   ```
4. Try uploading via `/update` endpoint directly
5. Increase upload timeout in browser

### Memory Issues

**Symptoms:** Random crashes, reboots

**Solutions:**
1. Monitor free heap:
   ```cpp
   Serial.println("Free heap: " + String(ESP.getFreeHeap()));
   ```
2. If free heap < 50KB:
   - Reduce buffer sizes
   - Use `PROGMEM` for large strings
   - Free unused objects
   - Reduce JSON document sizes

### Web Interface Not Accessible

**Symptoms:** Cannot access web pages

**Solutions:**
1. Check ESP32 is connected to WiFi (check serial monitor)
2. Verify IP address (shown in serial monitor)
3. Try mDNS: `http://[hostname].local`
4. Disable firewall temporarily
5. Try different browser
6. Clear browser cache

## ⚙️ Advanced Configuration

### Static IP Configuration

Configure via web interface or programmatically:

```cpp
auto wifiConfig = server.getWiFiConfig();
wifiConfig.useStaticIP = true;
wifiConfig.staticIP = "192.168.1.100";
wifiConfig.subnet = "255.255.255.0";
wifiConfig.gateway = "192.168.1.1";
wifiConfig.dns1 = "8.8.8.8";
wifiConfig.dns2 = "8.8.4.4";
```

### Custom Logging Levels

```cpp
logger.setLevel(logger.DEBUG);  // Show all messages
logger.setLevel(logger.WARNING);  // Only warnings and above
logger.setLevel(logger.ERROR);  // Only errors and critical
```

### Adjusting Reconnection Intervals

Modify in `mqtt.h`:

```cpp
unsigned long reconnectInterval = 2000;  // Initial: 2 seconds
// Max interval: 60 seconds (with exponential backoff)
```

### Custom Web Pages

You can extend the web interface by adding custom routes in `WiFiManagerOTA.cpp`:

```cpp
server.on("/custom", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<h1>Custom Page</h1>");
});
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 👤 Author

**traoreera**
- Email: traoreera@gmail.com
- GitHub: [@traoreera](https://github.com/traoreera)

## 🙏 Acknowledgments

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- ESP32 Arduino Core team

## 📞 Support

For issues, questions, or suggestions:
1. Check the [Troubleshooting](#troubleshooting) section
2. Review existing [GitHub Issues](https://github.com/traoreera/WifiotaMq/issues)
3. Create a new issue with detailed information

---

**Made with ❤️ for the ESP32 community**
