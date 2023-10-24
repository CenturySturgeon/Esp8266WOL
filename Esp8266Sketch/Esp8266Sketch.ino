#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <TOTP.h> // For Time-based One Time Passwords

// Importing Rhys Weatherly's cryptography library to use SHA256 encryption
#include <Crypto.h> 
#include <SHA256.h>
// Importing a7md0's WakeOnLan library
#include <WakeOnLan.h>

// Import the SecureServer 
#include "types.h"
// Import the SetRoutes function alongside the calculateSHA256 function
#include "routes.h"

// Import the environment variables (ssid, password, static IP, default local gateway (get it from your router) & hmacKey)
#include "envVariables.h"

// Define NTP Server and Time Zone (remember, most authenticator apps will use the default values, UTC0, to avoid timezone issues)
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0;       // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1); // Cloudflare DNS (can be another like google's or a local one of your choice)

// Set the main webserver on port 443 (HTTPS)
BearSSL::ServerSessions serverCache(5);
// Set a secondary web server for HTTP redirection to HTTPS
ESP8266WebServer serverHTTP(80);

WiFiUDP udp;
// WakeOnLan WOL(udp); // Pass WiFiUDP class

// Set the TOTP key to be used for code generation
// TOTP totp = TOTP(hmacKey, 10);

// Initial value for the TOTP code
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

// Maximum lifetime for the sessions in seconds
unsigned long maxSessionLifeTime = 60;

// User session array for the handling of session states (default should always have no session)
UserSession userSessions[2] = {
    // 127.0.0.1 corresponds to the loopback address (localhost) and is not routable on the public internet
    // Hash is admin:admin
    { "The Admin", "8da193366e1554c08b2870c50f737b9587c3372b656151c4a96028af26f51334", IPAddress(127, 0, 0, 1), false, 0, maxSessionLifeTime},
    // Hash is user:user
    { "The User", "dc05eb46a46f4645f14bff72c8dfe95e0ba1b1d3d72e189ac2c977a44b7dcaf8", IPAddress(127, 0, 0, 1), false, 0, maxSessionLifeTime}
};

// Create a new SecureServer instance
SecureServer secureServer(443, userSessions, hmacKey, udp);

// Redirects incoming HTTP trafic to the HTTPS server
void secureRedirect() {
  serverHTTP.sendHeader("Location", String("https://esp8266.local"), true);
  serverHTTP.send(301, "text/plain", "");
}

void setup()
{
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

  secureServer.WOL.setRepeat(3, 100); // Repeat the packet three times with 100ms delay between
  secureServer.WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Calculate and set broadcast address

  if (MDNS.begin("esp8266")){
    Serial.println("MDNS responder started");
  }

  // Add the openssl cert and private key to the SecureServer's server
  secureServer.server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  // Cache SSL sessions to accelerate the TLS handshake.
  secureServer.server.getServer().setCache(&serverCache);

  setServerRoutes(secureServer);
  
  // Redirect all users using HTTP to the HTTPS server
  serverHTTP.on("/", HTTP_GET, secureRedirect);

  // Synchronizes the time to an NTP server, after that, you can access the epoch time (# of seconds since Jan 1 1970) with time(nullptr)
  Serial.print("Synching time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  while (time(nullptr) < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("Time synched with NTP server on UTC 0");

  secureServer.server.begin();
  serverHTTP.begin();

  // String publicIP = getPublicIp();
}

void checkSessionTimeouts () {
  unsigned long currentTime = millis();

  for (int i = 0; i < sizeof(secureServer.userSessions) / sizeof(secureServer.userSessions[0]); i++) {
    UserSession &session = userSessions[i];

    if (session.isLoggedIn && currentTime - session.sessionStart > session.lifeTime * 1000) {
      // Session has exceeded its lifetime: log out
      secureServer.logout(session.ip);
    }
  }
}

void loop() {
  serverHTTP.handleClient();
  secureServer.server.handleClient();
  MDNS.update();
  // Routinely check for session timeouts
  checkSessionTimeouts();
}
