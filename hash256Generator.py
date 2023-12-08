import hashlib

def generate_sha256_hash(input_string):
    sha256_hash = hashlib.sha256(input_string.encode()).hexdigest()
    return sha256_hash

# Example usage:
input_string = "admin:admin"
sha256_hash = generate_sha256_hash(input_string)
print(f"Input string: {input_string}")
print(f"SHA-256 hash: {sha256_hash}")
