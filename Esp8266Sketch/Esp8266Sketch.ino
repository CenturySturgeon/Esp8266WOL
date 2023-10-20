#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <TOTP.h> // For Time-based One Time Passwords

// Import the html files
#include "wol_html.h"
#include "login_html.h"
// Import the environment variables (ssid, password, static IP, default local gateway (get it from your router) & hmacKey)
#include "envVariables.h"

// Define NTP Server and Time Zone (remember, most authenticator apps will use the default values, UTC0, to avoid timezone issues)
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 0;       // Change this to your time zone offset in seconds
const int daylightOffset = 0; // x hour offset for Daylight Saving Time (DST)

IPAddress subnet(255, 255, 255, 0);
IPAddress dns(1, 1, 1, 1); // Cloudflare DNS (can be another like google's or a local one of your choice)

// Set the main webserver on port 443 (HTTPS)
BearSSL::ESP8266WebServerSecure server(443);
BearSSL::ServerSessions serverCache(5);
// Set a secondary web server for HTTP redirection to HTTPS
ESP8266WebServer serverHTTP(80);

WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName, timeZone);

// Set the TOTP key to be used for code generation
TOTP totp = TOTP(hmacKey, 10);

// Initial value for the TOTP code
String totpCode = String("");

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
          break; // Retry on timeout
        }
      }

      while (client.available()) {
        publicIp = client.readStringUntil('\n');
        Serial.println(publicIp);
      }

      // Close the connection
      client.stop();

      if (publicIp.length() > 0) {
        return publicIp; // Successfully obtained the IP
      }

      Serial.println("IP not received on attempt " + String(attempt));
    } else {
      Serial.println("Connection to ipify.org failed on attempt " + String(attempt));
    }

    if (attempt < 3) {
      Serial.println("Waiting 5 seconds before reattempt...");
      delay(5000); // Wait 5 seconds before reattempt
    }
  }

  return String(); // Return an empty string after 3 failed attempts
}

// UserSession struct for the handling of session data
struct UserSession
{
  String username;
  String password;
  IPAddress ip;
  bool isLoggedIn;
  ;
};

// User session array for the handling of session states (default should always have no session)
UserSession userSessions[2] = {
    // 127.0.0.1 corresponds to the loopback address (localhost) and is not routable on the public internet
    { "admin", "admin", IPAddress(127, 0, 0, 1), false },
    { "user", "user", IPAddress(127, 0, 0, 1), false }
};

// Simple function to check if a client ip has already an active session
bool is_authenticated(IPAddress ip) {
  for (int i = 0; i < 2; i++) {
    if (userSessions[i].ip == ip && userSessions[i].isLoggedIn) {
      return true;
    }
  }
  return false;
}

// Simple function that checks the provided credentials match with the credentials on the array
bool credentialsMatch(String username, String password) {
  for (int i = 0; i < 2; i++) {
    if (userSessions[i].username == username && userSessions[i].password == password) {
      return true;
    }
  }
  return false;
}

// Assigns the ip a session to the provided user name
void assignSession(String username, IPAddress ip) {
  for (int i = 0; i < 2; i++) {
    if (userSessions[i].username == username) {
      userSessions[i].ip = ip;
      userSessions[i].isLoggedIn = true;
    }
  }
}

void handleAuthentication(String username, String password) {
  IPAddress clientIp = server.client().remoteIP(); // Get the client's IP address
  Serial.println(clientIp);
  bool clientAuthenticated = is_authenticated(clientIp);
  bool goodCredentials = credentialsMatch(username, password);

  if (clientAuthenticated) {
    server.send(200, "text/plain", "Welcome to the protected page!");
  } else {
    if (goodCredentials) {
      assignSession(username, clientIp);
      server.send(200, "text/plain", "Welcome to the protected page!");
    } else {
      server.send(200, "text/html", login_html);
    }
  }
}

// Redirects incoming HTTP trafic to the HTTPS server
void secureRedirect() {
  serverHTTP.sendHeader("Location", String("https://esp8266.local"), true);
  serverHTTP.send(301, "text/plain", "");
}

void setup()
{
  // Start Serial for debugging
  Serial.begin(115200);

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

  // Initialize the time client
  timeClient.begin();
  timeClient.update();

  if (MDNS.begin("esp8266")){
    Serial.println("MDNS responder started");
  }

  // Add the openssl cert and private key
  server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  // Cache SSL sessions to accelerate the TLS handshake.
  server.getServer().setCache(&serverCache);

  server.on("/", HTTP_GET, []() {
    handleAuthentication("", "");
  });

  // Handle the login submission
  server.on("/login", HTTP_POST, []() {
    timeClient.update();
    String username = server.arg("username");
    String password = server.arg("password");
    handleAuthentication(username, password);
  });

  // Handle the form submission
  server.on("/submit", HTTP_POST, []() {
    timeClient.update();
    String inputText = server.arg("inputText");
    Serial.print("Submitted Text: ");
    Serial.println(inputText);
    String newCode = String(totp.getCode(timeClient.getEpochTime()));
    server.send(200, "text/html", "You submitted: " + inputText + " The real code: " + newCode + " " + timeClient.getFormattedTime());
  });

  // Redirect all users using HTTP to the HTTPS server
  serverHTTP.on("/", HTTP_GET, secureRedirect);

  server.begin();
  serverHTTP.begin();

  String publicIP = getPublicIp();
}

void loop() {
  serverHTTP.handleClient();
  server.handleClient();
  MDNS.update();
}
