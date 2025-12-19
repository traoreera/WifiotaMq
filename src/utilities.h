#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "WiFiManagerOTA.h"
#include <cfloat>
// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE BUFFER CIRCULAIRE pour logs
// ═══════════════════════════════════════════════════════════

template <typename T, size_t SIZE>
class CircularBuffer
{
private:
    T buffer[SIZE];
    size_t head = 0;
    size_t tail = 0;
    size_t count = 0;

public:
    bool push(const T &item)
    {
        if (count >= SIZE)
        {
            tail = (tail + 1) % SIZE;
        }
        else
        {
            count++;
        }
        buffer[head] = item;
        head = (head + 1) % SIZE;
        return true;
    }

    bool pop(T &item)
    {
        if (count == 0)
            return false;
        item = buffer[tail];
        tail = (tail + 1) % SIZE;
        count--;
        return true;
    }

    size_t size() const { return count; }
    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count >= SIZE; }
    void clear() { head = tail = count = 0; }

    T &operator[](size_t index)
    {
        return buffer[(tail + index) % SIZE];
    }
};

// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE STATISTIQUES
// ═══════════════════════════════════════════════════════════

class Statistics
{
private:
    float min_val = FLT_MAX;
    float max_val = -FLT_MAX;
    float sum = 0;
    float sumSquared = 0;
    uint32_t count = 0;

public:
    void addValue(float value)
    {
        if (value < min_val)
            min_val = value;
        if (value > max_val)
            max_val = value;
        sum += value;
        sumSquared += value * value;
        count++;
    }

    float getMin() const { return count > 0 ? min_val : 0; }
    float getMax() const { return count > 0 ? max_val : 0; }
    float getAverage() const { return count > 0 ? sum / count : 0; }

    float getStdDev() const
    {
        if (count < 2)
            return 0;
        float avg = getAverage();
        return sqrt((sumSquared / count) - (avg * avg));
    }

    uint32_t getCount() const { return count; }

    void reset()
    {
        min_val = FLT_MAX;
        max_val = -FLT_MAX;
        sum = 0;
        sumSquared = 0;
        count = 0;
    }

    String toJSON() const
    {
        String json = "{";
        json += "\"min\":" + String(getMin(), 2) + ",";
        json += "\"max\":" + String(getMax(), 2) + ",";
        json += "\"avg\":" + String(getAverage(), 2) + ",";
        json += "\"stddev\":" + String(getStdDev(), 2) + ",";
        json += "\"count\":" + String(count);
        json += "}";
        return json;
    }
};

// ═══════════════════════════════════════════════════════════
// WATCHDOG LOGICIEL
// ═══════════════════════════════════════════════════════════

class SoftwareWatchdog
{
private:
    unsigned long timeout;
    unsigned long lastFeed;
    bool enabled = false;
    String name;

public:
    SoftwareWatchdog(const String &name, unsigned long timeoutMs = 60000)
        : name(name), timeout(timeoutMs), lastFeed(millis()) {}

    void enable()
    {
        enabled = true;
        lastFeed = millis();
        Serial.println("🐕 Watchdog '" + name + "' activé (" + String(timeout) + "ms)");
    }

    void disable()
    {
        enabled = false;
        Serial.println("🐕 Watchdog '" + name + "' désactivé");
    }

    void feed()
    {
        lastFeed = millis();
    }

    bool hasExpired()
    {
        if (!enabled)
            return false;
        return (millis() - lastFeed) > timeout;
    }

    void check()
    {
        if (hasExpired())
        {
            Serial.println("💥 WATCHDOG TIMEOUT: " + name);
            Serial.println("   Dernier feed il y a " + String((millis() - lastFeed) / 1000) + "s");
            Serial.println("   Redémarrage...");
            delay(1000);
            ESP.restart();
        }
    }

    unsigned long getTimeSinceLastFeed()
    {
        return millis() - lastFeed;
    }
};

// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE TÂCHES PÉRIODIQUES
// ═══════════════════════════════════════════════════════════

class TaskScheduler
{
private:
    struct Task
    {
        String name;
        unsigned long interval;
        unsigned long lastRun;
        void (*callback)();
        bool enabled;
    };

