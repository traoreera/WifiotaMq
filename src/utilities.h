#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h> // ✅ Fix: include explicite
#include <functional>    // ✅ Fix: pour std::function dans TaskScheduler
#include <cfloat>

// ═══════════════════════════════════════════════════════════
// BUFFER CIRCULAIRE
// ═══════════════════════════════════════════════════════════
template <typename T, size_t SIZE>
class CircularBuffer
{
private:
    T buffer[SIZE];
    size_t head = 0, tail = 0, count = 0;

public:
    bool push(const T &item)
    {
        if (count >= SIZE)
            tail = (tail + 1) % SIZE;
        else
            count++;
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

    T &operator[](size_t i) { return buffer[(tail + i) % SIZE]; }
    size_t size() const { return count; }
    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count >= SIZE; }
    void clear() { head = tail = count = 0; }
};

// ═══════════════════════════════════════════════════════════
// LOGGER  (instance partagée définie dans WiFiManagerOTA.cpp)
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

    bool isEnabled_logger = true;

    void setLogger(bool active) { isEnabled_logger = active; }
    void setLevel(Level l) { currentLevel = l; }

    void log(Level level, const String &message)
    {
        if (!isEnabled_logger || level < currentLevel)
            return;
        const char *prefix[] = {"[DEBUG] ", "[INFO]  ", "[WARN]  ", "[ERROR] ", "[CRIT]  "};
        Serial.println(String(prefix[level]) + message);
    }

    void debug(const String &m) { log(DEBUG, m); }
    void info(const String &m) { log(INFO, m); }
    void warning(const String &m) { log(WARNING, m); }
    void error(const String &m) { log(ERROR, m); }
    void critical(const String &m) { log(CRITICAL, m); }

private:
    Level currentLevel = INFO;
};

// ═══════════════════════════════════════════════════════════
// STATISTIQUES
// ═══════════════════════════════════════════════════════════
class Statistics
{
private:
    float min_val = FLT_MAX, max_val = -FLT_MAX;
    float sum = 0, sumSquared = 0;
    uint32_t count = 0;

public:
    void addValue(float v)
    {
        if (v < min_val)
            min_val = v;
        if (v > max_val)
            max_val = v;
        sum += v;
        sumSquared += v * v;
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
        return sqrtf((sumSquared / count) - (avg * avg));
    }
    uint32_t getCount() const { return count; }
    void reset()
    {
        min_val = FLT_MAX;
        max_val = -FLT_MAX;
        sum = sumSquared = 0;
        count = 0;
    }

    String toJSON() const
    {
        return "{\"min\":" + String(getMin(), 2) +
               ",\"max\":" + String(getMax(), 2) +
               ",\"avg\":" + String(getAverage(), 2) +
               ",\"stddev\":" + String(getStdDev(), 2) +
               ",\"count\":" + String(count) + "}";
    }
};

// ═══════════════════════════════════════════════════════════
// WATCHDOG LOGICIEL
// ═══════════════════════════════════════════════════════════
class SoftwareWatchdog
{
private:
    String name;
    unsigned long timeout, lastFeed;
    bool enabled = false;

public:
    SoftwareWatchdog(const String &name, unsigned long timeoutMs = 60000)
        : name(name), timeout(timeoutMs), lastFeed(millis()) {}

    void enable()
    {
        enabled = true;
        lastFeed = millis();
    }
    void disable() { enabled = false; }
    void feed() { lastFeed = millis(); }

    bool hasExpired() const { return enabled && (millis() - lastFeed) > timeout; }

    // ✅ Fix: utilise Serial directement (pas de dépendance circulaire)
    void check()
    {
        if (!hasExpired())
            return;
        Serial.printf("[WATCHDOG] TIMEOUT '%s' — dernier feed il y a %lus\n",
                      name.c_str(), (millis() - lastFeed) / 1000UL);
        delay(500);
        ESP.restart();
    }

    unsigned long timeSinceLastFeed() const { return millis() - lastFeed; }
};

// ═══════════════════════════════════════════════════════════
// TASK SCHEDULER  — supporte lambdas et méthodes membres
// ═══════════════════════════════════════════════════════════
class TaskScheduler
{
private:
    struct Task
    {
        String name;
        unsigned long interval;
        unsigned long lastRun;
        std::function<void()> callback; // ✅ Fix: accepte lambdas
        bool enabled = true;
    };

