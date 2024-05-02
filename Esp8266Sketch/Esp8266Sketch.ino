// Use Arduino library's ESP8266WiFi, more at https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ESP8266WiFi.h>
// Use the NTPClient library by Fabrice Weinberg
#include <NTPClient.h>
// Use Arduino Wifi library, more at https://www.arduino.cc/reference/en/libraries/wifi/
#include <WiFiUdp.h>
// Use Arduino library's ESP8266WebServer and ESP8266WebServerSecure
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
// Use Arduino library's ESP8266mDNS, more at https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266mDNS
#include <ESP8266mDNS.h>

// For Time-based One Time Passwords use the TOTP library by Luca Dentella
#include <TOTP.h>
// Use the Crypto library by Rhys Weatherly to use SHA256 encryption
#include <Crypto.h>
#include <SHA256.h>
// For WOL functions use the WakeOnLan library by a7md0
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

  connectAndSendIp("On start public IP: https://");

  secureServer.WOL.setRepeat(3, 100);                                             // Repeat the packet three times with 100ms delay between
  secureServer.WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());  // Calculate and set broadcast address

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Certificate(s) expiration check
  checkCertificatesExpiration();

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

void checkCertificatesExpiration() {
  unsigned long currentPosixTime = time(nullptr);
  unsigned long expirationTime = currentPosixTime + certExpWarnInterval;
  // Serial.println("Expiration time");
  // Serial.println(expirationTime);
  // Serial.println("Current time");
  // Serial.println(currentPosixTime);
  if (expirationTime > ipSiteCertExp && ! ipSiteWarnSent) {
    Serial.println("Sending IP Site certificate expiration warning message");
    sendTelegramMessage("WARNING: IP Site certificate expires in 1 week or less.");
    ipSiteWarnSent = true;
  }
  if (expirationTime > serverCertExp && ! serverWarnSent) {
    Serial.println("Sending Esp8266 certificate expiration warning message");
    sendTelegramMessage("WARNING: Esp8266 certificate expires in 1 week or less.");
    serverWarnSent = true;
  }
  if (expirationTime > telegramCertExp && ! telegramWarnSent) {
    Serial.println("Sending Telegram API certificate expiration warning message");
    sendTelegramMessage("WARNING: Telegram API certificate expires in 1 week or less.");
    telegramWarnSent = true;
  }
}

void dailyChecks () {
  // Group of functions that run once a day
  unsigned long currentTime = millis();

  // Check if checkPublicIpInterval amount of time has passed since the last check (86,400,000 miliseconds in a day)
  if (currentTime - dailyCheckTime >= 86400000) {
    // Update the last check time
    dailyCheckTime = currentTime;
    // Certificate(s) expiration check
    checkCertificatesExpiration();
    // Reset the amount of manual checks available for changes in the public IP.
    secureServer.resetIpRetries();
  }
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
  // Perform daily checks
  dailyChecks();

  serverHTTP.handleClient();
  secureServer.server.handleClient();
  MDNS.update();

  checkSessionTimeouts();
  
}