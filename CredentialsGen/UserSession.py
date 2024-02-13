import random
import string
import pyotp
import base64
import qrcode
import hashlib


class UserSession:
    def __init__(self, user_name: str, session_timeout: int, totp_label: str ='MyAccount', totp_issuer: str ="YourIssuer"):
        """
        Creates an UserSession object that stores the randomly created credentials for the Esp8266.
        
        Params:
            user_name (str): The usersname for this session (also the name that its QR code .png image will have).
            session_timeout (int): The amount of time in seconds before the session times out.
            totp_label (str): The name the QR code will have in the TOTP authentication app.
            totp_issuer (str): The name of the institution (in this case yourself) that generated the TOTP QR code.

        Returns
            None 
        """
        
        self.uname = user_name
        self.timeout = str(session_timeout)
        self.pin = self.get_random_pin_Code()
        self.hashedPin = self.generate_sha256_hash(self.uname + ":" + self.pin)
        self.hmacKey = self.create_qr_code(self.uname, totp_label, totp_issuer)
    
    def generate_sha256_hash(input_string: str) -> str:
        """
        Returns the sha256 hash for the received input string.
        
        Parameters:
        input_string (str): The string to hash.

        Returns:
            str: The sha256 hash of input_string.
        """
        sha256_hash = hashlib.sha256(input_string.encode()).hexdigest()
        return sha256_hash

    def get_random_pin_Code() -> str:
        """Returns a random four digit code."""
        chars = string.digits
        pin =  ''.join(random.choice(chars) for _ in range(4))
        return pin

    def get_random_secret_key() -> str:
        """Returns a random secret key consisting of random A-Z characters and numbers from 2-8."""
        characters = string.ascii_uppercase + ''.join(str(i) for i in range(2, 8))
        return ''.join(random.choice(characters) for _ in range(16))
    
    def create_qr_code(self, user_name: str, label: str, issuer: str) -> str:
        """
        Creates a QR code for the TOTP authentication app, saves it, and returns its hmac key.
        
        Params:
            user_name (str): The name that the QR code .png image will have.
            label (str): The name this code will have in the TOTP authentication app.
            issuer (str): The name of the institution (in this case yourself) that generated this QR code.

        Returns
            str: The hmac key byte array as a string. 
        """
        # Replace this with your secret key (16 characters string composed of A-Z and 2-7). You can generate a random one using the randomSecretKey.py file.
        secret_key = self.get_random_secret_key()

        # Create a TOTP object
        totp = pyotp.TOTP(secret_key)

        # Generate the otpauth URI, YourIssuer is the label that will be assigned in the authenticator app
        uri = totp.provisioning_uri(name=label, issuer_name=issuer)

        # Create a QR code image, you can scan with your favorite authenticator app
        img = qrcode.make(uri)

        # The arduino librari requires the key to be represented in b32 values. Decode the key string and store it in a byte array:
        byte_array = base64.b32decode(secret_key)

        # Print the byte array in a format that you can use in Arduino
        hmac_key = ', '.join([f'0x{byte:02X}' for byte in byte_array])
        # print(f"uint8_t hmacKey[] = {{ {hmac_key} }};")

        # Save the QR code in the same folder as the script is located
        img.save(user_name + ".png")

        return hmac_key
