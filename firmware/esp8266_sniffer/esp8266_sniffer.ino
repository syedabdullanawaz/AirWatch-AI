/*
  WiFi Security - ESP8266 RF Telemetry & Packet Sniffer
  -------------------------------------------------------
  Scans 2.4GHz WiFi channels (1-13) and transmits packet metadata
  (BSSID/MAC, RSSI signal strength, and channel number) over UART serial
  to the host Python Machine Learning Anomaly Detector.

  Baud Rate: 115200
*/

#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  
  // Set Wi-Fi to Station mode then disconnect to allow raw passive scanning
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("STARTING_SNIFFER");
}

void loop() {
  // Scan all 2.4GHz Wi-Fi channels (1-13)
  for (int ch = 1; ch <= 13; ch++) {
    // Scan networks on specific channel asynchronously=false, show_hidden=true
    int n = WiFi.scanNetworks(false, true, ch); 
    
    for (int i = 0; i < n; ++i) {
      // Send telemetry packet over serial in CSV format: DATA,MAC,RSSI,CHANNEL
      Serial.print("DATA,");
      Serial.print(WiFi.BSSIDstr(i)); // BSSID / MAC Address
      Serial.print(",");
      Serial.print(WiFi.RSSI(i));     // Signal Strength (dBm)
      Serial.print(",");
      Serial.println(ch);              // Channel Number
    }
    
    // Brief delay between channel hops to avoid RF frontend saturation
    delay(100); 
  }
}
