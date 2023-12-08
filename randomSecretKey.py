import random
import string

def generate_random_string():
    characters = string.ascii_uppercase + ''.join(str(i) for i in range(2, 8))
    return ''.join(random.choice(characters) for _ in range(16))

# Example usage:
random_string = generate_random_string()
print(random_string)
