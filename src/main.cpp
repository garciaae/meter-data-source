#include <Arduino.h>
#include <stdio.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <EmonLib.h>

#include <credentials.h>

const char* ssid = WIFI_SSID;
const char* pass = WIFI_PASS;
char payload[64];

EnergyMonitor energyMonitor;
float lineVoltage = 220.0;

WiFiClient client;

// Timeout de conexión WiFi: 30 segundos
const unsigned long WIFI_TIMEOUT_MS = 30000;
// Intervalo entre lecturas: 5 segundos
const unsigned long READ_INTERVAL_MS = 5000;
// Máximo de fallos HTTP consecutivos antes de reiniciar
const int MAX_HTTP_FAILURES = 60;

int httpFailures = 0;

void connectWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, pass);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed, restarting...");
    ESP.restart();
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println();

  energyMonitor.current(0, 55.7);

  connectWiFi();
}

void loop() {
  // Reconexión WiFi si se pierde
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    connectWiFi();
  }

  double Irms = energyMonitor.calcIrms(1484);
  double power = Irms * lineVoltage;

  Serial.print("Potencia= ");
  Serial.print(power);
  Serial.print("W        Irms = ");
  Serial.println(Irms);

  sprintf(payload, "{\"watts\":%f, \"station_id\":1}", power);

  HTTPClient http;
  if (http.begin(client, "http://192.168.1.175")) {
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.POST(payload);
    if (responseCode > 0) {
      String response = http.getString();
      Serial.println(response);
      httpFailures = 0;
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(responseCode);
      httpFailures++;
    }
    http.end();
  } else {
    httpFailures++;
  }

  // Si llevamos muchos fallos seguidos, reiniciamos
  if (httpFailures >= MAX_HTTP_FAILURES) {
    Serial.println("Too many HTTP failures, restarting...");
    ESP.restart();
  }

  delay(READ_INTERVAL_MS);
}
