/*
* Meteo en direct via l'API d'Open Wearher Map sur WIFI KIT 32 de Heltec
* Auteur : Johann Brochier
* Brochier.IO
* V0.4 2020
*/
 
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_wps.h"
#include <HTTPClient.h>
#include "images.h"
#include "heltec.h"

#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"
#define Fbattery    3700  //Une batterie à 100% fait 3700mv

static esp_wps_config_t config;

const char* ssid = "......";  // SSID Wifi
const char* password =  "......"; // Mot de passe Wifi
 
const String endpoint = "https://api.openweathermap.org/data/2.5/weather?q=......,..&units=metric&lang=fr&appid="; // URL vers l'API OWM
const String key = "......"; // Cle API de OWM

int zeroBattery = 1930; // seuil bas, batterie à 2,6v
double averageBattery = 100; // energie moyenne de la batterie initialisée à 100 pour le premier démarrage

int cycle = 0; // Init du cycle pour la mise en veille

void wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

String wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void WiFiEvent(WiFiEvent_t event, system_event_info_t info){
  switch(event){
    case SYSTEM_EVENT_STA_START:
      Serial.println("Mode station");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Connecté à :" + String(WiFi.SSID()));
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Déconnecté, en attente de reconnexion");
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Heltec.display -> clear();
      Heltec.display -> drawString(64,16,"Succès WPS");
      Heltec.display -> drawString(64,40,"Patientez...");
      Heltec.display -> display();
      Serial.println("Succès WPS, arrêt de WPS et connexion à: " + String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("Erreur WPS, nouvel essai");
      Heltec.display -> clear();
      Heltec.display -> drawString(64,32,"Erreur WPS");
      Heltec.display -> display();
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("Délais WPS dépassé, nouvel essai");
      Heltec.display -> clear();
      Heltec.display -> drawString(64,32,"Erreur WPS");
      Heltec.display -> display();
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
}

void logo(){
  Heltec.display -> clear();
  Heltec.display -> drawXbm(0,0,logo_bcrio_width,logo_bcrio_height,(const unsigned char *)logo_bcrio_bits);
  Heltec.display -> display();
}
 
void wifiConnect() {
  WiFi.begin(ssid, password);
  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10) {
    delay(500);
    count++;
    Heltec.display -> drawString(0, 0, "Connexion au Wifi...");
    Heltec.display -> display();
  }
  Heltec.display -> clear();
  if(WiFi.status() == WL_CONNECTED)
    {
      Heltec.display -> drawString(0, 0, "Connexion au Wifi...OK.");
      Heltec.display -> display();
    } else {
      Heltec.display -> clear();
      Heltec.display -> drawString(0, 0, "Connexion au Wifi...erreur.");
      Heltec.display -> display();
    while(1);
    }
  delay(500);
  Heltec.display -> clear();
}

void wifiConnectWPS () {
  Heltec.display -> clear();
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);  
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(64,11,"Appuyez sur");
  Heltec.display->drawString(64,27,"le bouton WPS");
  Heltec.display->drawString(64,43,"de votre routeur...");
  Heltec.display->display();
  Serial.println("Démarrage de WPS");
  wpsInitConfig();
  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);  
}

void displayWeather(String payload){
        Heltec.display -> clear(); // on efface l'affichage
        StaticJsonDocument<2048> doc; 
        auto error = deserializeJson(doc, payload); // debut du parsing JSON
        if (error) {
            Serial.print(F("deserializeJson() Erreur code : "));
            Serial.println(error.c_str());
            return;
        } else {
            const char* location = doc["name"]; // Le lieu
            Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
            Heltec.display->setFont(ArialMT_Plain_16);
            Heltec.display -> drawString(64, 0, location);
            Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
            Heltec.display->setFont(ArialMT_Plain_10);
            JsonObject data = doc["main"];
            float temperature = data["temp"];
            //float feels = data["feels_like"];
            int mini = data["temp_min"];
            int maxi = data["temp_max"];
            int pressure = data["pressure"];
            int humidity = data["humidity"];
            Heltec.display -> drawString(0, 14, "Temp : " + (String)temperature + "°C");
            Heltec.display -> drawString(0, 24, "Min : " + (String)mini + "°C");
            Heltec.display -> drawString(0, 34, "Max : " + (String)maxi + "°C");
            Heltec.display -> drawString(0, 44, "Pression : " + (String)pressure + " hPa");
            Heltec.display -> drawString(0, 54, "Humidite : " + (String)humidity + "%");
            JsonObject weather = doc["weather"][0];
            String sky = weather["icon"];
            displayIcon(sky);
            signalBars(); // affichage de la qualité du signal wifi
            batteryPower(); // affiche le niveau de batterie            
            Heltec.display -> display(); // Affichage de l'écran météo   
        }
}

