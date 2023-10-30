#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
// For Time-based One Time Passwords
#include <TOTP.h>

// Importing Rhys Weatherly's cryptography library to use SHA256 encryption
#include <Crypto.h>
#include <SHA256.h>
// Importing a7md0's WakeOnLan library
#include <WakeOnLan.h>

// Import the SecureServer
#include "./utils/types.h"
// Import the SetRoutes function alongside the calculateSHA256 function
#include "./utils/routes.h"
// Import the telegram utils
#include "./utils/telegram.h"
// Import the wifi utils
#include "./utils/wifi_utils.h"

// Import the environment variables (ssid, password, static IP, default local gateway (get it from your router) & hmacKey)
#include "envVariables.h"

// Set the main webserver on port 443 (HTTPS)
BearSSL::ServerSessions serverCache(5);
// Set a secondary web server for HTTP redirection to HTTPS
ESP8266WebServer serverHTTP(80);

WiFiUDP udp;

// Initial value for the TOTP code
String totpCode = String("");

// Maximum lifetime for the sessions in seconds
unsigned long maxSessionLifeTime = 60;

// User session array for the handling of session states (default should always have no session)
UserSession userSessions[2] = {
  // 127.0.0.1 corresponds to the loopback address (localhost) and is not routable on the public internet
  // Hash is admin:admin
  { "The Admin", "8da193366e1554c08b2870c50f737b9587c3372b656151c4a96028af26f51334", IPAddress(127, 0, 0, 1), false, 0, maxSessionLifeTime },
  // Hash is user:user
  { "The User", "dc05eb46a46f4645f14bff72c8dfe95e0ba1b1d3d72e189ac2c977a44b7dcaf8", IPAddress(127, 0, 0, 1), false, 0, maxSessionLifeTime }
};

// Create a new SecureServer instance
SecureServer secureServer(443, userSessions, hmacKey, udp);

// Redirects incoming HTTP trafic to the HTTPS server
void secureRedirect() {
  serverHTTP.sendHeader("Location", String("https://esp8266.local"), true);
  serverHTTP.send(301, "text/plain", "");
}

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Connect to WiFi and send the public ip over telegram
  connectAndSendIp("ESP8266 on Start Public IP: ");

  secureServer.WOL.setRepeat(3, 100);                                             // Repeat the packet three times with 100ms delay between
  secureServer.WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());  // Calculate and set broadcast address

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Add the openssl cert and private key to the SecureServer's server
  secureServer.server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  // Cache SSL sessions to accelerate the TLS handshake.
  secureServer.server.getServer().setCache(&serverCache);

  setServerRoutes(secureServer);

  // Redirect all users using HTTP to the HTTPS server
  serverHTTP.on("/", HTTP_GET, secureRedirect);

  secureServer.server.begin();
  serverHTTP.begin();
}

void checkSessionTimeouts() {
  unsigned long currentTime = millis();

  for (int i = 0; i < sizeof(secureServer.userSessions) / sizeof(secureServer.userSessions[0]); i++) {
    UserSession& session = secureServer.userSessions[i];

    if (session.isLoggedIn && currentTime - session.sessionStart > session.lifeTime * 1000) {
      // Session has exceeded its lifetime: log out
      secureServer.logout(session.ip);
    }
  }
}

void loop() {

  // Check for disconnections to the WiFi and attempt to reconnect if necessary
  checkAndReconnect();
  // Check for changes in the public ip every hour
  checkPublicIpChange();

  // Handle the clients and update the MDNS
  serverHTTP.handleClient();
  secureServer.server.handleClient();
  MDNS.update();

  // Routinely check for session timeouts
  checkSessionTimeouts();
  
}