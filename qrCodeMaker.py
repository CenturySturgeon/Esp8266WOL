import pyotp
import base64
import qrcode
from PIL import Image

### The purpose of this script is to generate a qr code readable by Microsoft authenticator to generate One Time Passwords

# Replace this with your secret key (16 characters A-Z and 2-7)
secret_key = "YOUR_SECRET_KEY"

# Replace this with your desired label (e.g., "MyAccount" or the account name)
label = "MyAccount"

# Create a TOTP object
totp = pyotp.TOTP(secret_key)

# Generate the otpauth URI
uri = totp.provisioning_uri(name=label, issuer_name="YourIssuer")

# Create a QR code image
img = qrcode.make(uri)

# Save the QR code as an image
img.save("qrcode.png")

# The arduino librari requires the key to be represented in b32 values. Decode the key string and store it in a byte array:
byte_array = base64.b32decode(secret_key)

# Print the byte array in a format that you can use in Arduino
formatted_byte_array = ', '.join([f'0x{byte:02X}' for byte in byte_array])
print(f"uint8_t hmacKey[] = {{ {formatted_byte_array} }};")

# Display the QR code
img.show()
