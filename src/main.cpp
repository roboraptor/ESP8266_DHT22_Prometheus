#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <WiFiManager.h> // WiFi Manager

#define DHTPIN 2     // GPIO2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);


void setup() {
  Serial.begin(115200);
  delay(10);

  // Start WiFiManager
  WiFiManager wifiManager;
  // Reset saved settings for testing purposes
  // wifiManager.resetSettings();

  // Automatically connect using saved credentials,
  // or start configuration portal if these fail.
  if (!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi
  Serial.println("Connected to WiFi");

  dht.begin();

  server.on("/metrics", []() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      server.send(500, "text/plain", "Failed to read from DHT sensor!");
      return;
    }

    String response = "# HELP temperature_celsius Temperature in Celsius\n";
    response += "# TYPE temperature_celsius gauge\n";
    response += "temperature_celsius " + String(t) + "\n";
    response += "# HELP humidity_percent Humidity in percent\n";
    response += "# TYPE humidity_percent gauge\n";
    response += "humidity_percent " + String(h) + "\n";

    server.send(200, "text/plain", response);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
