# Project Overview

This project is an Arduino library for ESP32 microcontrollers called `WifiotaMq`. Its primary purpose is to simplify the development of ESP32 projects that require WiFi connectivity, over-the-air (OTA) firmware updates, and MQTT communication.

The library provides a web-based interface for configuring WiFi credentials, MQTT broker settings, and performing OTA updates. It also includes a rich set of utility classes for common tasks such as logging, task scheduling, and managing hardware peripherals.

## Key Technologies

*   **Platform:** ESP32
*   **Framework:** Arduino
*   **Core Language:** C++
*   **Networking:**
    *   WiFi management with an access point fallback
    *   MQTT client with SSL/TLS support
    *   Web server for configuration and OTA updates
    *   mDNS for easy device discovery on the local network

## Architecture

The library is structured around a central class, `WiFiManagerOTA`, which encapsulates the core functionality. This class manages the web server, WiFi connection, and OTA updates.

The MQTT functionality is handled by a separate `MQTTController` class, which is designed to be used in conjunction with `WiFiManagerOTA`.

A collection of utility classes in `utilities.h` provides reusable components for various tasks, promoting a modular and organized code structure.

# Building and Running

## Dependencies

*   ESPAsyncWebServer
*   AsyncTCP
*   ElegantOTA
*   PubSubClient
*   ArduinoJson

## Building with PlatformIO

1.  Add the following to your `platformio.ini` file:

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

2.  Build and upload the project to your ESP32 board.

## Building with Arduino IDE

1.  Download the repository as a ZIP file.
2.  In the Arduino IDE, go to **Sketch > Include Library > Add .ZIP Library**.
3.  Select the downloaded ZIP file.
4.  Install the required dependencies from the Library Manager.
5.  Open one of the examples from **File > Examples > WifiotaMq**.
6.  Build and upload the sketch to your ESP32 board.

# Development Conventions

## Coding Style

*   The code follows the standard Arduino C++ style.
*   Class and function names are descriptive and use CamelCase.
*   Comments are used to document the purpose of classes, methods, and complex code blocks.
*   The code is organized into header (.h) and implementation (.cpp) files.

## Testing

The library includes a set of examples that can be used for testing the functionality. The `examples` directory contains sketches for demonstrating basic WiFi and OTA functionality, as well as more advanced scenarios with MQTT.

## Contribution

Contributions are welcome. The `README.md` file provides instructions for contributing to the project, including forking the repository, creating a feature branch, and submitting a pull request.
