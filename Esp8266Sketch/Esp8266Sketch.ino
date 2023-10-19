#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <TOTP.h> // For Time-based One Time Passwords
#include <unordered_map>

// Import the html header file
#include "index_html.h"
// Import the environment variables (ssid, password, static IP, default local gateway (get it from your router) & hmacKey)
#include "envVariables.h"

// Define NTP Server and Time Zone (remember, most authenticator apps will use the default values, UTC0, to avoid timezone issues)
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0;       // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1); // Cloudflare DNS (can be another like google's or a local one of your choice)

// Set the webserver on port 80
BearSSL::ESP8266WebServerSecure server(443);
BearSSL::ServerSessions serverCache(5);

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

// Set the TOTP key to be used for code generation
TOTP totp = TOTP(hmacKey, 10);

String totpCode = String("");

// getPublicIp attempts 3 times to get the router's public ip, waiting 5 seconds for each reattempt
String getPublicIp() {
  String publicIp;
  for (int attempt = 1; attempt <= 3; attempt++) {
    WiFiClient client;
    if (client.connect("api.ipify.org", 80)) {
      Serial.println("Connected to api.ipify.org");
      client.println("GET / HTTP/1.1");
      client.println("Host: api.ipify.org");
      client.println("Connection: close");
      client.println();

      // Set a timeout for connecting
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println("Client timeout");
          client.stop();
          break; // Retry on timeout
        }
      }

      while (client.available()) {
        publicIp = client.readStringUntil('\n');
        Serial.println(publicIp);
      }

      // Close the connection
      client.stop();

      if (publicIp.length() > 0) {
        return publicIp; // Successfully obtained the IP
      }

      Serial.println("IP not received on attempt " + String(attempt));
    } else {
      Serial.println("Connection to ipify.org failed on attempt " + String(attempt));
    }

    if (attempt < 3) {
      Serial.println("Waiting 5 seconds before reattempt...");
      delay(5000); // Wait 5 seconds before reattempt
    }
  }

  return String(); // Return an empty string after 3 failed attempts
}

// Set session username and password
const char* http_username = "admin";
const char* http_password = "admin";

// std::unordered_map<std::string, std::string> credentials = {
//         {"admin", "admin"},
//         {"user", "user"},
//     };

struct UserSession
{
  IPAddress ip;
  bool isLoggedIn;
  ;
};

UserSession userSessions[2];

bool is_authenticated(IPAddress ip)
{
  Serial.println("Inside is authenticated");
  for (int i = 0; i < 2; i++)
  {
    Serial.println(ip.toString());
    Serial.println(userSessions[i].ip.toString());
    if (userSessions[i].ip == ip)
    {
      return true;
    }
  }
  return false;
}

void handleAuthentication(IPAddress ip)
{
  server.send(200, "text/plain", "not authenticated!");
}

void handleRoot()
{
  IPAddress clientIP = server.client().remoteIP(); // Get the client's IP address
  Serial.println(clientIP);
  is_authenticated(clientIP);

  if (!is_authenticated(clientIP))
  {
    // handleAuthentication(clientIP);
    server.send(200, "text/plain", "not authenticated!");
  }

  server.send(200, "text/plain", "Welcome to the protected page!");
}

void setup()
{
  UserSession test;
  IPAddress myip(192, 168, 1, 64);
  test.ip = myip;
  test.isLoggedIn = true;

  userSessions[0] = test;
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

  if (MDNS.begin("esp8266")){
    Serial.println("MDNS responder started");
  }

  // Add the openssl cert and private key
  server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  // Cache SSL sessions to accelerate the TLS handshake.
  server.getServer().setCache(&serverCache);

  // // Define the root path to serve the HTML file
  // server.on("/", HTTP_GET, []() {
  //   server.send(200, "text/html", index_html);
  // });

  server.on("/", HTTP_GET, handleRoot);

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
}

void loop() {
  // timeClient.update();

  // Serial.print("Current time: ");
  // Serial.println(timeClient.getFormattedTime());

  server.handleClient();
  MDNS.update();
}
