# 🚀 Projet ESP32 Complet - Guide de Démarrage

## 📦 Contenu du Package

Vous disposez maintenant d'un système complet pour ESP32 avec WiFi, MQTT et OTA.

### 📁 Structure des fichiers

```
esp32-project/
├── src/
│   ├── WiFiManagerOTA.h       # Gestionnaire WiFi/Web/OTA
│   ├── MQTT.h                 # Contrôleur MQTT
│   └── utilities.h            # 12 classes utilitaires
├── exemples/
│   ├── simples                # Version de base
│   ├── otamqttinsecure        # Version avec mqtt insecures
│   └── secure                 # Version securisée
├── docs/
```

## ⚡ Démarrage Rapide (5 minutes)

### Étape 1: Créer le projet

```bash
# Créer un nouveau projet PlatformIO
pio project init --board esp32dev

# Ou via VS Code: PlatformIO > New Project
```

### Étape 2: Copier les fichiers

```
Copiez dans include/:
  - WiFiManagerOTA.h
  - MQTT.h
  - utilities.h

Copiez dans src/:
  - main.cpp (ou advanced_main.cpp)

Copiez à la racine:
  - platformio.ini
```

### Étape 3: Compiler et téléverser

```bash
# Compiler
pio run

# Téléverser
pio run --target upload

# Moniteur série
pio device monitor
```

### Étape 4: Configuration initiale

1. **L'ESP32 démarre en mode AP**
   ```
   SSID: ESP32-Config
   Password: 12345678
   ```

2. **Connectez-vous au WiFi** et ouvrez
   ```
   http://192.168.4.1
   User: admin
   Pass: admin123
   ```

3. **Configurez le WiFi**
   - Cliquez sur "⚙️ WiFi"
   - Sélectionnez votre réseau
   - Entrez le mot de passe
   - Topic: `home/sensor/`
   - User ID: `device001`

4. **Configurez MQTT**
   - Cliquez sur "📡 MQTT"
   - Hostname: votre broker MQTT
   - Port: 8883 (SSL) ou 1883
   - User/Password MQTT
   - Client ID unique

5. **Redémarrage automatique**
   - L'ESP32 se connecte au WiFi
   - Se connecte au broker MQTT
   - Commence à publier

## 🎯 Fonctionnalités Principales

### ✅ WiFi & Web Interface
- **Configuration WiFi** via interface web
- **Scan WiFi** avec sélection du réseau
- **Point d'accès** de secours automatique
- **mDNS**: `http://esp32-device.local`
- **Authentification** sur toutes les pages
- **Design moderne** et responsive

### ✅ MQTT
- **Configuration** via interface web
- **SSL/TLS** support (port 8883)
- **Reconnexion automatique** avec backoff
- **Queue de retry** pour messages perdus
- **Topics dynamiques** configurables
- **Callbacks** pour commandes

### ✅ OTA (Over-The-Air)
- **ElegantOTA** intégré
- **Upload firmware** via navigateur
- **Protégé** par authentification
- **Accès**: `http://[IP]/update`

### ✅ Utilitaires Avancés (12 classes)
1. **CircularBuffer**: Queue FIFO générique
2. **Statistics**: Min, max, moyenne, écart-type
3. **SoftwareWatchdog**: Surveillance système
4. **TaskScheduler**: Tâches périodiques
5. **LowPassFilter**: Filtrage de signal
6. **ChangeDetector**: Détection changements
7. **LEDManager**: Patterns LED (heartbeat, pulse, etc.)
8. **ConfigManager**: Sauvegarde flash
9. **Logger**: Logs multi-niveaux + MQTT
10. **MQTTHelper**: Retry automatique
11. **TimeFormatter**: Formatage affichage
12. **SerialCommander**: Commandes série

## 📡 Exemples d'utilisation

### Exemple 1: Version de base (main.cpp)

