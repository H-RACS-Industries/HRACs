# securepasscode.py
from passlib.context import CryptContext

pwd_config = CryptContext(schemes=["pbkdf2_sha256"],
                          default="pbkdf2_sha256",
                          pbkdf2_sha256__default_rounds=1000
                          )
def encrypt_password(user_password):
    return pwd_config.hash(user_password)

def check_hash(hashed_password, user_password):
    return pwd_config.verify(user_password, hashed_password)

def generate_confirmation_string(*args):
    unhashed_string = ''
    for arg in args:
        unhashed_string += str(arg)
    return unhashed_string
