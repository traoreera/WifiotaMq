#include "WiFiManagerOTA.h"
#include "utilities.h"


// create wifi status connection
bool wifi_connected = false;


// create an instance of the server
// specify the port to listen on as an argument
// or use the default port 80 for HTTP and ota username and password
WiFiManagerOTA server(80, "admin", "admin123");

// create logger for app
Logger logger;


void setup()
{
    logger.setLevel(logger.INFO); // set log level
    logger.isEnabled = true; // enable logging

    logger.info("╔══════════════════════════════════════════╗");
    logger.info("║   Simple OTA web server Initialisation   ║");
    logger.info("╚══════════════════════════════════════════╝");

    // start the server and configure hostname using dns server => http://esp32ota.local
    // set esp as an access point wiht name esp32-ota
    // and password esp32-pass
    server.begin("esp32ota", "esp32-ota","esp32-pass");
}

void loop()
{
    server.loop(); // loop for async web server for ota
    /*
        put your main code here, to run repeatedly
    */
    server.handleWiFiReconnect();
}