```cpp
#include "WiFiManagerOTA.h"
#include "MQTT.h"

bool wifi_connected = false;
WiFiManagerOTA wifiManager(80, "admin", "admin123");
MQTTController* mqttController = nullptr;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg = String((char*)payload).substring(0, length);
    if (msg == "status") {
        mqttController->publish("{\"status\":\"ok\"}");
    }
}

void setup() {
    Serial.begin(115200);
    wifiManager.begin("esp32-device", "ESP32-Config", "12345678");
    
    if (wifi_connected) {
        auto cfg = wifiManager.getMqttConfig();
        mqttController = new MQTTController(
            cfg.hostname.c_str(), cfg.port,
            cfg.user.c_str(), cfg.password.c_str()
        );
        mqttController->setClientId(cfg.client);
        mqttController->setPublishTopic(wifiManager.pubTopic("v1/"));
        mqttController->setSubscribeTopic(wifiManager.cmdTopic("v1/", "/cmd"));
        mqttController->begin();
    }
}

void loop() {
    wifiManager.loop();
    wifiManager.handleWiFiReconnect();
    if (wifi_connected && mqttController) {
        mqttController->loop();
    }
    delay(10);
}
```

### Exemple 2: Avec utilitaires (advanced_main.cpp)

Version complète avec logger, scheduler, watchdog, statistiques, etc.
Voir le fichier `advanced_main.cpp` pour l'implémentation complète.

### Exemple 3: Capteur DHT22 (examples.cpp)

```cpp
#define EXAMPLE_DHT22
#include "examples.cpp"
```

### Exemple 4: Contrôle de relais (examples.cpp)

```cpp
#define EXAMPLE_RELAY_CONTROL
#include "examples.cpp"
```

## 🔧 Configuration Avancée

### Changer les identifiants OTA

```cpp
WiFiManagerOTA wifiManager(80, "votre_user", "votre_password");
```

### Changer le hostname mDNS

```cpp
wifiManager.begin("mon-esp32", "ESP32-Config", "12345678");
// Accès: http://mon-esp32.local
```

### Personnaliser les topics MQTT

```cpp
// Dans /config de l'interface web:
Topic: home/sensor/           // Préfixe
User ID: salon                // Suffixe

// Résultat:
// Pub: home/sensor/v1/salon
// Sub: home/sensor/v1/salon/cmd
```

### Activer SSL/TLS MQTT

```cpp
// Dans MQTT.h, modifier connectMQTT():
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
[Votre certificat CA]
-----END CERTIFICATE-----
)EOF";

// Remplacer:
secureClient.setInsecure();
// Par:
secureClient.setCACert(root_ca);
```

## 📊 Commandes MQTT Disponibles

Publiez ces commandes sur le topic: `home/sensor/v1/device001/cmd`

| Commande | Description | Réponse |
|----------|-------------|---------|
| `status` | Statut système complet | JSON avec infos |
| `stats` | Statistiques | JSON statistiques |
| `reset_stats` | Reset statistiques | Confirmation |
| `ping` | Test connectivité | `{"response":"pong"}` |
| `reboot` | Redémarrer | Redémarrage ESP32 |
| `led:on` | LED allumée | Confirmation |
| `led:off` | LED éteinte | Confirmation |
| `led:heartbeat` | Pattern heartbeat | Confirmation |
| `led:blink_fast` | Clignotement rapide | Confirmation |
| `interval:30000` | Changer intervalle | Nouvel intervalle |
| `log:debug` | Niveau log debug | Confirmation |

## 💻 Commandes Série Disponibles

Tapez dans le moniteur série:

| Commande | Description |
|----------|-------------|
| `help` | Afficher l'aide |
| `status` | Statut système |
| `stats` | Statistiques |
| `wifi` | Informations WiFi |
| `mqtt` | Informations MQTT |
| `tasks` | Liste des tâches |
| `led <pattern>` | Changer LED |
| `config` | Configuration |
| `reset` | Redémarrer |

## 🔍 Monitoring & Débogage

### Moniteur série

```bash
pio device monitor --baud 115200
```

### Logs détaillés

Dans `platformio.ini`:
```ini
build_flags = 
    -D DEBUG_ESP_PORT=Serial
    -D DEBUG_ESP_WIFI
    -D CORE_DEBUG_LEVEL=5
```

### Page de statut JSON

Accédez à: `http://[IP]/status`

Retourne:
```json
{
  "ssid": "MonWiFi",
  "ip": "192.168.1.100",
  "rssi": -65,
  "uptime": "2j 5h 30m",
  "freeHeap": 45231,
  "chipModel": "ESP32-D0WDQ6",
  "cpuFreq": 240
}
```

## 🐛 Dépannage

### L'ESP32 ne se connecte pas au WiFi

**Symptômes**: Reste en mode AP

**Solutions**:
1. Vérifier SSID et mot de passe
2. Réseau en 2.4 GHz (pas 5 GHz)
3. Signal suffisamment fort
4. Reset config: `http://[IP]/reset`