    static constexpr int MAX_TASKS = 10;
    Task tasks[MAX_TASKS];
    int taskCount = 0;

public:
    // Accepte lambdas : scheduler.addTask("lire", 5000, [&]{ capteur.lire(); });
    bool addTask(const String &name, unsigned long intervalMs,
                 std::function<void()> cb)
    {
        if (taskCount >= MAX_TASKS)
        {
            Serial.println("[Scheduler] Trop de tâches (max " + String(MAX_TASKS) + ")");
            return false;
        }
        tasks[taskCount++] = {name, intervalMs, millis(), cb, true};
        Serial.println("[Scheduler] Tâche '" + name + "' ajoutée (" + String(intervalMs) + " ms)");
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

    void enable(const String &name) { setEnabled(name, true); }
    void disable(const String &name) { setEnabled(name, false); }

    void setInterval(const String &name, unsigned long newInterval)
    {
        for (int i = 0; i < taskCount; i++)
            if (tasks[i].name == name)
            {
                tasks[i].interval = newInterval;
                return;
            }
    }

private:
    void setEnabled(const String &name, bool state)
    {
        for (int i = 0; i < taskCount; i++)
            if (tasks[i].name == name)
            {
                tasks[i].enabled = state;
                return;
            }
    }
};

// ═══════════════════════════════════════════════════════════
// CONFIG MANAGER
// ═══════════════════════════════════════════════════════════
class ConfigManager
{
private:
    Preferences prefs;
    String ns;

public:
    explicit ConfigManager(const String &ns = "app_config") : ns(ns) {}

    bool saveString(const String &k, const String &v)
    {
        prefs.begin(ns.c_str(), false);
        bool r = prefs.putString(k.c_str(), v);
        prefs.end();
        return r;
    }
    String loadString(const String &k, const String &def = "")
    {
        prefs.begin(ns.c_str(), true);
        String v = prefs.getString(k.c_str(), def);
        prefs.end();
        return v;
    }
    bool saveInt(const String &k, int v)
    {
        prefs.begin(ns.c_str(), false);
        bool r = prefs.putInt(k.c_str(), v);
        prefs.end();
        return r;
    }
    int loadInt(const String &k, int def = 0)
    {
        prefs.begin(ns.c_str(), true);
        int v = prefs.getInt(k.c_str(), def);
        prefs.end();
        return v;
    }
    bool saveFloat(const String &k, float v)
    {
        prefs.begin(ns.c_str(), false);
        bool r = prefs.putFloat(k.c_str(), v);
        prefs.end();
        return r;
    }
    float loadFloat(const String &k, float def = 0.f)
    {
        prefs.begin(ns.c_str(), true);
        float v = prefs.getFloat(k.c_str(), def);
        prefs.end();
        return v;
    }
    bool saveBool(const String &k, bool v)
    {
        prefs.begin(ns.c_str(), false);
        bool r = prefs.putBool(k.c_str(), v);
        prefs.end();
        return r;
    }
    bool loadBool(const String &k, bool def = false)
    {
        prefs.begin(ns.c_str(), true);
        bool v = prefs.getBool(k.c_str(), def);
        prefs.end();
        return v;
    }

    void clear()
    {
        prefs.begin(ns.c_str(), false);
        prefs.clear();
        prefs.end();
        Serial.println("[ConfigManager] Namespace '" + ns + "' effacé");
    }
};

// ═══════════════════════════════════════════════════════════
// FILTRE PASSE-BAS
// ═══════════════════════════════════════════════════════════
class LowPassFilter
{
private:
    float alpha;
    float filtered = 0;
    bool initialized = false;

public:
    explicit LowPassFilter(float alpha = 0.1f) : alpha(constrain(alpha, 0.f, 1.f)) {}

    float filter(float v)
    {
        if (!initialized)
        {
            filtered = v;
            initialized = true;
            return v;
        }
        filtered = alpha * v + (1.f - alpha) * filtered;
        return filtered;
    }

    float getValue() const { return filtered; }
    void reset() { initialized = false; }
    void setSmoothingFactor(float a) { alpha = constrain(a, 0.f, 1.f); }
};

// ═══════════════════════════════════════════════════════════
// DÉTECTEUR DE CHANGEMENT (avec hystérésis)
// ═══════════════════════════════════════════════════════════
class ChangeDetector
{
private:
    float lastValue;
    float threshold;
    bool firstReading = true;

public:
    explicit ChangeDetector(float threshold = 1.f)
        : threshold(threshold), lastValue(0) {}

    bool hasChanged(float v)
    {
        if (firstReading)
        {
            lastValue = v;
            firstReading = false;
            return true;
        }
        if (fabsf(v - lastValue) >= threshold)
        {
            lastValue = v;
            return true;
        }
        return false;
    }

    float getLastValue() const { return lastValue; }
    void reset() { firstReading = true; }
    void setThreshold(float t) { threshold = t; }
};

// ═══════════════════════════════════════════════════════════
// LED MANAGER  — corrigé pour ESP32 (ledc au lieu d'analogWrite)
// ═══════════════════════════════════════════════════════════
class LEDManager
{
public:
    enum Pattern
    {
        OFF,
        ON,
        BLINK_SLOW,
        BLINK_FAST,
        PULSE,
        HEARTBEAT
    };

private:
    int pin;
    int ledcChannel; // canal LEDC pour PWM
    bool state = false;
    unsigned long lastToggle = 0;
    Pattern current = OFF;
    unsigned long patternStart = 0;

public:
    // ✅ Fix: ledcChannel requis pour PULSE sur ESP32
    LEDManager(int ledPin, int ledcCh = 0) : pin(ledPin), ledcChannel(ledcCh)
    {
        // LEDC : résolution 8 bits, 1 kHz
        ledcSetup(ledcChannel, 1000, 8);
        ledcAttachPin(pin, ledcChannel);
        ledcWrite(ledcChannel, 0);
    }

