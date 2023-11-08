// auth_utils.h
#ifndef AUTH_UTILS
#define AUTH_UTILS

// Function that returns the SHA256 hash for the provided string
String calculateSHA256Hash(const String &inputString) {
  SHA256 sha256;
  byte hash[32];
  sha256.reset();

  // Convert the String to a char* using c_str()
  const char *charArray = inputString.c_str();

  sha256.update(charArray, strlen(charArray));
  sha256.finalize(hash, 32);

  char hashHex[65];  // Each byte corresponds to 2 hexadecimal characters, plus a null terminator
  for (int i = 0; i < 32; i++) {
    sprintf(hashHex + 2 * i, "%02x", hash[i]);
  }
  hashHex[64] = '\0';

  return String(hashHex);
}

// Function that returns a random 16 character string
String generateRandomString() {
  const int length = 16;
  String charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String result = "";

  for (int i = 0; i < length; i++) {
    int index = random(charset.length());
    result += charset.charAt(index);
  }

  return result;
}

#endif