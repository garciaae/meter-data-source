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
float voltajeRed = 220.0;

WiFiClient client;


void setup() {
  Serial.begin(9600);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  energyMonitor.current(0, 55.7);

  WiFi.mode(WIFI_STA); // Modo cliente WiFi
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected"); 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // Mostramos la IP
}

void loop() {
  double Irms = energyMonitor.calcIrms(1484);
  
  // Calculamos la potencia aparente  
  double potencia =  Irms * voltajeRed;
  
  // Mostramos la información por el monitor serie
  Serial.print("Potencia= ");
  Serial.print(potencia);
  Serial.print("W        Irms = ");
  Serial.println(Irms);
  
  // Preparamos el payload para enviar
  sprintf(payload, "{\"watts\":%f, \"station_id\":1}", potencia);
  
  // Start sending data
  HTTPClient http;
  if (http.begin(client, "http://192.168.1.175")) {
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.POST(payload);
    if (responseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(responseCode);
    }
    http.writeToStream(&Serial);
    http.end();
  }
  delay(5000);
}
