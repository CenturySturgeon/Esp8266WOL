// wifi_utils.h
#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
// Import the environment variables
#include "envVariables.h"
// Import the telegram utils
#include "telegram.h"

// Create a list of certificates with the ipify certificate to get the public ip
X509List ipifyCert(ipifyRootCert);
// Create a list of certificates with the telegram certificate to send telegram messages using your bot
X509List telegramCert(telegramRootCert); 
// Define your subnet
IPAddress subnet(255, 255, 255, 0);
// Cloudflare DNS (can be another like google's or a local one of your choice)
IPAddress dns(1, 1, 1, 1);

// getPublicIp attempts 3 times to get the router's public ip, waiting 5 seconds for each reattempt
String getPublicIp(X509List &publicIpSiteCert) {
  String publicIp;
  for (int attempt = 1; attempt <= 3; attempt++) {

    WiFiClientSecure client;
    // Set the Esp8266 to trust ipify's certificate
    client.setTrustAnchors(&publicIpSiteCert);

    // Set the GET request on a single string
    if (client.connect("api.ipify.org", 443)) {
      Serial.println("Connected to api.ipify.org");
      client.print(String("GET / HTTP/1.0\r\n"
                          "Host: api.ipify.org\r\n"
                          "Connection: close\r\n\r\n"));

      // Set a timeout for connecting
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println("Client timeout");
          client.stop();
          break;  // Retry on timeout
        }
      }

      while (client.available()) {
        publicIp = client.readStringUntil('\n');
      }

      // Close the connection
      client.stop();

      if (publicIp.length() > 0) {
        return publicIp;  // Successfully obtained the IP
      }

      Serial.println("IP not received on attempt " + String(attempt));
    } else {
      Serial.println("Connection to ipify.org failed on attempt " + String(attempt));
    }

    if (attempt < 3) {
      Serial.println("Waiting 5 seconds before reattempt...");
      delay(5000);  // Wait 5 seconds before reattempt
    }
  }

  return String();  // Return an empty string after 3 failed attempts
}

void connectToWiFi() {
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

  // Synchronizes the time to an NTP server, after that, you can access the epoch time (# of seconds since Jan 1 1970) with time(nullptr)
  const int timeZone = 0;        // Change this to your time zone offset in seconds
  const int daylightOffset = 0;  // x hour offset for Daylight Saving Time (DST)
  Serial.print("Synching time: ");
  configTime(timeZone, daylightOffset, "pool.ntp.org");  // Get and set the time to UTC0 via an NTP server
  while (time(nullptr) < 24 * 3600) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Time synched with NTP server on UTC 0");
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi sucessfully.");
  Serial.println(WiFi.localIP());
  String publicIP = getPublicIp(ipifyCert);
  sendTelegramMessage("Your public IP: " + publicIP, BOT_TOKEN, CHAT_ID, telegramCert);
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi, trying to connect...");
  WiFi.disconnect();
  delay(2000);
  connectToWiFi();
}

#endif