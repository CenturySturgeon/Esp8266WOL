#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TOTP.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Define NTP Server and Time Zone
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0; // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

uint8_t hmacKey[] = {  };

TOTP totp = TOTP(hmacKey, 10);

String totpCode = String("");

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

  // Serial.print("Current time: ");
  // Serial.println(timeClient.getFormattedTime());
  
  // generate the TOTP code and, if different from the previous one, print to screen
  String newCode = String(totp.getCode(timeClient.getEpochTime()));
  if(totpCode!= newCode) {
    totpCode = String(newCode);
    Serial.print("TOTP code: ");
    Serial.println(newCode);
  }
}
