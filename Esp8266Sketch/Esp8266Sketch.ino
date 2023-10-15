#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Define NTP Server and Time Zone
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0; // Change this to your time zone offset in seconds

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the time client
  timeClient.begin();
  timeClient.update();
}

void loop() {
  timeClient.update();
  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
  delay(10000); // Print the time every 10 seconds (adjust as needed)
}
