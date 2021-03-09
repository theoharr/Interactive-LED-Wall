/*
 * mostly stolen from 
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 * and
 * https://github.com/thehookup/Alexa-Enabled-LED-Clock/blob/master/Alexa_Clock_CONFIGURE.ino
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <Espalexa.h>       //https://github.com/Aircoookie/Espalexa
#include <arduino-timer.h>  //https://github.com/contrem/arduino-timer

#define SERIAL_BAUDRATE 115200

#define WIFI_SSID "TODO"
#define WIFI_PASS "TODO"
#define ALEXA_DEVICE_NAME "Cool Project"

#define USER_MQTT_SERVER          "UNUSED"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "UNUSED"
#define USER_MQTT_PASSWORD        "UNUSED"
#define USER_MQTT_CLIENT_NAME     ALEXA_DEVICE_NAME

const char* ssid = WIFI_SSID ;
const char* password = WIFI_PASS ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ;

//WiFiClient espClient;
//PubSubClient client(espClient);
auto timer = timer_create_default();
Espalexa espalexa;
boolean connectWifi();
boolean wifiConnected = false;

boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 40) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}

bool helloworld(void *)
{
  Serial.println("'ello world.");
  return true; // keep timer active
}

void colorLightChanged1(uint8_t brightnessCommand, uint32_t rgb)
{
  Serial.println("OMG...its working!?");
  Serial.println(brightnessCommand);
  Serial.println(rgb);
}

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  wifiConnected = connectWifi();
  if (wifiConnected)
  {
    espalexa.addDevice("LED Grid", colorLightChanged1);
    espalexa.begin();
  }
  else
  {
    delay(5000);
    ESP.restart();
  }
  //ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  //ArduinoOTA.begin();
  timer.every(10000, helloworld);
}


void loop() {
  espalexa.loop();
  //ArduinoOTA.handle();

  timer.tick();
}
