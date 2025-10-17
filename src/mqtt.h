#ifndef MQTT_H
#define MQTT_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "utilities.h"



extern bool wifi_connected;
extern Logger logger;
extern void mqttCallback(char* topic, byte* payload, unsigned int length);

class MQTTController {
  private:
    const char* mqtt_server;
    int mqtt_port;
    const char* mqtt_user;
    const char* mqtt_password;

    WiFiClientSecure secureClient;
    PubSubClient client;

    String publishTopic = "";    // topic utilisé pour publish par défaut
    String subscribeTopic = "";  // topic utilisé pour subscribe par défaut
    String currentSubscribed = "";// topic auquel on est réellement abonné (pour unsubscribe si changement)

    // reconnexion
    unsigned long lastReconnectAttempt = 0;
    unsigned long reconnectInterval = 2000;  // 2s initial

    String clientId = "ESPClient";

  public:
    bool isSecure = false;
    MQTTController(const char* mqtt_server, int mqtt_port, const char* mqtt_user, const char* mqtt_password)
      : mqtt_server(mqtt_server), mqtt_port(mqtt_port), mqtt_user(mqtt_user), mqtt_password(mqtt_password), client(secureClient) {}

    // begin: prépare le client, n'oublie pas d'appeler setPublishTopic/setSubscribeTopic avant si tu veux
    void begin() {
      client.setServer(mqtt_server, mqtt_port);
      client.setCallback(mqttCallback);
      // si le Wi-Fi est déjà connecté, tenter une première connexion
      if (wifi_connected) {
        connectMQTT();
      } else {
        logger.error("MQTT not started (Wi-Fi non connecté). Appelle begin() après connexion ou laissez la loop gérer la reconnexion.");
      }
    }

    // loop: doit être appelé dans loop()
    void loop() {
      if (!wifi_connected) return;

      if (client.connected()) {
        client.loop();
      } else {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectInterval) {
          lastReconnectAttempt = now;
          if (connectMQTT()) {
            reconnectInterval = 2000; // reset backoff
          } else {
            reconnectInterval = min(reconnectInterval * 2UL, 60000UL);
          }
        }
      }
    }

    // publish sur le topic courant
    bool publish(const char* topic, const String& message) {
      if (wifi_connected && client.connected()) {
        bool ok = client.publish(topic, message.c_str());
        if (ok) logger.info("MQTT published [" + String(topic) + "] : " + message);

        else logger.error("MQTT publish failed [" + String(topic) + "]");
        return ok;
      } else {
        logger.error("MQTT non connecté. Publish ignoré.");
        return false;
      }
    }

    // publish sur le topic de publication configuré
    bool publish(const String& message) {
      return publish(publishTopic.c_str(), message);
    }

    // setters dynamiques pour topics
    void setPublishTopic(const String& topic) {
      publishTopic = topic;
      logger.info(" Publish topic set to: " + publishTopic);
    }

    // setSubscribeTopic : applique la subscription si déjà connecté (unsubscribe ancien si besoin)
    void setSubscribeTopic(const String& topic) {
      if (topic.length() == 0) return;
      if (subscribeTopic == topic) return; // pas de changement

      logger.info(" Subscribe topic requested: " + topic);
      subscribeTopic = topic;

      // Si connecté, unsubscribe ancien et subscribe nouveau
      if (client.connected()) {
        if (currentSubscribed.length() > 0 && currentSubscribed != subscribeTopic) {
          if (client.unsubscribe(currentSubscribed.c_str())) {
            logger.info(" Unsubscribed from: " + currentSubscribed);
          } else {
            logger.error(" Failed to unsubscribe from: " + currentSubscribed);
          }
        }
        if (client.subscribe(subscribeTopic.c_str())) {
          currentSubscribed = subscribeTopic;
          logger.info("Subscribed to: " + subscribeTopic);
        } else {
          logger.error(" Failed to subscribe to: " + subscribeTopic);
        }
      } else {
        logger.warning("Will subscribe to " + subscribeTopic + " once connected.");
      }
    }

    // option: changer clientId (avant connect)
    void setClientId(const String& id) {
      if (id.length() > 0) clientId = id;
      logger.info("MQTT clientId: " + clientId);
    }

    // geteurs
    String getPublishTopic() const { return publishTopic; }
    String getSubscribeTopic() const { return subscribeTopic; }

  
    void setSecure(const char* caCert){
      secureClient.setCACert(caCert);
      logger.info("CA cert set");
    }


  
  private:
    bool connectMQTT() {
      if (!wifi_connected) return false;
      
      if (!isSecure)secureClient.setInsecure(); //  en prod: utiliser setCACert()
      
      ///logger.info("Connexion au broker MQTT... ");
      if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
        logger.info("Connecté !");
        // subscribe au topic configuré
        if (subscribeTopic.length() > 0) {
          if (client.subscribe(subscribeTopic.c_str())) {
            currentSubscribed = subscribeTopic;
            logger.info("Abonné au topic : " + subscribeTopic);
          } else {
            logger.error(" Échec abonnement au topic : " + subscribeTopic);
          }
        }

        // message d'annonce facultatif
        if (publishTopic.length() > 0) {
          client.publish(publishTopic.c_str(), "ESP connected");
        }
        return true;
      } else {
        logger.critical("Échec connexion MQTT, code="+client.state());
        return false;
      }
    }
};

#endif
