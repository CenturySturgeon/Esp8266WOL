// wifi_utils for the handling of wifi events
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

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
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi sucessfully.");
  Serial.println(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi, trying to connect...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
}