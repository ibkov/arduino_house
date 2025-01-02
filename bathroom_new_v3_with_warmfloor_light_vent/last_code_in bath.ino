#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// WiFi settings
const char* ssid = "nov8-52";
const char* password = "dfsDSvrdsw52";

// MQTT settings
const char* mqtt_server = "192.168.14.7";

// GPIO pins
#define DHTPIN 21
#define DHTTYPE DHT11
#define LED_INDICATOR 2
#define VENTILATION_PIN 5
#define WARM_FLOOR_PIN 18
#define LIGHT_PIN 19
#define SWITCH_LIGHT 35
#define SWITCH_VENT 34

// MQTT topics
#define TEMP_TOPIC "bathroom/temperature"
#define HUM_TOPIC "bathroom/humidity"
#define LIGHT_TOPIC "bathroom/light"
#define LIGHT_STATUS_TOPIC "bathroom/light/status"
#define VENTILATION_TOPIC "bathroom/ventilation"
#define VENTILATION_STATUS_TOPIC "bathroom/ventilation/status"
#define WARM_FLOOR_TOPIC "bathroom/warm_floor"
#define WARM_FLOOR_STATUS_TOPIC "bathroom/warm_floor/status"

// Global variables
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
bool lightState = false;
bool ventState = false;
bool floorState = false;
unsigned long lastDHTUpdate = 0;

// Function prototypes
void blinkIndicator(int times = 1);
void connectToWiFi();
void connectToMQTT();
void handleMQTTMessage(char* topic, byte* payload, unsigned int length);
void handleManualSwitches();
void publishDHTData();

void blinkIndicator(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_INDICATOR, HIGH);
    delay(100);
    digitalWrite(LED_INDICATOR, LOW);
    delay(100);
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    blinkIndicator();
  }
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(LIGHT_TOPIC);
      client.subscribe(VENTILATION_TOPIC);
      client.subscribe(WARM_FLOOR_TOPIC);
      blinkIndicator(3);
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void handleMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == LIGHT_TOPIC) {
    lightState = (message == "on");
    digitalWrite(LIGHT_PIN, lightState ? LOW : HIGH);
    client.publish(LIGHT_STATUS_TOPIC, lightState ? "on" : "off");
  } else if (String(topic) == VENTILATION_TOPIC) {
    ventState = (message == "on");
    digitalWrite(VENTILATION_PIN, ventState ? HIGH : LOW);
    client.publish(VENTILATION_STATUS_TOPIC, ventState ? "on" : "off");
  } else if (String(topic) == WARM_FLOOR_TOPIC) {
    floorState = (message == "on");
    digitalWrite(WARM_FLOOR_PIN, floorState ? HIGH : LOW);
    client.publish(WARM_FLOOR_STATUS_TOPIC, floorState ? "on" : "off");
  }
}

void handleManualSwitches() {
  bool currentLightState = digitalRead(SWITCH_LIGHT) == HIGH;
  if (currentLightState != lightState) {
    lightState = currentLightState;
    digitalWrite(LIGHT_PIN, lightState ? LOW : HIGH);
    client.publish(LIGHT_STATUS_TOPIC, lightState ? "on" : "off");
  }

  bool currentVentState = digitalRead(SWITCH_VENT) == HIGH;
  if (currentVentState != ventState) {
    ventState = currentVentState;
    digitalWrite(VENTILATION_PIN, ventState ? HIGH : LOW);
    client.publish(VENTILATION_STATUS_TOPIC, ventState ? "on" : "off");
  }
}

void publishDHTData() {
  unsigned long now = millis();
  if (now - lastDHTUpdate > 10000) {
    lastDHTUpdate = now;
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
      char tempStr[8];
      char humStr[8];
      dtostrf(temperature, 6, 2, tempStr);
      dtostrf(humidity, 6, 2, humStr);
      client.publish(TEMP_TOPIC, tempStr);
      client.publish(HUM_TOPIC, humStr);
    } else {
      client.publish(TEMP_TOPIC, "nan");
      client.publish(HUM_TOPIC, "nan");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_INDICATOR, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(VENTILATION_PIN, OUTPUT);
  pinMode(WARM_FLOOR_PIN, OUTPUT);
  pinMode(SWITCH_LIGHT, INPUT_PULLUP);
  pinMode(SWITCH_VENT, INPUT_PULLUP);

  digitalWrite(LIGHT_PIN, HIGH);
  digitalWrite(VENTILATION_PIN, LOW);
  digitalWrite(WARM_FLOOR_PIN, LOW);

  dht.begin();
  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(handleMQTTMessage);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (!client.connected()) {
    connectToMQTT();
  }

  client.loop();
  handleManualSwitches();
  publishDHTData();
}
