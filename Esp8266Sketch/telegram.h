// telegram.h holds the functions necessary to send telegram messages
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

void sendTelegramMessage(String message, String botToken, String chatId, X509List &telegramCert) {
  WiFiClientSecure client;
  // client.setInsecure(); // Remember, setInsecure makes the client to not validate the ssl certificate
  client.setTrustAnchors(&telegramCert);

  if (client.connect("api.telegram.org", 443)) {
    String payload = "chat_id=" + String(chatId) + "&text=" + String(message);
    client.print(String("POST /bot") + botToken + "/sendMessage HTTP/1.1\r\n" +
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