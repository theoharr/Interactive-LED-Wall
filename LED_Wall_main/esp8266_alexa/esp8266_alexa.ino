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
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Espalexa.h>       //https://github.com/Aircoookie/Espalexa
#include <arduino-timer.h>  //https://github.com/contrem/arduino-timer

#define WIFI_SSID "tjhome"
#define WIFI_PASS "deadface07"
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
const char *alexa_device_name = ALEXA_DEVICE_NAME;
char start_msg = '<';
char end_msg = '>';

//WiFiClient espClient;
//PubSubClient client(espClient);
auto timer = timer_create_default();
Espalexa espalexa;
boolean connectWifi();
boolean wifiConnected = false;


template <typename... T>
void logToSerial(const char *str, T... args) {
  int i;
  int len = snprintf(NULL, 0, str, args...);
  if (len) {
      char buf[len];
      bzero(buf, len);
      sprintf(buf, str, args...);
      Serial.print('<');
      Serial.print(buf);
      Serial.print('>');
  }
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
      logToSerial("Start updating sketch");
    } else { // U_FS
      type = "filesystem";
      logToSerial("Start updating filesystem");
    }
  });
  ArduinoOTA.onEnd([]() {
    logToSerial("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    logToSerial("Progress: %u", total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    logToSerial("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      logToSerial("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      logToSerial("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      logToSerial("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      logToSerial("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      logToSerial("End Failed");
    }
  });
  ArduinoOTA.begin();
  logToSerial("OTA Ready");
}

boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);
  logToSerial("Connecting to WiFi");

  // Wait for connection
  logToSerial("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    logToSerial(".");
    if (i > 40) {
      state = false; 
      break;
    }
    i++;
  }
  if (state) {
    logToSerial("Connected to ");
    logToSerial(ssid);
    //logToSerial("IP address: ");
    //char ipaddr[16];
    //WiFi.localIP().toString().toCharArray(ipaddr, sizeof(ipaddr));
    //logToSerial(ipaddr);
    setupOTA();
  }
  else {
    logToSerial("Connection failed.");
  }
  return state;
}

bool helloworld(void *)
{
  logToSerial("Hello from ESP world.");
  return true; // keep timer active
}

void colorLightChanged1(uint8_t brightnessCommand, uint32_t rgb)
{
  logToSerial("OMG...its working!?. brightness - 0x%x rbd - 0x%x", brightnessCommand, rgb);
}

void setup() {
  Serial.begin(9600);
  logToSerial("Booting");

  wifiConnected = connectWifi();
  if (wifiConnected)
  {
    espalexa.addDevice(alexa_device_name, colorLightChanged1);
    espalexa.begin();
  }
  else
  {
    delay(5000);
    ESP.restart();
  }
  timer.every(5000, helloworld);
}

void loop() {
  espalexa.loop();
  ArduinoOTA.handle();
  timer.tick();
}
