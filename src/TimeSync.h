#pragma once
#include <Arduino.h>
#include <time.h>

namespace TimeSync
{
    // ===============================
    //  Paramètres NTP et Fuseau horaire
    // ===============================
    static const char *NTP_SERVER = "time.google.com";
    static const unsigned long TIMEOUT_MS = 20000; // 20 sec
    static long gmtOffset_sec = 0;                 // Décalage GMT (en secondes)
    static int daylightOffset_sec = 0;             // Offset heure d’été (en secondes)

    // ===============================
    //  Initialisation de l'heure
    // ===============================
    inline void begin(long gmtOffset = 0, int daylightOffset = 0)
    {
        gmtOffset_sec = gmtOffset;
        daylightOffset_sec = daylightOffset;

        configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
        Serial.print("[TimeSync] Waiting for time sync");

        unsigned long start = millis();
        time_t now = 0;

        while (millis() - start < TIMEOUT_MS)
        {
            time(&now);
            if (now > 100000)
            {
                Serial.println("\n[TimeSync] Time synchronized.");
                return;
            }
            Serial.print(".");
            delay(500);
        }

        Serial.println("\n[TimeSync] Failed to sync time within timeout.");
    }

    // ===============================
    //  Timestamp complet
    // ===============================
    inline String timestamp()
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        char buffer[30];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    }

    // ===============================
    //  Heure simplifiée
    // ===============================
    inline String shortTime()
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        char buffer[12];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }

    // ===============================
    //  Infos de débogage
    // ===============================
    inline void info()
    {
        Serial.println("===== TimeSync Info =====");
        Serial.printf("NTP Server: %s\n", NTP_SERVER);
        Serial.printf("GMT Offset: %ld sec (%.1f h)\n", gmtOffset_sec, gmtOffset_sec / 3600.0);
        Serial.printf("DST Offset: %d sec (%.1f h)\n", daylightOffset_sec, daylightOffset_sec / 3600.0);
        Serial.printf("Current Time: %s\n", timestamp().c_str());
        Serial.println("=========================");
    }
}