    void setPattern(Pattern p)
    {
        current = p;
        patternStart = millis();
        if (p == OFF)
            ledcWrite(ledcChannel, 0);
        if (p == ON)
            ledcWrite(ledcChannel, 255);
    }

    // Surcharge string pour compatibilité avec le code existant
    void setPattern(const String &p)
    {
        if (p == "off")
            setPattern(OFF);
        else if (p == "on")
            setPattern(ON);
        else if (p == "blink_slow")
            setPattern(BLINK_SLOW);
        else if (p == "blink_fast")
            setPattern(BLINK_FAST);
        else if (p == "pulse")
            setPattern(PULSE);
        else if (p == "heartbeat")
            setPattern(HEARTBEAT);
    }

    void update()
    {
        unsigned long now = millis();
        switch (current)
        {
        case OFF:
            break;
        case ON:
            break;

        case BLINK_SLOW:
            if (now - lastToggle > 1000)
            {
                state = !state;
                ledcWrite(ledcChannel, state ? 255 : 0);
                lastToggle = now;
            }
            break;

        case BLINK_FAST:
            if (now - lastToggle > 200)
            {
                state = !state;
                ledcWrite(ledcChannel, state ? 255 : 0);
                lastToggle = now;
            }
            break;

        case PULSE:
        {
            // ✅ Fix: ledcWrite au lieu d'analogWrite (inexistant sur ESP32 core < 3.x)
            float phase = (float)(now - patternStart) / 1000.f * PI;
            uint8_t brightness = (uint8_t)((sinf(phase) + 1.f) * 127.5f);
            ledcWrite(ledcChannel, brightness);
            break;
        }

        case HEARTBEAT:
        {
            unsigned long ph = (now - patternStart) % 2000;
            bool on = (ph < 100) || (ph > 200 && ph < 300);
            ledcWrite(ledcChannel, on ? 255 : 0);
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
    static String formatUptime(unsigned long ms)
    {
        unsigned long s = ms / 1000, m = s / 60, h = m / 60, d = h / 24;
        String r;
        if (d > 0)
            r += String(d) + "j ";
        if (h % 24 > 0)
            r += String(h % 24) + "h ";
        if (m % 60 > 0)
            r += String(m % 60) + "m ";
        r += String(s % 60) + "s";
        return r;
    }

    static String formatBytes(size_t bytes)
    {
        if (bytes < 1024)
            return String(bytes) + " B";
        if (bytes < 1024 * 1024)
            return String(bytes / 1024.f, 2) + " KB";
        if (bytes < 1024 * 1024 * 1024UL)
            return String(bytes / 1024.f / 1024.f, 2) + " MB";
        return String(bytes / 1024.f / 1024.f / 1024.f, 2) + " GB";
    }

    static String formatRSSI(int rssi)
    {
        const char *q = rssi > -50   ? "Excellent"
                        : rssi > -60 ? "Bon"
                        : rssi > -70 ? "Moyen"
                        : rssi > -80 ? "Faible"
                                     : "Très faible";
        return String(rssi) + " dBm (" + q + ")";
    }
};

// ═══════════════════════════════════════════════════════════
// SERIAL COMMANDER
// ═══════════════════════════════════════════════════════════
class SerialCommander
{
private:
    String buffer;
    std::function<void(String, String)> callback; // ✅ Fix: lambda-compatible

public:
    explicit SerialCommander(std::function<void(String, String)> cb) : callback(cb) {}

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
                buffer += c;
        }
    }

private:
    void processCommand(String input)
    {
        input.trim();
        int sp = input.indexOf(' ');
        String cmd = (sp == -1) ? input : input.substring(0, sp);
        String args = (sp == -1) ? "" : input.substring(sp + 1);
        cmd.toLowerCase();
        if (callback)
            callback(cmd, args);
    }
};

// ═══════════════════════════════════════════════════════════
// BUZZER
// ═══════════════════════════════════════════════════════════
class Buzzer
{
private:
    int pin;
    bool state = false;
    unsigned long lastToggle = 0;
    unsigned int interval = 0;

public:
    explicit Buzzer(int p) : pin(p) {}

    void begin()
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void setPattern(const String &mode)
    {
        if (mode == "off")
        {
            interval = 0;
            digitalWrite(pin, LOW);
        }
        else if (mode == "alert")
        {
            interval = 100;
        }
        else if (mode == "warning")
        {
            interval = 500;
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