// wifi_utils.h
#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
// Import the environment variables
#include "../envVariables.h"
// Import the telegram utils
#include "telegram.h"

// Cloudflare DNS (can be another like google's or a local one of your choice)
IPAddress dns(1, 1, 1, 1);
// Your time zone offset in seconds (recommendation is to leave it at 0 for TOTP)
const int timeZone = 0;
// X hour offset for Daylight Saving Time "DST" (recommendation is to leave it at 0 for TOTP)
const int daylightOffset = 0;
// NTP server to synch the time with
String ntpServer = "pool.ntp.org";
// Time interval in miliseconds to get the public IP, check if it changed and if so re-send it
unsigned long checkPublicIpInterval = 3600000;

// Create a list of certificates with the ipSiteRootCert certificate to get the public IP
X509List ipSiteCert(ipSiteRootCert);
// Initial value for the public IP (127.0.0.1 corresponds to the loopback address and is not routable on the public internet)
String publicIp = "127.0.0.1";
// Initial value for the lastCheckTime marker, this is used to check if the public IP has changed
unsigned long lastCheckTime = millis();

// Attempts 3 times to get the router's public IP, waiting 2 seconds for each reattempt
String getPublicIp() {
  String ip;
  for (int attempt = 1; attempt <= 3; attempt++) {

    WiFiClientSecure client;
    // Set the Esp8266 to trust ipify's certificate
    client.setTrustAnchors(&ipSiteCert);

    // Set the GET request on a single string
    if (client.connect(ipSiteAddress, 443)) {
      Serial.println("Connected to " + ipSiteAddress);
      client.print(String("GET / HTTP/1.0\r\n"
                          "Host:" + ipSiteAddress + "\r\n"
                          "Connection: close\r\n\r\n"));

      // Set a timeout for connecting
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 2000) {
          Serial.println("Client timeout");
          client.stop();
          break;  // Retry on timeout
        }
        yield();
      }

      while (client.available()) {
        ip = client.readStringUntil('\n');
        yield();
      }

      // Close the connection
      client.stop();

      if (ip.length() > 0) {
        return ip;  // Successfully obtained the IP
      }

      Serial.println("IP not received on attempt " + String(attempt));
    } else {
      Serial.println("Connection to " + ipSiteAddress + " failed on attempt " + String(attempt));
    }

    if (attempt < 3) {
      Serial.println("Waiting 2 seconds before reattempt...");
      yield();
      delay(2000);
    }
  }

  return String();  // Return an empty string after 3 failed attempts
}

// Returns wether the provided string is a valid IP or not
bool isValidIP(const String& ip) {
  IPAddress addr;
  return addr.fromString(ip);
}

// Synchronises the Esp's time with an NTP server of your choice
void synchTime() {
  // Synchronizes the time to an NTP server, after that, you can access the POSIX time (commonly known as epoch time), which is the number of seconds since January 1, 1970, 00:00:00 (UTC), using time(nullptr)
  Serial.println("Synching time: ");
  configTime(timeZone, daylightOffset, ntpServer);  // Get and set the time to UTC0 via an NTP server
  while (time(nullptr) < 24 * 3600) {
    Serial.println(".");
    delay(100);
    yield();
  }
  Serial.println("Time synched with NTP server on UTC 0");
}

// Recursive function that doesn't stop until it gets a valid IPV4 address
String persintentGetPublicIp () {
  String ip = getPublicIp();
  if (isValidIP(ip)) {
    return ip;
  } else {
    yield();
    return persintentGetPublicIp();
  }
}

// Gets the public IP and sends it over telegram
void sendPublicIp(String messagePrefix) {
  String ip = persintentGetPublicIp();
  // Set the public IP now to avoid resending it in the first checkPublicIpChange check
  publicIp = ip;
  sendTelegramMessage(messagePrefix + ip);
}

// Connects to the WiFi network and synchs the time with the NTP server
void connectToWiFi() {
  // Set the WiFi mode to station (the Soc connects as a client to the WiFi, instead of becoming an access point)
  WiFi.mode(WIFI_STA);
  // Configures static IP address
  WiFi.config(staticIP, gateway, subnet, dns);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  // Retry connection until success
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    yield();
  }
  synchTime();
}

// Connects to the WiFi network, synchs the time, and sends the public IP with a message prefix
void connectAndSendIp(String messagePrefix) {
  connectToWiFi();
  sendPublicIp(messagePrefix);
}

// Constantly checks the WiFi connection. If disconnected, it attempts to reconnect and on success re-sends the public IP
void checkAndReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectAndSendIp("WiFi connection lost. Public IP: ");
    delay(3000);
  }
}

// Checks if the public IP changed for the specified interval of time, re-sending if it changed
void checkPublicIpChange () {
  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Check if checkPublicIpInterval amount of time has passed since the last check
  if (currentTime - lastCheckTime >= checkPublicIpInterval) {
    // Update the last check time
    lastCheckTime = currentTime;

    // Check if the public IP has changed
    String currentPublicIp = persintentGetPublicIp();
    if (currentPublicIp != publicIp) {
      // Public IP has changed, send the new one
      sendTelegramMessage("Your public IP changed to: " + currentPublicIp);
      publicIp = currentPublicIp;  // Update the previous value
    }
  }
}

#endif