### MQTT ne se connecte pas

**Symptômes**: Pas de messages publiés

**Solutions**:
1. Vérifier broker accessible
2. Port correct (1883 ou 8883)
3. Identifiants valides
4. Vérifier logs série: `MQTT State: -4`
   - `-4`: Timeout
   - `-3`: Connection lost
   - `-2`: Connect failed
   - `-1`: Disconnected

### Mémoire insuffisante

**Symptômes**: Crashes, reboots aléatoires

**Solutions**:
```cpp
// Surveiller:
Serial.println(ESP.getFreeHeap());

// Si < 50KB:
// - Réduire taille buffers
// - Utiliser PROGMEM
// - Libérer objets inutilisés
```

### OTA ne fonctionne pas

**Symptômes**: Upload échoue

**Solutions**:
1. WiFi stable
2. Firmware pas trop gros
3. Partition scheme: `default.csv`
4. Essayer depuis `/update` direct

## 📚 Documentation Complète

- **README.md**: Vue d'ensemble et démarrage
- **INTEGRATION_GUIDE.md**: Intégration WiFiManager + MQTT
- **UTILITIES_README.md**: Guide des 12 utilitaires

## 🎓 Exemples Avancés

### 1. Système complet avec tous les utilitaires
Voir: `advanced_main.cpp`

### 2. Capteur DHT22 + MQTT
Voir: `examples.cpp` → `EXAMPLE_DHT22`

### 3. Contrôle de relais
Voir: `examples.cpp` → `EXAMPLE_RELAY_CONTROL`

### 4. Moniteur système avec alertes
Voir: `examples.cpp` → `EXAMPLE_SYSTEM_MONITOR`

### 5. Multi-capteurs avec cache
Voir: `examples.cpp` → `EXAMPLE_MULTI_SENSOR`

### 6. Deep Sleep pour économie batterie
Voir: `examples.cpp` → `EXAMPLE_DEEP_SLEEP`

## 🚀 Prochaines Étapes

1. **Testez la version de base** (`main.cpp`)
2. **Configurez WiFi et MQTT** via l'interface web
3. **Testez les commandes MQTT** depuis un client
4. **Explorez les exemples** dans `examples.cpp`
5. **Intégrez les utilitaires** nécessaires
6. **Personnalisez** selon vos besoins

## 🔒 Sécurité - Points Importants

### En production:

✅ **Changez les identifiants par défaut**
```cpp
WiFiManagerOTA wifiManager(80, "votre_user", "MotDePasseSecurise123!");
```

✅ **Activez SSL/TLS pour MQTT**
```cpp
secureClient.setCACert(root_ca);
```

✅ **Désactivez les logs debug**
```ini
# Commentez dans platformio.ini:
# -D DEBUG_ESP_PORT=Serial
```

✅ **Utilisez des certificats valides**
```cpp
// Pas de setInsecure() en production
```

## 📈 Optimisations Performance

### Mémoire
```cpp
// Utiliser PROGMEM pour HTML
const char html[] PROGMEM = "...";

// Libérer après scan WiFi
WiFi.scanDelete();

// CircularBuffer limité
CircularBuffer<String, 10> queue; // Pas 100
```

### Réseau
```cpp
// Publier seulement si changement
if (changeDetector.hasChanged(value)) {
    publish(value);
}

// Queue MQTT pour retry
mqttHelper.publishWithRetry(topic, msg);
```

### CPU
```cpp
// TaskScheduler au lieu de delay()
scheduler.addTask("sensor", 1000, readSensor);

// Filtrer les données
float filtered = lowPassFilter.filter(raw);
```

## 🎉 Félicitations !

Vous avez maintenant un système ESP32 complet, robuste et professionnel avec:

- ✅ Configuration web intuitive
- ✅ MQTT fiable avec retry
- ✅ OTA pour mises à jour
- ✅ 12 utilitaires puissants
- ✅ Exemples pratiques
- ✅ Documentation complète

**Bon développement ! 🚀**

---

## 📞 Support

Pour toute question:
1. Consultez la documentation
2. Vérifiez les exemples
3. Examinez les logs série
4. Testez les commandes de debug

## 📄 Licence

Ce projet est sous licence MIT. Libre d'utilisation, modification et distribution.

---

**Créé avec ❤️ pour la communauté ESP32**