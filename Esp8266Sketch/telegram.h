// telegram.h
#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "envVariables.h"

// Create a list of certificates with the telegram certificate to send telegram messages using your bot
X509List telegramCert(telegramRootCert); 

void sendTelegramMessage(String message) {
  WiFiClientSecure client;
  // client.setInsecure(); // Remember, setInsecure makes the client to not validate the ssl certificate
  client.setTrustAnchors(&telegramCert);

  if (client.connect("api.telegram.org", 443)) {
    String payload = "chat_id=" + String(CHAT_ID) + "&text=" + String(message);
    client.print(String("POST /bot") + BOT_TOKEN + "/sendMessage HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "Content-Type: application/x-www-form-urlencoded\r\n" +
                 "Content-Length: " + payload.length() + "\r\n\r\n" +
                 payload);

    // Read and print the full response from the server
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  } else {
    Serial.println("Failed to connect to Telegram API server!");
    Serial.println(client.getWriteError());
  }

  client.stop();
}

#endif