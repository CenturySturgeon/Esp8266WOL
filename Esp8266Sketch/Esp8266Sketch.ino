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

// Import utils
#include "./utils/types.h"
#include "./utils/routes.h"
#include "./utils/telegram.h"
#include "./utils/wifi_utils.h"

// Import the environment variables
#include "envVariables.h"

// Set the main webserver on port 443 (HTTPS)
BearSSL::ServerSessions serverCache(5);
// Set a secondary web server for HTTP redirection to HTTPS
ESP8266WebServer serverHTTP(80);

WiFiUDP udp;

// Initial value for the TOTP code
String totpCode = String("");

SecureServer secureServer(443, userSessions, udp, numUSessions);

// Redirects incoming HTTP trafic to the HTTPS server
void secureRedirect() {
  serverHTTP.sendHeader("Location", String("https://esp8266.local"), true);
  serverHTTP.send(301, "text/plain", "");
}

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  connectAndSendIp("On Start Public IP: ");

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

  serverHTTP.on("/", HTTP_GET, secureRedirect);

  // List of headers to be recorded
  const char * headerkeys[] = {"Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //Set the server to track these headers
  secureServer.server.collectHeaders(headerkeys, headerkeyssize);

  secureServer.server.begin();
  serverHTTP.begin();
}

void checkSessionTimeouts() {
  unsigned long currentTime = millis();

  for (int i = 0; i < numUSessions; i++) {
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

  serverHTTP.handleClient();
  secureServer.server.handleClient();
  MDNS.update();

  checkSessionTimeouts();
  
}