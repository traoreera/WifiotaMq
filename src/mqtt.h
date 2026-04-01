#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "utilities.h"

extern bool wifi_connected;
extern Logger logger;
extern void mqttCallback(char *topic, byte *payload, unsigned int length);

// ─── Timeouts (ajustables) ───────────────────────────────
#define MQTT_SOCKET_TIMEOUT_S 5      // timeout socket TLS (secondes)
#define MQTT_CONNECT_TIMEOUT_MS 5000 // garde-fou connexion (ms)
#define MQTT_RECONNECT_MIN_MS 2000   // backoff minimal
#define MQTT_RECONNECT_MAX_MS 60000  // backoff maximal

class MQTTController
{
private:
    const char *mqtt_server;
    int mqtt_port;
    const char *mqtt_user;
    const char *mqtt_password;

    WiFiClientSecure secureClient;
    PubSubClient client;

    String publishTopic = "";
    String subscribeTopic = "";
    String currentSubscribed = "";

    unsigned long lastReconnectAttempt = 0;
    unsigned long reconnectInterval = MQTT_RECONNECT_MIN_MS;

    String clientId = "ESPClient";
    bool connecting = false; // ← garde re-entrant

public:
    bool isSecure = false;

    MQTTController(const char *srv, int port,
                   const char *user, const char *pass)
        : mqtt_server(srv), mqtt_port(port),
          mqtt_user(user), mqtt_password(pass),
          client(secureClient) {}

    // ── begin ────────────────────────────────────────────
    void begin()
    {
        // Timeout socket AVANT toute connexion
        secureClient.setTimeout(MQTT_SOCKET_TIMEOUT_S);

        client.setServer(mqtt_server, mqtt_port);
        client.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S); // PubSubClient >= 2.8
        client.setKeepAlive(15);                        // keepalive 15 s
        client.setCallback(mqttCallback);

        if (wifi_connected)
        {
            connectMQTT();
        }
        else
        {
            logger.warning("[MQTT] begin() : WiFi non connecté, connexion différée.");
        }
    }

    // ── loop : NON-BLOQUANT ──────────────────────────────
    void loop()
    {
        if (!wifi_connected)
            return;

        if (client.connected())
        {
            client.loop();
            return;
        }

        // Backoff exponentiel
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= reconnectInterval)
        {
            lastReconnectAttempt = now;
            if (connectMQTT())
            {
                reconnectInterval = MQTT_RECONNECT_MIN_MS; // reset
            }
            else
            {
                reconnectInterval = min(reconnectInterval * 2UL,
                                        (unsigned long)MQTT_RECONNECT_MAX_MS);
                logger.warning("[MQTT] Prochain essai dans " +
                               String(reconnectInterval / 1000) + "s");
            }
        }
    }

    // ── publish ──────────────────────────────────────────
    bool publish(const char *topic, const String &message)
    {
        if (!wifi_connected || !client.connected())
        {
            logger.error("[MQTT] Publish ignoré (non connecté).");
            return false;
        }
        bool ok = client.publish(topic, message.c_str());
        if (ok)
            logger.info("[MQTT] Publié [" + String(topic) + "] : " + message);
        else
            logger.error("[MQTT] Échec publish [" + String(topic) + "]");
        return ok;
    }

    bool publish(const String &message)
    {
        return publish(publishTopic.c_str(), message);
    }

    // ── setters topics ───────────────────────────────────
    void setPublishTopic(const String &topic)
    {
        publishTopic = topic;
        logger.info("[MQTT] Publish topic : " + publishTopic);
    }

    void setSubscribeTopic(const String &topic)
    {
        if (topic.isEmpty() || subscribeTopic == topic)
            return;
        subscribeTopic = topic;
        logger.info("[MQTT] Subscribe topic demandé : " + topic);

        if (!client.connected())
        {
            logger.info("[MQTT] Abonnement différé (non connecté).");
            return;
        }
        _applySubscription();
    }

    void setClientId(const String &id)
    {
        if (!id.isEmpty())
            clientId = id;
        logger.info("[MQTT] clientId : " + clientId);
    }

    // ── TLS ─────────────────────────────────────────────
    void setSecure(const char *caCert)
    {
        secureClient.setCACert(caCert);
        isSecure = true;
        logger.info("[MQTT] CA cert configuré.");
    }

    // ── getters ──────────────────────────────────────────
    bool isConnected() { return client.connected(); }
    String getPublishTopic() const { return publishTopic; }
    String getSubscribeTopic() const { return subscribeTopic; }

    // ── déconnexion propre ───────────────────────────────
    void disconnect()
    {
        if (client.connected())
            client.disconnect();
        logger.info("[MQTT] Déconnecté proprement.");
    }

private:
    // ── connexion (gardée contre re-entrance) ────────────
    bool connectMQTT()
    {
        if (!wifi_connected)
            return false;
        if (connecting)
            return false; // déjà en cours
        connecting = true;

        if (!isSecure)
            secureClient.setInsecure();

        logger.info("[MQTT] Connexion à " + String(mqtt_server) +
                    ":" + String(mqtt_port) + " ...");

        bool ok = false;

        // client.connect() peut quand même bloquer ≤ MQTT_SOCKET_TIMEOUT_S
        ok = client.connect(clientId.c_str(), mqtt_user, mqtt_password);

        if (ok)
        {
            logger.info("[MQTT] Connecté !");
            _applySubscription();
            if (!publishTopic.isEmpty())
                client.publish(publishTopic.c_str(), "ESP connected");
        }
        else
        {
            // client.state() retourne un int → String() obligatoire
            logger.error("[MQTT] Échec connexion, state=" +
                         String(client.state()));
        }

        connecting = false;
        return ok;
    }

    // ── (re)abonnement ───────────────────────────────────
    void _applySubscription()
    {
        if (subscribeTopic.isEmpty())
            return;

        // Désabonner l'ancien topic si différent
        if (!currentSubscribed.isEmpty() &&
            currentSubscribed != subscribeTopic)
        {
            if (client.unsubscribe(currentSubscribed.c_str()))
                logger.info("[MQTT] Désabonné de : " + currentSubscribed);
            else
                logger.error("[MQTT] Échec désabonnement : " + currentSubscribed);
        }

        if (client.subscribe(subscribeTopic.c_str()))
        {
            currentSubscribed = subscribeTopic;
            logger.info("[MQTT] Abonné à : " + subscribeTopic);
        }
        else
        {
            logger.error("[MQTT] Échec abonnement : " + subscribeTopic);
        }
    }
};

#endif // MQTT_H