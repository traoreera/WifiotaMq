#include "mqtt.h"
#include "WiFiManagerOTA.h"
#include "utilities.h"


// create logger for app
Logger logger;




const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIID2TCCAsGgAwIBAgIUYbYrlMB3xSCAcXPz0gERcziwVBswDQYJKoZIhvcNAQEL
BQAwfDELMAkGA1UEBhMCQkYxDzANBgNVBAgMBkNlbnRyZTEUMBIGA1UEBwwLT3Vh
Z2Fkb3Vnb3UxGTAXBgNVBAoMEE1vbiBPcmdhbmlzYXRpb24xFTATBgNVBAsMDE1v
biBVbml0w4PCqTEUMBIGA1UEAwwLZXhhbXBsZS5jb20wHhcNMjUxMDEzMTA0MzQ2
WhcNMjYxMDEzMTA0MzQ2WjB8MQswCQYDVQQGEwJCRjEPMA0GA1UECAwGQ2VudHJl
MRQwEgYDVQQHDAtPdWFnYWRvdWdvdTEZMBcGA1UECgwQTW9uIE9yZ2FuaXNhdGlv
bjEVMBMGA1UECwwMTW9uIFVuaXTDg8KpMRQwEgYDVQQDDAtleGFtcGxlLmNvbTCC
ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALRzcuDDsfe1JD8oojxQX7fC
nqyhbK62TjQ14WglYte3GfGpb4ecZU1NSuMlfI1B/SUwoKRKSVF816rRPRakhXf6
0LRbcUvZV2EzmwzqdC3NEFEUMiY2NI7GXPwaeq6LoS8gs1gXFVU08v6pheS40ty9
lmbt1WojgdYXW8dPDChccKGRqH3vEjir587lRGc9oNtl7BUfxsOIIDQ4x4xEo1eO
ttjb8N7ypk7VJiLC1eEW0qf6eZUPkKnV/lORtZM/Ti2YF3VboSuTkWEB8fb4tGLb
/kF7SQAFbhsRVDGxocpr0H5pNRK4ixDWYW8I85Uwb14uxfCMqifdKtTT9gU0dhEC
AwEAAaNTMFEwHQYDVR0OBBYEFJxIsjQMD1HhoAHuyIU8h4b7ZdgbMB8GA1UdIwQY
MBaAFJxIsjQMD1HhoAHuyIU8h4b7ZdgbMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZI
hvcNAQELBQADggEBAJEMlzpfcyzbPfOtbtGv2Zmn8l1TP+ELaAoe5Taf9Lp4R/oh
ervr2t+SYAMAcGJHNifIn3WtZjNn/SpZl0vLSVzR2ZYinZpmYHH8+rNBacdX4yLr
rKIqHZ9kolMdkO43gDG58NRiwTpCm9HI22b7z6eQU6sEE2ZWnd/6r8iVYJMovQGE
m5Dp2t1rZ6HQDivR6ehVK89uHFhafF+sRI8BXhqgpu9h25N0/Xb7jprGIc9t6viG
45Docn5YtFUy4DPcm6tjqiqB0ZZX9529Gt19MWGKu2O4hhQEA9Wjp34Yjlggyy6S
67i1NQi6ohuqTk3m6rfbkDyeVZm+9Fgw1FyJNKg=
-----END CERTIFICATE-----

)EOF"; // this is certication but you can use filefts to send on esp certificate.pem file 

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
        mqttController->isSecure = true;
        mqttController->setSecure(caCert);
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