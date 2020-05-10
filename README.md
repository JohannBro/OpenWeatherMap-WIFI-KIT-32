# OpenWeatherMap-WIFI-KIT-32
API REST OWM on ESP32 with OLED Display (Heltec WIFI KIT 32)
https://youtu.be/elu4MmB6_aU

# Intégration
Etude de l'intégration de l'API d'OWM sur l'ESP32 d'Heltec équipé d'un écran OLED de 0,96 pouces.

# Librairies
```
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "images.h"
#include "heltec.h"
```
Utilisation de la librairie ArduinoJson version 6.x

# Configuration
```
const char* ssid = "......";  // SSID du Wifi
const char* password =  "......"; // Mot de passe du Wifi
 
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=......,..&units=metric&lang=fr&appid="; // URL vers l'API OWM
const String key = "......"; // Cle API de OWM
```
dans le setup()
Choix de connexion Wifi (Hard coded ou WPS)
```
//wifiConnect(); // Choisir wifiConnect() pour une connection Wifi avec les identifiants Wifi
wifiConnectWPS(); // Choisir wifiConnectWPS() pour une connection Wifi en WPS
```

# A faire
Gestion de la batterie
