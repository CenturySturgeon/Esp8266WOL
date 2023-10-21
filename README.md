# Esp8266WOL: Wake devices on your LAN using an SoC

This program configures your Esp8266 as a web server, equipping it with the capability to dispatch magic packets for awakening any device within your local area network (LAN). The web server implements HTTPS for secure and private connections, bolstered by Two-Factor Authentication (2FA) through Time-Based One-Time Passwords (TOTP) to thwart potential threats seeking to rouse your devices maliciously.

### Requirements

The Esp8266 device loads your credentials, keys and certificate from a file 'envVariables.h' that should look like this:

```
// Replace with your network credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Set the static IP address for your Esp8266 (especially important for port forwarding)
IPAddress staticIP(192, 168, 7, 77);

// Get the default local gateway from your router
IPAddress gateway(192, 168, 8, 88);

// Replace with your hmacKey hex values for TOTP generation (DO NOT use this one)
uint8_t hmacKey[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Your certificate for ssl connection over HTTPS (DO NOT use this one)
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIICPjCCAacCFD+seHHSMYXsW9CK14lEbZRJBDeUMA0GCSqGSIb3DQEBCwUAMF4x
CzAJBgNVBAYTAlhYMQswCQYDVQQIDAJYWDELMAkGA1UEBwwCWFgxEDAOBgNVBAoM
B1hYIFtST10xCzAJBgNVBAsMAlhYMRYwFAYDVQQDDA1lc3A4MjY2LmxvY2FsMB4X
DTIzMTAyMTAzMjQxNloXDTI0MTAyMDAzMjQxNlowXjELMAkGA1UEBhMCWFgxCzAJ
BgNVBAgMAlhYMQswCQYDVQQHDAJYWDEQMA4GA1UECgwHWFggW1JPXTELMAkGA1UE
CwwCWFgxFjAUBgNVBAMMDWVzcDgyNjYubG9jYWwwgZ8wDQYJKoZIhvcNAQEBBQAD
gY0AMIGJAoGBAL84K7WGVrjY+qZRiGVfQ7N3U1WQtN3XV/0cAAoVfXaoUr/q2MV9
plG401srgmSqMUYziMzm+S29KGX9XLLeXczIrwvdPW9vueplO/60VNjow3O1RXWh
9iZexBS+gnzIlh7/vB+jILDmwf+OgwHwf8hnaSbK76KrZwiKjqBoSdxPAgMBAAEw
DQYJKoZIhvcNAQELBQADgYEADoKvfFsmoN16W/1O+0hsn4OTTKQGolwInR+E3iWm
WR+n/QzvJpwV3OJcIsiIiIyYpqRp43RuZd3by9jOkg4Lu7v8aq3PqKQlquzTIuWJ
1JE7B++p0oXtX3oayhcAotBd/fRfr/AHwJx0pe9Czx4LABvRi5av5JIQ810X1c9o
Dyk=
-----END CERTIFICATE-----
)EOF";

// Your private key for ssl connection over HTTPS (DO NOT use this one)
static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQC/OCu1hla42PqmUYhlX0Ozd1NVkLTd11f9HAAKFX12qFK/6tjF
faZRuNNbK4JkqjFGM4jM5vktvShl/Vyy3l3MyK8L3T1vb7nqZTv+tFTY6MNztUV1
ofYmXsQUvoJ8yJYe/7wfoyCw5sH/joMB8H/IZ2kmyu+iq2cIio6gaEncTwIDAQAB
AoGALNqzPhB2FUQof40OjqDrE5UBAkhAdO1HoYLI65Sg6o2PY59mG+VedzsAsRki
rBRUHKCIDXB8sOL3L/1fmkGZBMGOZ44tJEVmYjweiXD8Sir5adBXNtBr4t1IVdkD
FOK5NN5MAhCV6PyR0uC898aXyezynbsqEqEMade0K4diY9kCQQDk6ynLecv2ZERo
s5W9m1nWGVP6oLGNjlnZl6jYCdn+5UzpsCrnwKpnEtjHVfZ0uAbByw77vAwgisSV
Fc5kGuWtAkEA1ddIDoWiMwh8h6K2NNOWocRfNnjAJ+CXh2bR5Dlsp1F5jXsjoeFj
wwfE3kJNN/HpgD166ZlfPLnQJ6mgDKTxawJBALC0qq2L1hrbKUddIQCc08sGACJe
OtCXitoBTO9/I0y0ehuFxojg22j3TmkI/Vb52xVrBqThcscH91lR3OC/Nn0CQGKk
Duob/Kkb1g3fIbuWitqOMfl3k/QRJvTfmhxz3MoFzFNWJSasSI+Tit2XqfUPk02D
jqopBISHlhxlo52RjIcCQEW1KUjUzI07pFBChZ+gdVOwzpM5BUenCIauF520PeS8
Rl61sLaVBI01IswtfZ+xegI6uXNL9wrAD6aWlvdqvp8=
-----END RSA PRIVATE KEY-----
)EOF";
```

You should place this file on the same level as the Esp8266Sketch.ino. If you cloned this repo, the folder structure should look like this:

```
Esp8266Sketch/
├── envVariables
├── Esp8266Sketch.ino
├── login_html.h
└── wol_html.h
.gitignore
qrCodeMaker.py
README.md
```

### Generating Requirements

If you don't know what any of the variables form 'envVariables.h' are, don't worry, you'll find the explanation bellow:

The 'ssid' and 'password' variables hold your WiFi network name and password respectively. These allow the Esp8266 to connect to your WiFi, enabling it to send the magic packets to any device in that network.

The 'staticIp' makes it so your Esp8266 always has the same ip on your network. This is specially usefull if you're planning to access the web server over the internet via port-forwarding. If you only want to use the web server on your LAN just set the ip to an address currently not used in your network.

To avoid issues when communicating to the internet, the Esp8266 needs to know your routers default local gateway. You can look for tutorials on how to get it by googling "how to get default gateway ip address".

The 'hmacKey' is the key used to generate one-time passwords for 2FA. In this repository you can find a python script 'qrCodeMaker.py' that creates your hmacKey hex values and a qr code you can later scan with your prefered authenticator app (Google authenticator, Microsoft authenticator, etc.). You can follow the instructions inside the script so you can replace the hex values into the hmacKey inside the 'envVariables.h' file.

Finally, the private key and the ssl certificate. To generate them you can use the following commands:

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
