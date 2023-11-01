# Esp8266WOL: Wake devices on your LAN, over the internet, using an SoC

Configure your Esp8266 as a web server, equipping it with the capability to dispatch magic packets for awakening any device within your local area network (LAN). The web server implements HTTPS for secure and private connections, bolstered by Two-Factor Authentication (2FA) through Time-Based One-Time Passwords (TOTP) to thwart potential threats seeking to rouse your devices maliciously.

![Diagram](https://github.com/CenturySturgeon/Esp8266WOL/blob/main/WOL_Diagram.svg)

### NOTES

This code uses certificates from the different sites used to enhance security, they're used to verify the identity of the sites used to get or send information to. You can easily get certificates from any website using openssl:

```
openssl s_client -connect www.your-site.com:443
```

Copy the output from the terminal connection from '-----BEGIN CERTIFICATE-----' to '-----END CERTIFICATE-----' INCLUDING those two lines.

For example, to get the telegram cert for your bot api messages:

```
openssl s_client -connect api.telegram.org:443
```

Even though the essential custom variables are found on the 'envVariables.h' file, on the 'wifi_utils.h' file you'll find aditional customizable variables you can modify, like the DNS server used, the site where the SoC gets the public IP, the NTP server to synch the time with, the time interval to check for public IP changes and more.

The Crypto library is used to securely safekeep passwords as SHA-256 encrypted hashes amd not in plain text. You can install this library directly from the Arduino IDE, it's the one from Rhys Weatherly, more of this in the github repo https://github.com/rweather/arduinolibs. To generate your hashes, you can use online tools like https://emn178.github.io/online-tools/sha256.html or use the library and print them to serial for later use (which is safer than trusting a random site).

You can read more about the WOL library used in this code at https://github.com/a7md0/WakeOnLan.

### Requirements

The Esp8266 device loads your credentials, keys and certificate from a file 'envVariables.h' that should look like this:

```
// envVariables.h
#ifndef ENVVARIABLES_H
#define ENVVARIABLES_H

// Replace with your network credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Set static IP address
IPAddress staticIP(192, 168, 7, 77);
// Get the default local gateway from your router
IPAddress gateway(192, 168, 8, 88);
// Set your subnet (the default one should probably work)
IPAddress subnet(255, 255, 255, 0);

// Set the hmacKey for TOTP generation
uint8_t hmacKey[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Set your telegram bot API token
String BOT_TOKEN = "XXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

// Set your telegram user id
String CHAT_ID = "XXXXXXXXXX";


// Cert for ssl connection over HTTPS
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
//    YOUR CERT HERE    //
-----END CERTIFICATE-----
)EOF";

// Private key for ssl connection over HTTPS
static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
//    YOUR CERT HERE    //
-----END RSA PRIVATE KEY-----
)EOF";


const char telegramRootCert [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
//    YOUR CERT HERE    //
-----END CERTIFICATE-----
)CERT";

const char ipSiteRootCert [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
//    YOUR CERT HERE    //
-----END CERTIFICATE-----
)CERT";

#endif
```

This file should be on the same level as the Esp8266Sketch.ino. If you cloned this repo, the folder structure should look like this:

```
Esp8266Sketch/
├── utils/
├── views/
├── envVariables.h
└── Esp8266Sketch.ino
.gitignore
qrCodeMaker.py
```

### Generating Requirements

The 'ssid' and 'password' variables hold your WiFi network name and password respectively. These allow the Esp8266 to connect to your WiFi, enabling it to send the magic packets to any device in that network.

The 'staticIp' makes it so your Esp8266 always has the same ip on your network. This is specially usefull if you're planning to access the web server over the internet via port-forwarding, although you can also configure this from the router using the SoC's MAC address. If you only want to use the web server on your LAN just set the ip to an address currently not used in your network.

To avoid issues when communicating to the internet, the Esp8266 needs to know your routers default local gateway and store it in the 'gateway' variable. Depending on your OS, you can look for tutorials on how to get it.

The 'BOT_TOKEN' and 'CHAT_ID' variables store information regarding your telegram bot. This is usefull if you're planning to access your SoC's website from outside the LAN since you'll need the public IP from your router. This code makes it so that the Esp8266 sends you the public IP upon startup and keeps looking for changes in the public IP every hour (you can change this on the wifi_utils.h file) to re-send it if it changed, all of this via your telegram bot.

The 'hmacKey' is the key used to generate one-time passwords for 2FA. In this repository you can find a python script 'qrCodeMaker.py' that creates your hmacKey hex values and a qr code you can later scan with your prefered authenticator app (Google authenticator, Microsoft authenticator, etc.). You can follow the instructions inside the script so you can replace the hex values into the hmacKey inside the 'envVariables.h' file.

The 'telegramRootCert' and 'ipSiteRootCert' are certificates for the telegram API webiste and the site used to get the public ip from (api.ipify.org is the default, although you can change it on the wifi_utils.h file). This certificates are important since the SoC needs to verify the identity of these sites in order to avoid sending (when sending messages to the telegram bot) or receiving (when receiving the public IP) tampered information.

Finally, the private key 'serverKey' and the ssl certificate 'serverCert'. These certificate and private key are used to create a secure connection over HTTPS for the SoC's website. To generate them you can use the following commands:

```
openssl genrsa -out key.txt 1024

openssl rsa -in key.txt -out key.txt

openssl req -sha256 -new -nodes -key key.txt -out cert.csr -subj '/C=XX/ST=XX/L=XX/O=XX [RO]/OU=XX/CN=esp8266.local' -addext subjectAltName=DNS:esp8266.local

openssl x509 -req -sha256 -days 365 -in cert.csr -signkey key.txt -out cert.txt
```

Once you run these commands you can get the certificate and key from the .txt files. But before you create the ceritificate and the private key, you need to understand the commands variable names and their meaning:

    C  - Country (abreviated)
    ST - State or province
    L  - Locality or city
    O  - Organization
    OU - Organizational unit
    CN - Common name (domain name)

The subjectAltName parameter must contain the domain name(s) where your server is accessible. It can specify also IP addresses like this: 'subjectAltName=DNS:esp8266.local,IP:192.168.7.77'.
