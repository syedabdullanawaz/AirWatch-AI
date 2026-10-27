/*
  AirWatch-AI: ESP8266 Direct ThingSpeak IoT Cloud Broadcaster
  -------------------------------------------------------------
  Scans surrounding 2.4GHz Wi-Fi networks, calculates RF congestion & average
  signal strength, and streams telemetry directly to ThingSpeak IoT Cloud:
    - Field 1: Detected Networks Count (Device Density)
    - Field 2: Average Signal Strength / RSSI (dBm)
    - Field 3: Most Congested Wi-Fi Channel
    - Field 4: RF Threat / Suspicious Density Flag

  Prerequisites:
    - ESP8266 Board Package in Arduino IDE
    - ThingSpeak library by MathWorks (or direct HTTP GET)
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// --- Wi-Fi & ThingSpeak Credentials ---
const char* ssid = "YOUR_WIFI_SSID";           // Replace with your Wi-Fi SSID
const char* password = "YOUR_WIFI_PASSWORD";   // Replace with your Wi-Fi Password
const char* thingSpeakApiKey = "YOUR_WRITE_API_KEY"; // Replace with your ThingSpeak Write Key
const char* thingSpeakServer = "api.thingspeak.com";

WiFiClient client;
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 20000; // Post every 20 seconds (ThingSpeak free limit is 15s)

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n[+] Initializing AirWatch-AI ThingSpeak Node...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("[*] Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[✓] Connected to Wi-Fi! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[!] Wi-Fi connection failed. Will continue scanning locally.");
  }
}

void sendToThingSpeak(int networkCount, int avgRssi, int topChannel, int threatFlag) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    return;
  }

  if (client.connect(thingSpeakServer, 80)) {
    String postStr = "/update?api_key=" + String(thingSpeakApiKey);
    postStr += "&field1=" + String(networkCount);
    postStr += "&field2=" + String(avgRssi);
    postStr += "&field3=" + String(topChannel);
    postStr += "&field4=" + String(threatFlag);

    client.print(String("GET ") + postStr + " HTTP/1.1\r\n" +
                 "Host: " + thingSpeakServer + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("[☁️ ThingSpeak] Telemetry packet sent successfully.");
  } else {
    Serial.println("[!] Connection to ThingSpeak failed.");
  }
  client.stop();
}

void loop() {
  if (millis() - lastConnectionTime > postingInterval) {
    Serial.println("\n[*] Scanning ambient 2.4 GHz RF spectrum...");
    int n = WiFi.scanNetworks(false, true);

    if (n > 0) {
      int totalRssi = 0;
      int channelCounts[14] = {0};

      for (int i = 0; i < n; ++i) {
        totalRssi += WiFi.RSSI(i);
        int ch = WiFi.channel(i);
        if (ch >= 1 && ch <= 13) {
          channelCounts[ch]++;
        }
      }

      int avgRssi = totalRssi / n;

      // Determine most congested channel
      int topChannel = 1;
      int maxOnChannel = channelCounts[1];
      for (int c = 2; c <= 13; c++) {
        if (channelCounts[c] > maxOnChannel) {
          maxOnChannel = channelCounts[c];
          topChannel = c;
        }
      }

      // Flag as potential threat if sudden density spike or abnormal signal strength
      int threatFlag = (n > 25 || avgRssi > -35) ? 1 : 0;

      Serial.printf("[*] Found %d networks | Avg RSSI: %d dBm | Congested Ch: %d | Threat: %s\n",
                    n, avgRssi, topChannel, threatFlag ? "HIGH" : "NORMAL");

      sendToThingSpeak(n, avgRssi, topChannel, threatFlag);
    }

    lastConnectionTime = millis();
  }
}
