#pragma once
#include <Arduino.h>
#include <time.h>

namespace TimeSync
{
    // ── Paramètres ────────────────────────────────────────
    constexpr const char *NTP_SERVER = "time.google.com";
    constexpr unsigned long TIMEOUT_MS = 20000;

    // Variables d'état (non-static pour éviter les surprises multi-TU)
    inline long gmtOffset_sec = 0;
    inline int daylightOffset_sec = 0;

    // ── Initialisation ────────────────────────────────────
    inline void begin(long gmtOffset = 0, int daylightOffset = 0)
    {
        gmtOffset_sec = gmtOffset;
        daylightOffset_sec = daylightOffset;

        configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
        Serial.print("[TimeSync] Synchronisation NTP");

        unsigned long start = millis();
        struct tm ti{};

        while (millis() - start < TIMEOUT_MS)
        {
            // ✅ Fix: vérifier l'année plutôt qu'un timestamp brut
            //    tm_year est le nombre d'années depuis 1900 ; > 100 = après l'an 2000
            if (getLocalTime(&ti) && ti.tm_year > 100)
            {
                Serial.println("\n[TimeSync] Heure synchronisée.");
                return;
            }
            Serial.print('.');
            delay(500);
        }

        Serial.println("\n[TimeSync] Échec de synchronisation (timeout).");
    }

    // ── Timestamp complet ─────────────────────────────────
    inline String timestamp()
    {
        struct tm ti{};
        if (!getLocalTime(&ti))
            return "N/A";

        char buf[24];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
        return String(buf);
    }

    // ── Heure courte ──────────────────────────────────────
    inline String shortTime()
    {
        struct tm ti{};
        if (!getLocalTime(&ti))
            return "N/A";

        char buf[10];
        strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
        return String(buf);
    }

    // ── Est-ce que l'heure est valide ? ───────────────────
    inline bool isSynced()
    {
        struct tm ti{};
        return getLocalTime(&ti) && ti.tm_year > 100;
    }

    // ── Debug ─────────────────────────────────────────────
    inline void info()
    {
        Serial.println("===== TimeSync Info =====");
        Serial.printf("NTP Server : %s\n", NTP_SERVER);
        Serial.printf("GMT Offset : %ld s (%.1f h)\n", gmtOffset_sec, gmtOffset_sec / 3600.0f);
        Serial.printf("DST Offset : %d s (%.1f h)\n", daylightOffset_sec, daylightOffset_sec / 3600.0f);
        Serial.printf("Synced     : %s\n", isSynced() ? "oui" : "non");
        Serial.printf("Heure      : %s\n", timestamp().c_str());
        Serial.println("=========================");
    }
}