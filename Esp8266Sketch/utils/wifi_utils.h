// wifi_utils.h
#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
// Import the environment variables
#include "../envVariables.h"
// Import the telegram utils
#include "telegram.h"

// Create a list of certificates with the ipify certificate to get the public ip
X509List ipifyCert(ipifyRootCert);
// Initial value for the public IP
String publicIp = "127.0.0.1";
// Initial value for the lastCheckTime marker, this is used to check if the public ip has changed
unsigned long lastCheckTime = millis();
// Define your subnet
IPAddress subnet(255, 255, 255, 0);
// Cloudflare DNS (can be another like google's or a local one of your choice)
IPAddress dns(1, 1, 1, 1);

// getPublicIp attempts 3 times to get the router's public ip, waiting 5 seconds for each reattempt
String getPublicIp() {
  String ip;
  for (int attempt = 1; attempt <= 3; attempt++) {

    WiFiClientSecure client;
    // Set the Esp8266 to trust ipify's certificate
    client.setTrustAnchors(&ipifyCert);

    // Set the GET request on a single string
    if (client.connect("api.ipify.org", 443)) {
      Serial.println("Connected to api.ipify.org");
      client.print(String("GET / HTTP/1.0\r\n"
                          "Host: api.ipify.org\r\n"
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
      Serial.println("Connection to ipify.org failed on attempt " + String(attempt));
    }

    if (attempt < 3) {
      Serial.println("Waiting 2 seconds before reattempt...");
      yield();
      delay(2000);
    }
  }

  return String();  // Return an empty string after 3 failed attempts
}

bool isValidIP(const String& ip) {
  IPAddress addr;
  return addr.fromString(ip);
}

void synchTime() {
  // Synchronizes the time to an NTP server, after that, you can access the epoch time (# of seconds since Jan 1 1970) with time(nullptr)
  const int timeZone = 0;        // Change this to your time zone offset in seconds
  const int daylightOffset = 0;  // x hour offset for Daylight Saving Time (DST)
  Serial.print("Synching time: ");
  configTime(timeZone, daylightOffset, "pool.ntp.org");  // Get and set the time to UTC0 via an NTP server
  while (time(nullptr) < 24 * 3600) {
    Serial.print(".");
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

void sendPublicIp(String messagePrefix) {
  String ip = persintentGetPublicIp();
  // Set the public ip now to avoid resending it in the first checkPublicIpChange check
  publicIp = ip;
  sendTelegramMessage(messagePrefix + ip);
}

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

void connectAndSendIp(String messagePrefix) {
  connectToWiFi();
  sendPublicIp(messagePrefix);
}

void checkAndReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectAndSendIp("WiFi connection lost. Public IP: "); // Reconnect when the connection is lost
    delay(3000);
  }
}

void checkPublicIpChange () {
  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Check if 60 minutes (3600000 milliseconds) have passed since the last check
  if (currentTime - lastCheckTime >= 3600000) {
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