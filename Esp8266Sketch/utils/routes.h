// routes.h handles the server's routes

// Importing Rhys Weatherly's cryptography library to use SHA256 encryption
#include <Crypto.h>
#include <SHA256.h>

// Import the hash calculation function
#include "auth_utils.h"

// Import the html files
#include "../views/wol_html.h"
#include "../views/login_html.h"
#include "../views/status_html.h"

// Sets the path handlers for the server variable inputed
void setServerRoutes(SecureServer &secureServer) {
  // Home path handler
  secureServer.server.on("/", HTTP_GET, [&]() {
    if (secureServer.handleAuthentication("")) {
      secureServer.redirectTo("/wol");
    } else {
      secureServer.redirectTo("/login");
    }
  });

  // WOL path handler
  secureServer.server.on("/wol", HTTP_GET, [&]() {
    if (secureServer.handleAuthentication("")) {
      secureServer.server.send(200, "text/html", wol_html);
    } else {
      secureServer.redirectTo("/login");
    }
  });

  // Success path handler
  secureServer.server.on("/success", HTTP_GET, [&]() {
    secureServer.server.send(200, "text/html", status_html);
  });

  // Error path handler
  secureServer.server.on("/error", HTTP_GET, [&]() {
    secureServer.server.send(200, "text/html", status_html);
  });

  // Login path handler
  secureServer.server.on("/login", HTTP_GET, [&]() {
    if (secureServer.handleAuthentication("")) {
      secureServer.redirectTo("/wol");
    } else {
      secureServer.server.send(200, "text/html", login_html);
    }
  });

  // Login submission handler
  secureServer.server.on("/login", HTTP_POST, [&]() {
    String username = secureServer.server.arg("username").substring(0, 16);
    String password = secureServer.server.arg("password").substring(0, 16);
    // Use a mix of the username and the password to create the credentials
    String credentials = calculateSHA256Hash(username + ":" + password);
    if (secureServer.handleAuthentication(credentials)) {
      secureServer.redirectTo("/wol");
    } else {
      secureServer.server.send(200, "text/html", login_html);
    }
  });

  // WOL submission handler
  secureServer.server.on("/wol", HTTP_POST, [&]() {
    String macAddress = secureServer.server.arg("macAddress").substring(0, 17);
    String secureOn = secureServer.server.arg("secureOn").substring(0, 17);
    String broadcastAddress = secureServer.server.arg("broadcastAddress").substring(0, 17);
    String pin = secureServer.server.arg("pin").substring(0, 6);

    // Check if authenticated and TOTP PIN match using current time (time(nullptr))
    if (secureServer.handleAuthentication("") && String(secureServer.totp.getCode(time(nullptr))) == pin) {

      // Send magic packet to the equipment
      if (secureOn != "") {
        Serial.println("Sent SecureOn");
        secureServer.WOL.sendSecureMagicPacket(macAddress.c_str(), secureOn.c_str());  // Convert String to const char *
      } else {
        Serial.println("Sent MAC only");
        secureServer.WOL.sendMagicPacket(macAddress.c_str());  // Convert String to const char *
      }
      // Return success page and logout
      macAddress.replace(" ", "%20");
      secureServer.redirectTo("success?message=Magic%20Packet%20sent%20to%20equipment:%20" + macAddress);
      secureServer.logout(secureServer.server.client().remoteIP());

    } else {
      secureServer.logout(secureServer.server.client().remoteIP());
      secureServer.redirectTo("error?message=Not%20Allowed");
    }
  });
}