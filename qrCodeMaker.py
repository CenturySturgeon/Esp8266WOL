import pyotp
import base64
import qrcode
from PIL import Image

### The purpose of this script is to generate a qr code readable by authenticator apps to generate One Time Passwords
### Alternatively, you can directly use the secret_key on the authenticator app and just use the hmackey output for the arduino code

# Replace this with your secret key (16 characters string composed of A-Z and 2-7)
secret_key = "YOUR_SECRET_KEY"

# Replace this with your desired label (e.g., "MyAccount" or the account name)
label = "MyAccount"

# Create a TOTP object
totp = pyotp.TOTP(secret_key)

# Generate the otpauth URI, YourIssuer is the label that will be assigned in the authenticator app
uri = totp.provisioning_uri(name=label, issuer_name="YourIssuer")

# Create a QR code image, you can scan with your favorite authenticator app
img = qrcode.make(uri)

# Save the QR code in the same folder as the script is located
img.save("qrcode.png")

# The arduino librari requires the key to be represented in b32 values. Decode the key string and store it in a byte array:
byte_array = base64.b32decode(secret_key)

# Print the byte array in a format that you can use in Arduino
formatted_byte_array = ', '.join([f'0x{byte:02X}' for byte in byte_array])
print(f"uint8_t hmacKey[] = {{ {formatted_byte_array} }};")

# Display the QR code
img.show()
