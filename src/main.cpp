#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <WiFiManager.h> // WiFi Manager
#include <Adafruit_Sensor.h>
#include <LittleFS.h>

#define DHTPIN 2     // GPIO2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

// Unikátní identifikátor desky
String deviceID = String(ESP.getChipId());

// Proměnná pro poznámku
String notes = "";

// Funkce pro načtení poznámek z LittleFS
void loadNotes() {
  if (LittleFS.exists("/notes.txt")) {
    File file = LittleFS.open("/notes.txt", "r");
    if (file) {
      notes = file.readString();
      file.close();
    }
  }
}

// Funkce pro uložení poznámek do LittleFS
void saveNotes() {
  File file = LittleFS.open("/notes.txt", "w");
  if (file) {
    file.print(notes);
    file.close();
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  loadNotes();

  // Start WiFiManager
  WiFiManager wifiManager;
  // Reset saved settings for testing purposes
  // wifiManager.resetSettings();

  WiFi.hostname("ESP8266-" + deviceID);
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
  server.on("/", []() {
    String html = "<html><head><title>" + deviceID + " " + notes + "</title></head><body><h1>Name and description</h1><p>ChipID: " + deviceID + "<br>Hostname: ESP8266-" + deviceID + "</p><form action='/add' method='POST'>"
                  "<textarea name='note' rows='3' cols='30'>" + notes + "</textarea><br>"
                  "<input type='submit' value='Save'></form><br><a href='/metrics'>Metrics</a><br><h2>Saved:</h2>"
                  + notes + "</body></html>";
    server.send(200, "text/html", html);
  });
  server.on("/add", HTTP_POST, []() {
    if (server.hasArg("note")) {
      notes = server.arg("note");
      saveNotes();  // Uložení poznámky do LittleFS
      server.sendHeader("Location", "/");
      server.send(303);  // Redirect to the main page
    } else {
      server.send(400, "text/plain", "Invalid Request");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