    static const int MAX_TASKS = 10;
    Task tasks[MAX_TASKS];
    int taskCount = 0;

public:
    bool addTask(const String &name, unsigned long intervalMs, void (*callback)())
    {
        if (taskCount >= MAX_TASKS)
        {
            Serial.println("Trop de tâches");
            return false;
        }
        tasks[taskCount] = {name, intervalMs, millis(), callback, true};
        taskCount++;
        Serial.println("Tâche ajoutée: " + name + " (" + String(intervalMs) + "ms)");
        return true;
    }

    void run()
    {
        unsigned long now = millis();
        for (int i = 0; i < taskCount; i++)
        {
            if (!tasks[i].enabled)
                continue;
            if (now - tasks[i].lastRun >= tasks[i].interval)
            {
                tasks[i].lastRun = now;
                tasks[i].callback();
            }
        }
    }

    void setInterval(const String &name, unsigned long newInterval)
    {
        for (int i = 0; i < taskCount; i++)
        {
            if (tasks[i].name == name)
            {
                tasks[i].interval = newInterval;
                Serial.println("Intervalle modifié: " + name + " -> " + String(newInterval) + "ms");
                return;
            }
        }
    }
};
// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE CONFIGURATION JSON
// ═══════════════════════════════════════════════════════════

class ConfigManager
{
private:
    Preferences prefs;
    String namespace_name;

public:
    ConfigManager(const String &ns = "app_config") : namespace_name(ns) {}

    bool saveString(const String &key, const String &value)
    {
        prefs.begin(namespace_name.c_str(), false);
        bool result = prefs.putString(key.c_str(), value);
        prefs.end();
        return result;
    }

    String loadString(const String &key, const String &defaultValue = "")
    {
        prefs.begin(namespace_name.c_str(), true);
        String value = prefs.getString(key.c_str(), defaultValue);
        prefs.end();
        return value;
    }

    bool saveInt(const String &key, int value)
    {
        prefs.begin(namespace_name.c_str(), false);
        bool result = prefs.putInt(key.c_str(), value);
        prefs.end();
        return result;
    }

    int loadInt(const String &key, int defaultValue = 0)
    {
        prefs.begin(namespace_name.c_str(), true);
        int value = prefs.getInt(key.c_str(), defaultValue);
        prefs.end();
        return value;
    }

    bool saveFloat(const String &key, float value)
    {
        prefs.begin(namespace_name.c_str(), false);
        bool result = prefs.putFloat(key.c_str(), value);
        prefs.end();
        return result;
    }

    float loadFloat(const String &key, float defaultValue = 0.0)
    {
        prefs.begin(namespace_name.c_str(), true);
        float value = prefs.getFloat(key.c_str(), defaultValue);
        prefs.end();
        return value;
    }

    bool saveBool(const String &key, bool value)
    {
        prefs.begin(namespace_name.c_str(), false);
        bool result = prefs.putBool(key.c_str(), value);
        prefs.end();
        return result;
    }

    bool loadBool(const String &key, bool defaultValue = false)
    {
        prefs.begin(namespace_name.c_str(), true);
        bool value = prefs.getBool(key.c_str(), defaultValue);
        prefs.end();
        return value;
    }

    void clear()
    {
        prefs.begin(namespace_name.c_str(), false);
        prefs.clear();
        prefs.end();
        Serial.println("🗑️ Configuration '" + namespace_name + "' effacée");
    }
};

// ═══════════════════════════════════════════════════════════
// FILTRE PASSE-BAS (pour lisser les lectures de capteurs)
// ═══════════════════════════════════════════════════════════

class LowPassFilter
{
private:
    float alpha;
    float filteredValue;
    bool initialized = false;

public:
    LowPassFilter(float smoothingFactor = 0.1) : alpha(smoothingFactor) {}

    float filter(float newValue)
    {
        if (!initialized)
        {
            filteredValue = newValue;
            initialized = true;
            return filteredValue;
        }

        filteredValue = alpha * newValue + (1.0 - alpha) * filteredValue;
        return filteredValue;
    }

