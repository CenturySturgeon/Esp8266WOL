// wifi_utils for the handling of wifi events

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
          break;  // Retry on timeout
        }
      }

      while (client.available()) {
        publicIp = client.readStringUntil('\n');
        Serial.println(publicIp);
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
