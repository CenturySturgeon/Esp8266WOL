import ssl
import socket
from typing import Dict

def get_certificate_data(hostname: str, port: int = 443) -> Dict | None:
    context = ssl.create_default_context()
    with socket.create_connection((hostname, port)) as sock:
        with context.wrap_socket(sock, server_hostname=hostname) as sslsock:
            certificate = sslsock.getpeercert()
            return certificate

def get_certificate(hostname: str, port: int = 443) -> tuple[str, Dict | None]:
    cert = ssl.get_server_certificate((hostname, 443))
    certData = get_certificate_data(hostname, port)
    return (cert, certData)