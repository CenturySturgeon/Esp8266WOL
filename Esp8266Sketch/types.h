// types.h
#include <ESP8266WebServerSecure.h>

// Importing Rhys Weatherly's cryptography library to use SHA256 encryption
#include <Crypto.h>
#include <SHA256.h>

// UserSession struct for the handling of session data
struct UserSession {
  String localUName;   // Holds a local username (doesn't need to match the credential's) for easier, human readable tracking
  String credentials;  // Hash that holds the session credentials (a mix of your username and password)
  IPAddress ip;
  bool isLoggedIn;
  unsigned long sessionStart;  // Time of the session begining in milliseconds
  unsigned long lifeTime;      // Maximum session lifetime in seconds
};

// Define the SecureServer struct
struct SecureServer {
  BearSSL::ESP8266WebServerSecure server;
  UserSession userSessions[2];

  // Constructor for SecureServer
  SecureServer(int serverPort, UserSession *sessions)
    : server(serverPort) {
    for (int i = 0; i < 2; i++) {
      userSessions[i] = sessions[i];
    }
  }

  // Simple function to check if a client ip has already an active session
  bool is_authenticated(IPAddress ip) {
    for (int i = 0; i < 2; i++) {
      if (userSessions[i].ip == ip && userSessions[i].isLoggedIn) {
        return true;
      }
    }
    return false;
  }

  // Handles the logout of a session for an ip
  void logout(IPAddress ip) {
    for (int i = 0; i < 2; i++) {
      if (userSessions[i].ip == ip) {
        userSessions[i].ip = IPAddress(127, 0, 0, 1);
        userSessions[i].isLoggedIn = false;
        userSessions[i].sessionStart = 0;
      }
    }
  }

  // Simple function that checks the provided credentials match with the credentials on the sessions array
  bool credentialsMatch(String credentials) {
    for (int i = 0; i < 2; i++) {
      if (userSessions[i].credentials == credentials) {
        return true;
      }
    }
    return false;
  }

  // Assigns the ip a session to the provided user name
  void assignSession(String credentials, IPAddress ip) {
    for (int i = 0; i < 2; i++) {
      if (userSessions[i].credentials == credentials) {
        userSessions[i].ip = ip;
        userSessions[i].isLoggedIn = true;
        userSessions[i].sessionStart = millis();
      }
    }
  }

  // Returns wether or not there's an existing session for the given ip, and also has the ability to create a new session
  bool handleAuthentication(String credentials) {
    IPAddress clientIp = server.client().remoteIP();  // Get the client's IP address
    bool clientAuthenticated = is_authenticated(clientIp);
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

  void redirectTo(String path) {
    server.sendHeader("Location", path, true);  // Redirect to the login path
    server.send(301, "text/plain", "Redirecting to " + path);
  }
};