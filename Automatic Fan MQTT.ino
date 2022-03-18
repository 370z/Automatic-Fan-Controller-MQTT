#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#define FIREBASE_HOST "firebasehost"  //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_AUTH "token" //Change to your Firebase RTDB secret password
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
//MQTT
const char* mqtt_server = "server";
const char *mqtt_user = "user";
const char *mqtt_pass = "password";

#include "DHT.h"
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float t;
//Define Firebase Data objects
FirebaseData fanMode;
FirebaseData fanSpeed;


bool modeState = true;
String path = "/Fancontrol";

int MotorL = D1;
int MotorR = D2;
int MotorPWM = D3;

// multitask
unsigned long t0 = 0;
unsigned long t1 = 0;
unsigned long t2 = 0;

// Topic
#define M_mode  "/setMode"
#define M_speed  "/setSpeed"

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{

  Serial.begin(115200);
  delay(20);
  dht.begin();
  pinMode(MotorL, OUTPUT);
  pinMode(MotorR, OUTPUT);
  pinMode(MotorPWM, OUTPUT);

  Serial.println();
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("connected...yeey :)");
  client.setServer(mqtt_server, 16232);
  client.setCallback(callback);
  reconnect();
  delay(3000);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  if (!Firebase.beginStream(fanMode, path + "/Mode"))
  {
    Serial.println("Could not begin stream");
    Serial.println("REASON: " + fanMode.errorReason());
    Serial.println();
  }
  if (!Firebase.beginStream(fanSpeed, path + "/Speed"))
  {
    Serial.println("Could not begin stream");
    Serial.println("REASON: " + fanSpeed.errorReason());
    Serial.println();
  }
}

void loop()
{
  t = dht.readTemperature();
  unsigned long task = millis();
  int fanVal;
  if (task - t0 >= 1000) {
    t0 = task;
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    digitalWrite(MotorL, 1);
    digitalWrite(MotorR, 0);
    if (modeState == true) {
      if (t < 27) {

        Serial.print("Temp: ");
        Serial.println(t);
        client.publish("temp", String(t).c_str());
        client.publish("speed", String(70).c_str());
        Firebase.setInt(fanSpeed, path + "/Speed", 70);
        analogWrite(MotorPWM, 70);
      }
      if (t > 27 && t < 32) {
        Serial.print("Temp: ");
        Serial.println(t);
        client.publish("temp", String(t).c_str());
        client.publish("speed", String(125).c_str());
        Firebase.setInt(fanSpeed, path + "/Speed", 125);
        analogWrite(MotorPWM, 125);
      }
      if (t > 32) {
        Serial.print("Temp: ");
        Serial.println(t);
        client.publish("temp", String(t).c_str());
        client.publish("speed", String(255).c_str());
        Firebase.setInt(fanSpeed, path + "/Speed", 255);
        analogWrite(MotorPWM, 255);
      }
    } else {
      if (Firebase.getInt(fanSpeed, path + "/Speed")) {
        if (fanSpeed.dataType() == "int") {
          client.publish("temp", String(t).c_str());
          client.publish("speed", String(fanSpeed.intData()).c_str());
          Serial.println(fanSpeed.intData());
          analogWrite(MotorPWM, fanSpeed.intData());
        }
      }
    }
  }


  if (task - t1 >= 50) {
    t1 = task;
    if (!Firebase.readStream(fanMode))
    {
      Serial.println();
      Serial.println("Can't read stream data");
      Serial.println("REASON: " + fanMode.errorReason());
      Serial.println();
    }

    if (fanMode.streamTimeout())
    {
      Serial.println();
      Serial.println("Stream timeout, resume streaming...");
      Serial.println();
    }

    if (fanMode.boolData() != modeState) {
      bool _modeState = modeState;
      modeState = fanMode.boolData();
      if (Firebase.setBool(fanMode, path + "/Mode", modeState)) {
        if (modeState)
          Serial.println("Set Mode to Auto");
        else
          Serial.println("Set Mode to Manual");
      }
      else {
        modeState = _modeState;
        Serial.println("Could not set Mode");
      }
    }
  }


}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = topic;
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if (topicStr == "/setMode") {
    Firebase.setBool(fanMode, path + "/Mode", (message == "1" ? 1 : 0));
    modeState = (message == "1" ? true : false);
  }
  if (topicStr == "/setSpeed") {
    Firebase.setInt(fanSpeed, path + "/Speed", message.toInt());
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
      client.subscribe(M_mode);
      client.subscribe(M_speed);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
