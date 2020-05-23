
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

const char* ssid = "nov8-52";
const char* password = "dfsDSvrdsw52";
const char* mqtt_server = "192.168.14.7";
const char led = 13;

/* define DHT pins */
#define DHTPIN 27
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

/* system variables */
float temperature = 0;
float humidity = 0;
float VOLTAGE_INPUT_VENT;
int INPUT_SWITCHER_VENT = 34;
int INPUT_SWITCHER_LIGHT = 35;
boolean CHECK_WARM_FLOOR = false;
boolean CHECK_VENTILATION = false;
boolean CHECK_LIGHT = false;
boolean CHANGE_VAL = false;
boolean CHANGE_VAL_LIGHT = false;
long lastMsg = 0;
char msg[20];
#define VENTILATION 5
#define ledcard 2
#define warm_floor_switch 18
#define LIGHT 19



/* create an instance of PubSubClient client */
WiFiClient espClient;
PubSubClient client(espClient);

/* topics */
#define TEMP_TOPIC    "bathroom/temp"
#define HUM_TOPIC    "bathroom/hum"
#define LED_TOPIC     "smarthome/room1/led" /* 1=on, 0=off */
#define VENTILATION_TOPIC  "bathroom/ventilation" /* 1=on, 0=off */
#define VENTILATION_TOPIC_STATUS  "bathroom/ventilation/status" /* 1=on, 0=off */
#define WARM_FLOOR_TOPIC "bathroom/warm_floor"
#define WARM_FLOOR_STATUS_TOPIC "bathroom/warm_floor/status"
#define LIGHT_TOPIC "bathroom/light"
#define LIGHT_STATUS_TOPIC "bathroom/light/status"


void receivedCallback(char* topic, byte* payload, unsigned int length) {
  String payload_ = "";
  String topic_ = String(topic);
  Serial.print("Message received: ");
  Serial.println(topic);
  Serial.print("payload: ");
  for (int i = length - 1; i >= 0; i--) {
    payload_ = (char)payload[i] + payload_;
  }
  Serial.println(payload_);
  if (topic_ == VENTILATION_TOPIC and payload_ == "on") CHECK_VENTILATION = true;
  else if (topic_ == VENTILATION_TOPIC and payload_ == "off") CHECK_VENTILATION = false;

  if (topic_ == LIGHT_TOPIC and payload_ == "on") CHECK_LIGHT = true;
  else if (topic_ == LIGHT_TOPIC and payload_ == "off") CHECK_LIGHT = false;

  if (topic_ == WARM_FLOOR_TOPIC and payload_ == "on") CHECK_WARM_FLOOR = true;
  else if (topic_ == WARM_FLOOR_TOPIC and payload_ == "off") CHECK_WARM_FLOOR = false;
}


void ledcard_blick_connect() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(ledcard, HIGH);
    delay(100);
    digitalWrite(ledcard, LOW);
    delay(100);
  }
  digitalWrite(ledcard, HIGH);
}


void ledcard_blick_connect_Wifi() {
  digitalWrite(ledcard, HIGH);
  delay(100);
  digitalWrite(ledcard, LOW);
  delay(100);
}

void light_switch() {
  if (digitalRead(INPUT_SWITCHER_LIGHT) == HIGH and CHANGE_VAL_LIGHT == false) {
    CHECK_LIGHT = true;
    CHANGE_VAL_LIGHT = true;
  }
  if (digitalRead(INPUT_SWITCHER_LIGHT) == LOW and CHANGE_VAL_LIGHT == true)  {
    CHECK_LIGHT = false;
    CHANGE_VAL_LIGHT = false;
  }
  if (CHECK_LIGHT == true) {
    digitalWrite(LIGHT, LOW);
    client.publish(LIGHT_STATUS_TOPIC, "on");
  }
  else if (CHECK_LIGHT == false) {
    digitalWrite(LIGHT, HIGH);
    client.publish(LIGHT_STATUS_TOPIC, "off");
  }
}

void connect_Wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    light_switch();
    ledcard_blick_connect_Wifi();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    light_switch();
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "ESP32Client";
    /* connect now */
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      ledcard_blick_connect();
      /* subscribe topic with default QoS 0*/
      client.subscribe(LED_TOPIC);
      client.subscribe(VENTILATION_TOPIC);
      client.subscribe(WARM_FLOOR_TOPIC);
      client.subscribe(LIGHT_TOPIC);
    } else {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
//      digitalWrite(ledcard, HIGH);
//      delay(1000);
//      digitalWrite(ledcard, LOW);
//      delay(1000);
//      /* Wait 5 seconds before retrying */
//      delay(2000);
    }
  }
}

void warm_floor() {
  if (CHECK_WARM_FLOOR == true) {
    digitalWrite(warm_floor_switch, HIGH);
    client.publish(WARM_FLOOR_STATUS_TOPIC, "on");
  }
  else {
    digitalWrite(warm_floor_switch, LOW);
    client.publish(WARM_FLOOR_STATUS_TOPIC, "off");
  }
}

void ventilation_switch() {
  if (digitalRead(INPUT_SWITCHER_VENT) == HIGH and CHANGE_VAL == false) {
    CHECK_VENTILATION = true;
    CHANGE_VAL = true;
  }
  if (digitalRead(INPUT_SWITCHER_VENT) == LOW and CHANGE_VAL == true)  {
    CHECK_VENTILATION = false;
    CHANGE_VAL = false;
  }

  if (CHECK_VENTILATION == true) {
    digitalWrite(VENTILATION, HIGH);
    client.publish(VENTILATION_TOPIC_STATUS, "on");
  }
  else if (CHECK_VENTILATION == false) {
    digitalWrite(VENTILATION, LOW);
    client.publish(VENTILATION_TOPIC_STATUS, "off");
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(ledcard, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(INPUT_SWITCHER_VENT, INPUT_PULLUP);
  pinMode(VENTILATION, OUTPUT);
  pinMode(warm_floor_switch, OUTPUT);
  pinMode(INPUT_SWITCHER_LIGHT, INPUT_PULLUP);
  pinMode(LIGHT, OUTPUT);
  connect_Wifi();
  /* configure the MQTT server with IPaddress and port */
  client.setServer(mqtt_server, 1883);
  /* this receivedCallback function will be invoked
    when client received subscribed topic */
  client.setCallback(receivedCallback);
  /*start DHT sensor */
  dht.begin();
}



void loop() {
  if (WiFi.status() != WL_CONNECTED){
      light_switch();
      connect_Wifi();
    }
  /* if client was disconnected then try to reconnect again */
  if (!client.connected()) {
    mqttconnect();
  }
  
  /* this function will listen for incomming
    subscribed topic-process-invoke receivedCallback */
  client.loop();
  /* we measure temperature every 3 secs
    we count until 3 secs reached to avoid blocking program if using delay()*/
  long now = millis();
  ventilation_switch();
  light_switch();
  warm_floor();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    /* read DHT11/DHT22 sensor and convert to string */
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();


    if (!isnan(temperature)) {
      snprintf (msg, 20, "%lf", temperature);
      client.publish(TEMP_TOPIC, msg);
      snprintf (msg, 20, "%lf", humidity);
      client.publish(HUM_TOPIC, msg);
    }
    else {
      client.publish(TEMP_TOPIC, "NO");
    }
  }
}
