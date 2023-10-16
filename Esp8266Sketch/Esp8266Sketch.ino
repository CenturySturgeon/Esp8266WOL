#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>  // For mDNS support
#include <FS.h>
#include <TOTP.h> // For Time-based One Time Passwords

// Import the html header file
#include "index_html.h"

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Define NTP Server and Time Zone (remember, most authenticator apps will use the default values, UTC0, to avoid timezone issues)
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0; // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

// Set static IP address
IPAddress staticIP(192, 168, 7, 77);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Set the webserver on port XX
ESP8266WebServer server(80);

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

uint8_t hmacKey[] = {  };

TOTP totp = TOTP(hmacKey, 10);

String totpCode = String("");

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Configures static IP address
  if (!WiFi.config(staticIP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

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

  // Define the root path to serve the HTML file
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  // Handle the form submission
  server.on("/submit", HTTP_POST, []() {
    String inputText = server.arg("inputText");
    Serial.print("Submitted Text: ");
    Serial.println(inputText);
    String newCode = String(totp.getCode(timeClient.getEpochTime()));
    server.send(200, "text/html", "You submitted: " + inputText + " The real code: " + newCode);
  });

  server.begin();
}

void loop() {
  timeClient.update();

  // Serial.print("Current time: ");
  // Serial.println(timeClient.getFormattedTime());
  
  // generate the TOTP code and, if different from the previous one, print to screen
  // String newCode = String(totp.getCode(timeClient.getEpochTime()));
  // if(totpCode!= newCode) {
  //   totpCode = String(newCode);
  //   Serial.print("TOTP code: ");
  //   Serial.println(newCode);
  // }
  server.handleClient();
}
