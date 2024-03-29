// types.h
#ifndef TYPES_H
#define TYPES_H

#include <WiFiUdp.h>
#include <ESP8266WebServerSecure.h>

// For Time-based One Time Passwords
#include <TOTP.h>
// Importing Rhys Weatherly's cryptography library to use SHA256 encryption
#include <Crypto.h>
#include <SHA256.h>
// Importing a7md0's WakeOnLan library
#include <WakeOnLan.h>
// Import the hash calculation and random string generation functions
#include "auth_utils.h"

// UserSession struct for the handling of session data
struct UserSession {
  String localUName;   // Holds a local username (doesn't need to match the credential's) for easier, human readable tracking
  String credentials;  // Hash that holds the session credentials (a mix of your username and password)
  uint8_t hmacKey[10];  // Key to generate TOTP tokens
  unsigned long lifeTime;      // Maximum session lifetime in seconds
  IPAddress ip = IPAddress(127, 0, 0, 1); // 127.0.0.1 corresponds to the loopback address (localhost) and is not routable on the public internet
  bool isLoggedIn = false;
  String token = ""; // Token to validate the client's session identity alongside the IP
  unsigned long sessionStart = 0;  // Time of the session begining in milliseconds
};

// SecureServer struct holds the ESP8266WebServerSecure, the userSessions, the TOTP object, and the WOL object
struct SecureServer {
  BearSSL::ESP8266WebServerSecure server;
  int sessionCount;
  UserSession* userSessions;
  WakeOnLan WOL;
  int ipRetries = 3;

  // Constructor for SecureServer
  SecureServer(int serverPort, UserSession *sessions, WiFiUDP &udp, int numUSessions)
    : server(serverPort), WOL(udp), sessionCount(numUSessions) {
    userSessions = new UserSession[sessionCount]; // Allocate memory for userSessions
    for (int i = 0; i < sessionCount; i++) {
      userSessions[i] = sessions[i];
    }
  }

  // Destructor to release memory
  ~SecureServer() {
    delete[] userSessions;
  }

  // Function that checks if a client ip has already an active session. If so, it closes the old one.
  bool is_authenticated(IPAddress ip, String token) {
    for (int i = 0; i < sessionCount; i++) {
      if (userSessions[i].ip == ip && userSessions[i].isLoggedIn && userSessions[i].token != "" && token.indexOf("Esp8266AuthCookie=" + userSessions[i].token) != -1) {
        return true;
      }
    }
    return false;
  }

  // Handles the session logout for an ip
  void logout(IPAddress ip) {
    for (int i = 0; i < sessionCount; i++) {
      if (userSessions[i].ip == ip) {
        userSessions[i].ip = IPAddress(127, 0, 0, 1);
        userSessions[i].isLoggedIn = false;
        userSessions[i].sessionStart = 0;
        userSessions[i].token = "";
      }
    }
  }

  // Function that checks the provided credentials match with the credentials on the sessions array
  bool credentialsMatch(String credentials) {
    for (int i = 0; i < sessionCount; i++) {
      if (userSessions[i].credentials == credentials) {
        return true;
      }
    }
    return false;
  }

  // Assigns the ip to a session of the provided user name
  void assignSession(String credentials, IPAddress ip) {
    for (int i = 0; i < sessionCount; i++) {
      if (userSessions[i].credentials == credentials) {
        userSessions[i].ip = ip;
        userSessions[i].isLoggedIn = true;
        userSessions[i].sessionStart = millis();
        userSessions[i].token = calculateSHA256Hash(generateRandomString());
        sendCookieToClient(userSessions[i].token, String(userSessions[i].lifeTime));
      }
    }
  }

  // Returns wether or not there's an existing session for the given ip, and also has the ability to create a new session
  bool handleAuthentication(String credentials, String token) {
    IPAddress clientIp = server.client().remoteIP();  // Get the client's IP address
    bool clientAuthenticated = is_authenticated(clientIp, token);
    bool goodCredentials = credentialsMatch(credentials);

    if (clientAuthenticated) {
      return true;
    } else {
      if (goodCredentials) {
        assignSession(credentials, clientIp);
        return true;
      } else {
        return false;
      }
    }
  }

  bool isPinValid(String pin){
    IPAddress clientIp = server.client().remoteIP();
    for (int i = 0; i < sessionCount; i++) {
      if (userSessions[i].ip == clientIp) {
        return String(TOTP(userSessions[i].hmacKey, 10).getCode(time(nullptr))) == pin;
      }
    }
    return false;
  }

  String getAuthCookie() {
  if (server.hasHeader("Cookie")) {
      String cookieHeader = server.header("Cookie");
      return cookieHeader;
  }
    return "";
  }

  void sendCookieToClient(String token, String lifeTime) {
    // name:value, expiration age in seconds, path(s) to were the cookie applies
    String cookie = "Esp8266AuthCookie=" + token + "; Max-Age=" + lifeTime + "; Path=/; Secure; HttpOnly; SameSite=Strict";
    // Secure: Ensures that the cookie is only sent over HTTPS.
    // HttpOnly:Prevents client-side scripts from accessing the cookie.
    // SameSite=Strict: Ensures that the cookie is only sent in a first-party context, providing some protection against CSRF attacks.
    
    server.sendHeader("Set-Cookie", cookie);
  }

  void resetIpRetries () {
    ipRetries = 3;
  }

  void redirectTo(String path) {
    server.sendHeader("Location", path, true);
    server.send(301, "text/plain", "Redirecting to " + path);
  }
};

#endif