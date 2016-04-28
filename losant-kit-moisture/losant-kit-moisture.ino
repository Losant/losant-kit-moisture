/**
 * Firmware for the Losant IoT Moisture Sensor Kit.
 * Sends moisture level to Losant platform every hour.
 * Losant will send command to toggle LED based on level.
 *
 * Visit https://www.losant.io/kit for full instructions.
 *
 * Copyright (c) 2016 Losant IoT. All rights reserved.
 * https://www.losant.com
 */

#include <ESP8266WiFi.h>
#include <Losant.h>

// WiFi credentials.
const char* WIFI_SSID = "my-wifi-ssid";
const char* WIFI_PASS = "my-wifi-pass";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "my-device-id";
const char* LOSANT_ACCESS_KEY = "my-access-key";
const char* LOSANT_ACCESS_SECRET = "my-access-secret";

const int LED_PIN = 5; // D1
const int MOISTURE_PIN = 4; // D2

// Time between moisture sensor readings.
// The longer this delay, the longer the sensor
// will last without corroding.
const int REPORT_INTERVAL = 60000; // ms

// Delay between loops.
const int LOOP_INTERVAL = 10; // ms

WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

/**
 * Invoked whenever a command is sent from Losant
 * to this device.
 */
void handleCommand(LosantCommand *command) {
  Serial.print("Command received: ");
  Serial.println(command->name);

  if(strcmp(command->name, "turnLedOn") == 0) {
    Serial.println("Turning LED On");
    digitalWrite(LED_PIN, HIGH);
  }
  else if(strcmp(command->name, "turnLedOff") == 0) {
    Serial.println("Turning LED Off");
    digitalWrite(LED_PIN, LOW);
  }
}

/**
 * Connects to WiFi and then to the Losant platform.
 */
void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  unsigned long connectionStart = millis();
  while(!device.connected()) {
    delay(500);
    Serial.print(".");

    // If we can't connect after 5 seconds, restart the board.
    if(millis() - connectionStart > 5000) {
      Serial.println("Failed to connect to Losant, restarting board.");
      ESP.restart();
    }
  }

  Serial.println("Connected!");
  Serial.println("This device is now ready for use!");
}

void setup() {
  Serial.begin(115200);

  // Wait until the serial in fully initialized.
  while(!Serial) {}
  
  Serial.println();
  Serial.println("Running Moisture Sensor Firmware.");
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOISTURE_PIN, OUTPUT);

  // Attached the command handler.
  device.onCommand(&handleCommand);
  
  connect();
}

/**
 * Reads the value of the moisture sensor
 * and sends the value to Losant.
 */
void reportMoisture() {
  // Turn on the moisture sensor.
  // The sensor will corrode very quickly with current
  // running through it all the time. We just need to send
  // current through it long enough to read the value.
  digitalWrite(MOISTURE_PIN, HIGH);

  // Little time to stablize.
  delay(100);

  double raw = analogRead(A0);
  
  Serial.println();
  Serial.print("Moisture level: ");
  Serial.println(raw);

  // Turn the sensor back off.
  digitalWrite(MOISTURE_PIN, LOW);

  // Build the JSON payload and send to Losant.
  // { "moisture" : <raw> }
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["moisture"] = raw;
  device.sendState(root);
}

int timeSinceLastRead = 0;

void loop() {

  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if(!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if(toReconnect) {
    connect();
  }

  device.loop();

  // If enough time has elapsed, report the moisture.
  if(millis() - timeSinceLastRead > REPORT_INTERVAL) {
    timeSinceLastRead = millis();
    reportMoisture();
  }

  delay(LOOP_INTERVAL);
}