    float getValue() const { return filteredValue; }
    void reset() { initialized = false; }
    void setSmoothingFactor(float factor) { alpha = constrain(factor, 0.0, 1.0); }
};

// ═══════════════════════════════════════════════════════════
// DÉTECTEUR DE CHANGEMENT avec hystérésis
// ═══════════════════════════════════════════════════════════

class ChangeDetector
{
private:
    float lastValue;
    float threshold;
    bool firstReading = true;

public:
    ChangeDetector(float changeThreshold = 1.0)
        : threshold(changeThreshold), lastValue(0) {}

    bool hasChanged(float newValue)
    {
        if (firstReading)
        {
            lastValue = newValue;
            firstReading = false;
            return true;
        }

        if (abs(newValue - lastValue) >= threshold)
        {
            lastValue = newValue;
            return true;
        }
        return false;
    }

    float getLastValue() const { return lastValue; }
    void reset() { firstReading = true; }
    void setThreshold(float newThreshold) { threshold = newThreshold; }
};

// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE LED avec patterns
// ═══════════════════════════════════════════════════════════

class LEDManager
{
private:
    int pin;
    unsigned long lastToggle = 0;
    bool state = false;

    enum Pattern
    {
        OFF,
        ON,
        BLINK_SLOW,
        BLINK_FAST,
        PULSE,
        HEARTBEAT
    };

    Pattern currentPattern = OFF;
    unsigned long patternStartTime = 0;

public:
    LEDManager(int ledPin) : pin(ledPin)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void setPattern(const String &pattern)
    {
        if (pattern == "off")
            currentPattern = OFF;
        else if (pattern == "on")
            currentPattern = ON;
        else if (pattern == "blink_slow")
            currentPattern = BLINK_SLOW;
        else if (pattern == "blink_fast")
            currentPattern = BLINK_FAST;
        else if (pattern == "pulse")
            currentPattern = PULSE;
        else if (pattern == "heartbeat")
            currentPattern = HEARTBEAT;

        patternStartTime = millis();
    }

    void update()
    {
        unsigned long now = millis();

        switch (currentPattern)
        {
        case OFF:
            digitalWrite(pin, LOW);
            break;

        case ON:
            digitalWrite(pin, HIGH);
            break;

        case BLINK_SLOW:
            if (now - lastToggle > 1000)
            {
                state = !state;
                digitalWrite(pin, state);
                lastToggle = now;
            }
            break;

        case BLINK_FAST:
            if (now - lastToggle > 200)
            {
                state = !state;
                digitalWrite(pin, state);
                lastToggle = now;
            }
            break;

        case PULSE:
        {
            int brightness = (sin((now - patternStartTime) / 1000.0 * PI) + 1) * 127;
            analogWrite(pin, brightness);
            break;
        }

        case HEARTBEAT:
        {
            unsigned long phase = (now - patternStartTime) % 2000;
            if (phase < 100 || (phase > 200 && phase < 300))
            {
                digitalWrite(pin, HIGH);
            }
            else
            {
                digitalWrite(pin, LOW);
            }
            break;
        }
        }
    }
};

// ═══════════════════════════════════════════════════════════
// FORMATTEUR DE TEMPS
// ═══════════════════════════════════════════════════════════

class TimeFormatter
{
public:
    static String formatUptime(unsigned long milliseconds)
    {
        unsigned long seconds = milliseconds / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        unsigned long days = hours / 24;

        String result = "";
        if (days > 0)
            result += String(days) + "j ";
        if (hours % 24 > 0)
            result += String(hours % 24) + "h ";
        if (minutes % 60 > 0)
            result += String(minutes % 60) + "m ";
        result += String(seconds % 60) + "s";

        return result;
    }

    static String formatBytes(size_t bytes)
    {
        if (bytes < 1024)
            return String(bytes) + " B";
        else if (bytes < 1024 * 1024)
            return String(bytes / 1024.0, 2) + " KB";
        else if (bytes < 1024 * 1024 * 1024)
            return String(bytes / 1024.0 / 1024.0, 2) + " MB";
        else
            return String(bytes / 1024.0 / 1024.0 / 1024.0, 2) + " GB";
    }