void displayIcon(String sky) {
  String sk = sky.substring(0,2);
  const char* icon;
  switch (sk.toInt()) {
    case 1:
      icon = one_bits;
      break;
    case 2:
      icon = two_bits;
      break;
    case 3:
      icon = three_bits;
      break;
    case 4:
      icon = four_bits;
      break;
    case 9:
      icon = nine_bits;
      break;
    case 10:
      icon = ten_bits;
      break;
    case 11:
      icon = eleven_bits;
      break;
    case 13:
      icon = thirteen_bits;
      break;
    case 50:
      icon = fifty_bits;
      break;  
    default:
      icon = nothing_bits; // pas d'icône trouvée
      break;
  }
  Heltec.display -> drawXbm(80,15,30,30,(const unsigned char *)icon);
}

void signalBars () {
      long rssi = WiFi.RSSI();
      int bars;
      if (rssi > -55) { 
        bars = 5;
      } else if (rssi < -55 & rssi > -65) {
        bars = 4;
      } else if (rssi < -65 & rssi > -70) {
        bars = 3;
      } else if (rssi < -70 & rssi > -78) {
        bars = 2;
      } else if (rssi < -78 & rssi > -82) {
        bars = 1;
      } else {
        bars = 0;
      }
      for (int b=0; b <= bars; b++) {
        Heltec.display->fillRect(110 + (b*3),50 - (b*2),2,b*2); 
      }
}

void batteryPower () {
  Heltec.display->drawRect(110,56,18,8);
  Heltec.display->fillRect(110,56,0.18*averageBattery,8);
}
double averagePower() {
  double total = 0;
  for (int i=0;i<=11;i++) {
    uint16_t lec  =  analogRead(37);
    Serial.println("Lecture : " + (String)lec);
    double delta = (lec-zeroBattery);
    Serial.println("delta : " + (String)delta);
    double percent = (delta/810)*100;
    if ( percent >= 100 ) {
      percent = 100;
    }
    Serial.println("pourcentage : " + (String)percent);
    total=total+percent;
    delay(2000);
  }
  double averagePower = total/12;
  Serial.println("averagePower : " + (String)averagePower);
  return averagePower;
}

void lightSleep () {
  Serial.println("light sleep");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  esp_sleep_enable_gpio_wakeup();
  Heltec.display -> displayOff();
  int ls = esp_light_sleep_start();
  Serial.printf("light_sleep: %d\n", ls);
  Heltec.display -> displayOn();
}

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);  // Activalion de l'écran OLED et de la liaison série
  logo();  // Affiche le "splash screen"
  delay(2000);
  Heltec.display->clear();
  adcAttachPin(37); // Lecture du niveau de la batterie sur le GPIO37
  analogSetClockDiv(255); // 1338mS 
  Serial.begin(115200);
  /*
   * Choisr SOIT wifiConnect() SOIT wifiConnectWPS();
   */
  //wifiConnect(); // Choisir wifiConnect() pour une connexion Wifi avec les identifiants Wifi dans le code
  wifiConnectWPS(); // Choisir wifiConnectWPS() pour une connexion Wifi en WPS
}
 
void loop() {
  bool errorHttp = false; 
  if ((WiFi.status() == WL_CONNECTED)) { // Vérification du status Wifi  
    HTTPClient http;
    http.begin(endpoint + key); // URL vers l'API
    int httpCode = http.GET(); 
    if (httpCode > 0) { // Vérification du code HTTP retourné
        String payload = http.getString();
        Serial.println("Code http : " + (String)httpCode);
        Serial.println(payload);       
        displayWeather(payload);
    } else {
      Heltec.display -> clear();
      Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
      Heltec.display->setFont(ArialMT_Plain_16);
      Heltec.display -> drawString(64,4,"Reconnexion");
      Heltec.display -> drawString(64,20,"Au service météo");
      Heltec.display -> drawString(64,36,"Patientez...");
      Heltec.display -> display();
      cycle=0; // On ne repasse pas en veille tant que nous sommes en erreur de requête http
      Serial.println("Erreur requete HTTP");
      errorHttp = true;        
    }
    http.end(); // libération des ressources
    if (!errorHttp) { //Si la connexion est en erreur on n'effectue pas le calcul de la moyenne batterie pour ne pas perdre 28 secondes
      averageBattery=averagePower(); // Calcul du niveau de batterie en faisant une moyenne sur 28 secondes
    }
  } else {
    cycle=0; // on ne passe pas en veille tant que la connexion wifi n'est pas établie
  }
  delay(2000);
  cycle++;
  Serial.println("Cycle = " + (String)cycle);
  if (cycle >= 3) { //Déclenchement de la veille au bout de la 3ème boucle
    cycle=0;
    lightSleep();
  } 
}
