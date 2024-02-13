import os
from typing import List, Dict
from CredentialUtils import UserSession

def quotate(input: str) -> str:
    """Returns the input between double quotes."""
    return '\"' + input + '\"'

def create_envVariables_file(router_info: tuple[str, str, tuple[int, int, int, int], tuple[int, int, int, int]],user_sessions: List[UserSession], static_ip: tuple[int, int, int, int], telegram_info: tuple[str, str, tuple[str, Dict | None]], cert_and_pkey: tuple[bytes, bytes], ip_site_cert: tuple[str, Dict | None]):
    """Creates the envVariables.h file with the provided information."""
    # Get the placeHolder.h absolute file path
    module_dir = os.path.dirname(__file__)
    file_path = os.path.join(module_dir, 'placeHolder.h')

    # Read the placeHolders.h file
    with open(file_path, 'r') as file:
        content = file.read()

    wifi_name, wifi_password, gateway, subnet = router_info
    telegram_bot_token, telegram_chat_id, telegram_api_cert = telegram_info
    certificate, private_key = cert_and_pkey

    # Replace placeholders with user values
    content = content.replace("{{WIFI_SSID}}", quotate(wifi_name))
    content = content.replace("{{WIFI_PASSWORD}}", quotate(wifi_password))
    content = content.replace("{{NUMBER_OF_SESSIONS}}", str(len(user_sessions)))
    content = content.replace("{{USER_SESSIONS_ARRAY}}", ',\n  '.join([str(usession.toString()) for usession in user_sessions]))
    content = content.replace("{{STATIC_IP}}", str(static_ip))
    content = content.replace("{{LOCAL_GATEWAY}}", str(gateway))
    content = content.replace("{{SUBNET}}", str(subnet))
    content = content.replace("{{BOT_TOKEN}}", quotate(telegram_bot_token))
    content = content.replace("{{CHAT_ID}}", quotate(telegram_chat_id))
    content = content.replace("{{SERVER_CERTIFICATE}}", certificate.decode().removesuffix('\n'))
    content = content.replace("{{PRIVATE_KEY}}", private_key.decode().removesuffix('\n'))
    content = content.replace("{{TELEGRAM_CERT}}", telegram_api_cert[0].removesuffix('\n'))
    content = content.replace("{{IP_SITE_CERT}}", ip_site_cert[0].removesuffix('\n'))

    # print(content)

    # Write the envVariables.h file
    with open('envVariables.h', 'w') as file:
        file.write(content)