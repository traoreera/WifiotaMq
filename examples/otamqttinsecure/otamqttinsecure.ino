#include "mqtt.h"
#include "WiFiManagerOTA.h"
#include "utilities.h"


// create logger for app
Logger logger;

// create wifi status connection
bool wifi_connected = false
int sensorInterval = 60000;
unsigned long lastSensorRead = 0;
// create an instance of the server
// specify the port to listen on as an argument
// or use the default port 80 for HTTP and ota username and password
WiFiManagerOTA server(80, "admin", "admin123");

// mqtt manager this is pointer
MQTTController *mqttController = nullptr;



// create mqtt calback
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    logger.info("Message arrivé: " + message);
}




void setup()
{
    logger.setLevel(logger.INFO); // set log level
    logger.isEnabled = true; // enable logging

    String cliendId = "ep32"+ String((uint32_t)ESP.getEfuseMac(), HEX); // create client id with mac address




    logger.info("╔══════════════════════════════════════════╗");
    logger.info("║   Simple OTA web server Initialisation   ║");
    logger.info("╚══════════════════════════════════════════╝");

    // start the server and configure hostname using dns server => http://esp32ota.local
    // set esp as an access point wiht name esp32-ota
    // and password esp32-pass
    server.begin("esp32ota", "esp32-ota","esp32-pass"); 
    // after this you can create an mqtt client and connect to the broker

    if (wifi_connected){
        // create this
        auto cfg = server.getMqttConfig();
        mqttController = new MQTTController(
            cfg.hostname.c_str(),
            cfg.port, cfg.user.c_str(),
            cfg.password.c_str()
        );
        mqttController->setClientId(cliendId);
        mqttController->setPublishTopic(server.pubTopic("/fire/")); // this for publish
        mqttController->setSubscribeTopic(server.cmdTopic("/fire/", "b0x1323")); // this for calback
        
        //security 
        mqttController->isSecure = false; // this line is important you can remove it if you want
        mqttController->begin();
    }
}




void loop()
{
    server.loop(); // loop for async web server for ota
    if(wifi_connected){
        mqttController->loop(); // this fonction test reconnection wifi and mqtt
    }
    // exemple of json mqtt message for send 
    String json = "{";
    json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + formatUptime() + "\",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz());
    //json +="\"timestamp\":"+ String(millis()); this is optional value
    json += "}";

    unsigned long now = millis();
    if (now - lastSensorRead > sensorInterval) { // every one mm this conde send json message 
        logger.info(json);
        lastSensorRead = now;
        mqttController->publish(mqttController->getPublishTopic().c_str(), json);
    }
    server.handleWiFiReconnect();
}