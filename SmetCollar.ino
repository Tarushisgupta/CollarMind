#include <WiFi.h>
#include <string.h>
#include "PubSubClient.h"
#include "DHTesp.h"
#include "ThingSpeak.h"
using namespace std;


#define BUTTON 14
#define LED 12
#define BUZZER 13
#define DHT_PIN 15

#define CHANNEL_ID 1814183
#define WRITE_API_KEY "NZIGQEYVNFES258N"
#define READ_API_KEY "OWOSMXM9U9Z9FVT2"

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "test.mosquitto.org";

const char * iftttServer = "maker.ifttt.com";
const char * iftttRequest = "/trigger/SmetCollar/with/key/jW-xnwHKhkbUasbLtww26-BiYI2v9QWDGCUUzinTsKt?value1=";

const char * thinkSpeakServer = "api.thingspeak.com";

const int PORT = 80;
const int mqttPort = 1883;

const char* RAIN_TOPIC = "SmetCollar/in/rain";
const char* TEMP_TOPIC = "SmetCollar/in/temperature";
const char* HUMIDITY_TOPIC = "SmetCollar/in/humidity";
const char* LATITUDE_TOPIC = "SmetCollar/in/longitude";
const char* LONGITUDE_TOPIC = "SmetCollar/in/latitude";

const char* BUZZER_TOPIC = "SmetCollar/out/buzzer";


WiFiClient espClient;
WiFiClient iftttclient;
WiFiClient TSclient;

PubSubClient mqttClient(espClient);
DHTesp dht;


//DATA
int RAIN = 0;
float TEMP = 0;
float HUMIDITY = 0;
float LATITUDE = 0;
float LONGITUDE = 0;


void wifiConnect()
{
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void mqttReconnect()
{
  while(!mqttClient.connected())
  {
    // Serial.println("Attempting MQTT connecting...");
    if (mqttClient.connect("12345678")){
      // Serial.println("Connected");
      mqttClient.subscribe(BUZZER_TOPIC);
    }
    else {
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

bool isbuzzerOn = false;
void callback(char* topic, byte* message, unsigned int length) {
  String stTopic = topic;
  String stMessage = (char*)message;

  Serial.println(stTopic);
  Serial.println(stMessage);
  
  if (stTopic == BUZZER_TOPIC)
    isbuzzerOn = !isbuzzerOn;
}

unsigned long startBuzz = millis();
void triggerBuzzer()
{ 
  if (millis() - startBuzz > 500)
  {
    startBuzz = millis();
    digitalWrite(LED, HIGH);
    tone(BUZZER, 1000, 100);
    tone(BUZZER, 800, 100);
  }
  digitalWrite(LED, LOW);
  // noTone(BUZZER);
}

void sendRequest(const char* host,const char* request, String value) {
  while(!iftttclient.connect(host, PORT)){
    Serial.println("connection fail");
    delay(1000);
  }

  iftttclient.print(String("GET ") + request + value + " HTTP/1.1\r\n" + 
            "Host: "+ host + "\r\n" + 
            "Connection: close\r\n\r\n");
}



unsigned int DelayRequired = 10000;
unsigned long PrevMillis = millis();
void uploadData()
{
  if(millis() - PrevMillis > DelayRequired)
    {
          PrevMillis = millis();
          printData();

           //Send to ThingSpeak Cloud
          ThingSpeak.setField(1, RAIN);
          ThingSpeak.setField(2, TEMP);
          ThingSpeak.setField(3, HUMIDITY);
          ThingSpeak.setField(4, LATITUDE);
          ThingSpeak.setField(5, LONGITUDE);

          int sc = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

          if (sc == 200)
            Serial.println("[Sent to Clould]\n");

          
          //Send Data through MQTT Server
          mqttClient.publish(RAIN_TOPIC, (to_string(RAIN)).c_str());
          mqttClient.publish(TEMP_TOPIC, (String(TEMP, 2)).c_str());
          mqttClient.publish(HUMIDITY_TOPIC, (String(HUMIDITY, 1)).c_str());
          mqttClient.publish(LATITUDE_TOPIC, String(LATITUDE).c_str());
          mqttClient.publish(LONGITUDE_TOPIC, String(LONGITUDE).c_str());

          Serial.println("[Sent Data]\n");
      }
}

void updateData()
{
    TEMP = dht.getTemperature();
    HUMIDITY = dht.getHumidity();
    RAIN = random(1024);
    LATITUDE = random(-9000, 9000)*0.01;
    LONGITUDE = random(-18000, 18000)*0.01;
}

void printData()
{
    Serial.println("Rain: " + String(RAIN));
    Serial.println("Temp: " + String(TEMP, 2) + "°C");
    Serial.println("Humidity: " + String(HUMIDITY, 2) + "%");
    Serial.println("Langtitude: " + String(LATITUDE));
    Serial.println("Longitude: " + String(LONGITUDE));
}

String envCheck()
{
  String msg = "";
  if (RAIN >= 1024/3 && RAIN <= 1024/2 && TEMP >= 20 && TEMP <= 33) //Sunny Day
    msg = "It's_a_sweat_sunny_day_to_play_with_your_pet_outside!";
  else if (RAIN >= 1024/2) //Rainy Day
    msg = "[!]Be_aware_of_the_rain!_Your_pet_should_be_in_a_shelter_now.";
  else if (TEMP <= 10) // Cold day
    msg = "[!]What_a_chilly_day,_your_pet_needs_a_cozy_corner!";
  else if (HUMIDITY >= 60) //The pet is wet
    msg = "[!]Your_pet_is_soaking_wet_right_now!";

  return msg;
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Connecting to Wifi");

  digitalWrite(LED, HIGH);
  wifiConnect();
 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  ThingSpeak.begin(TSclient);

  dht.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  delay(10000);
  digitalWrite(LED, LOW);
}

unsigned long startTimeUpdate = millis();
unsigned long DelayNotify = 3000;
unsigned long startNotify = millis() ;
void loop() {

  if (!mqttClient.connected()){
    mqttReconnect();
  }
  
  mqttClient.loop();

  if (millis() - startTimeUpdate > 3000)
  {
    startTimeUpdate = millis();

    //Update all data once
    updateData();

    if (millis() - startNotify > DelayNotify)
    {
      startNotify = millis();
      String msg = envCheck();
      if (msg != "")
      {
        sendRequest(iftttServer, iftttRequest, "Rain:[" + String(RAIN) + "]--Temp:[" + String(TEMP) + "C]--Humidity:[" + String(HUMIDITY) + "%]");
        delay(500);
        sendRequest(iftttServer, iftttRequest, msg);
        
        Serial.println("[Sent Notifications]\n");
        DelayNotify = 28800000;
      }
      else
        DelayNotify = 3000;
    }
  }

  if (isbuzzerOn)
    triggerBuzzer();



  uploadData();
}
