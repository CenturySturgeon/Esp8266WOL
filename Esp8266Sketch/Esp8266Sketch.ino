#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <TOTP.h> // For Time-based One Time Passwords

// Import the html header file
#include "index_html.h"
// Import the environment variables (ssid, password, static IP, default local gateway (get it from your router) & hmacKey)
#include "envVariables.h"

// Define NTP Server and Time Zone (remember, most authenticator apps will use the default values, UTC0, to avoid timezone issues)
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0; // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1);            // Cloudflare DNS (can be another like google's or a local one of your choice)

// Set the webserver on port 80
ESP8266WebServer server(80);

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

TOTP totp = TOTP(hmacKey, 10);

String totpCode = String("");

String getPublicIp()
{
  WiFiClient client;
  if (client.connect("api.ipify.org", 80)) 
  {
      Serial.println("connected");
      client.println("GET / HTTP/1.0");
      client.println("Host: api.ipify.org");
      client.println();
  } else {
      Serial.println("Connection to ipify.org failed");
      return String();
  }
  delay(5000);
  String line;
  while(client.available())
  {
    line = client.readStringUntil('\n');
    // Serial.println(line);
  }
  return line;
}

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Set the WiFi mode to station (the Soc connects as a client to the WiFi, instead of becoming an access point)
  WiFi.mode(WIFI_STA);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  // Configures static IP address
  WiFi.config(staticIP, gateway, subnet, dns);
  // Retry connection until success
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  } 
  Serial.println("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  // Initialize the time client
  timeClient.begin();
  timeClient.update();

  // Define the root path to serve the HTML file
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  // Handle the form submission
  server.on("/submit", HTTP_POST, []() {
    timeClient.update();
    String inputText = server.arg("inputText");
    Serial.print("Submitted Text: ");
    Serial.println(inputText);
    String newCode = String(totp.getCode(timeClient.getEpochTime()));
    server.send(200, "text/html", "You submitted: " + inputText + " The real code: " + newCode + " " + timeClient.getFormattedTime());
  });

  server.begin();

  String publicIP = getPublicIp();
  Serial.println( "your ip" + publicIP);
}

void loop() {
  // timeClient.update();

  // Serial.print("Current time: ");
  // Serial.println(timeClient.getFormattedTime());

  server.handleClient();
}