    static String formatRSSI(int rssi)
    {
        String quality;
        if (rssi > -50)
            quality = "Excellent";
        else if (rssi > -60)
            quality = "Bon";
        else if (rssi > -70)
            quality = "Moyen";
        else if (rssi > -80)
            quality = "Faible";
        else
            quality = "Très faible";

        return String(rssi) + " dBm (" + quality + ")";
    }
};

// ═══════════════════════════════════════════════════════════
// GESTIONNAIRE DE COMMANDES SÉRIE
// ═══════════════════════════════════════════════════════════

class SerialCommander
{
private:
    String buffer = "";
    void (*commandCallback)(String cmd, String args);

public:
    SerialCommander(void (*callback)(String, String)) : commandCallback(callback) {}

    void process()
    {
        while (Serial.available())
        {
            char c = Serial.read();

            if (c == '\n' || c == '\r')
            {
                if (buffer.length() > 0)
                {
                    processCommand(buffer);
                    buffer = "";
                }
            }
            else
            {
                buffer += c;
            }
        }
    }

private:
    void processCommand(String input)
    {
        input.trim();

        int spaceIndex = input.indexOf(' ');
        String cmd, args;

        if (spaceIndex == -1)
        {
            cmd = input;
            args = "";
        }
        else
        {
            cmd = input.substring(0, spaceIndex);
            args = input.substring(spaceIndex + 1);
        }

        cmd.toLowerCase();
        Serial.println("\n> " + cmd + (args.length() > 0 ? " " + args : ""));

        if (commandCallback)
        {
            commandCallback(cmd, args);
        }
    }
};

// ═══════════════════════════════════════════════════════════
// SYSTÈME DE LOGS avec niveaux
// ═══════════════════════════════════════════════════════════

class Logger
{
public:
    enum Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

private:
    Level currentLevel = INFO;

public:
    bool isEnabled_logger = true;

    void setLevel(Level level) { currentLevel = level; }
    void setLogger(bool active = true)
    {
        isEnabled_logger = active;
    }
    void log(Level level, const String &message)
    {
        if (level < currentLevel)
            return;

        String prefix;
        switch (level)
        {
        case DEBUG:
            prefix = "[DEBUG] ";
            break;
        case INFO:
            prefix = "[INFO] ";
            break;
        case WARNING:
            prefix = "[WARN] ";
            break;
        case ERROR:
            prefix = "[ERROR] ";
            break;
        case CRITICAL:
            prefix = "[CRIT] ";
            break;
        }
        String fullMessage = prefix + message;
        if (isEnabled_logger)
            Serial.println(fullMessage);
    }

    void debug(const String &msg) { log(DEBUG, msg); }
    void info(const String &msg) { log(INFO, msg); }
    void warning(const String &msg) { log(WARNING, msg); }
    void error(const String &msg) { log(ERROR, msg); }
    void critical(const String &msg) { log(CRITICAL, msg); }

private:
    String getLevelString(Level level)
    {
        switch (level)
        {
        case DEBUG:
            return "debug";
        case INFO:
            return "info";
        case WARNING:
            return "warning";
        case ERROR:
            return "error";
        case CRITICAL:
            return "critical";
        default:
            return "unknown";
        }
    }
};

// ═══════════════════════════════════════════════════════════
// SYSTÈME DE GESTION DU BUZZER
// ═══════════════════════════════════════════════════════════

class Buzzer
{
private:
    int pin;
    bool state = false;
    unsigned long lastToggle = 0;
    unsigned int interval = 0;

public:
    Buzzer(int p) : pin(p) {}

    void begin()
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void setPattern(const String &mode)
    {
        if (mode == "off")
        {
            state = false;
            digitalWrite(pin, LOW);
            interval = 0;
        }
        else if (mode == "alert")
        {
            interval = 100; // bip rapide
        }
        else if (mode == "warning")
        {
            interval = 500; // bip lent
        }
    }

    void update()
    {
        if (interval == 0)
            return;
        unsigned long now = millis();
        if (now - lastToggle >= interval)
        {
            state = !state;
            digitalWrite(pin, state ? HIGH : LOW);
            lastToggle = now;
        }
    }
};

#endif // UTILITIES_H