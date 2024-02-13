import os
import json
from dotenv import load_dotenv
from CredentialUtils import UserSession, CertUtils, CreateEnvFile

# Load environment variables from .env file
load_dotenv()

# Read environment variables
wifi_name = os.getenv('WIFI_NAME')
wifi_password = os.getenv('WIFI_PASSWORD')

esp8266_static_ip = tuple(map(int, os.getenv('ESP8266_STATIC_IP').split('.')))
router_local_gateway = tuple(map(int, os.getenv('ROUTER_LOCAL_GATEWAY').split('.')))
subnet = tuple(map(int, os.getenv('SUBNET').split('.')))
ip_site_url = os.getenv('IP_SITE_URL')

user_sessions_data = json.loads(os.getenv('USER_SESSIONS'))
user_sessions = [UserSession(session['username'], int(session['timeout']), session['totp_label'], session['issuer']) for session in user_sessions_data]

telegram_bot_token = os.getenv('TELEGRAM_BOT_TOKEN')
telegram_chat_id = os.getenv('TELEGRAM_CHAT_ID')
telegram_api_url = os.getenv('TELEGRAM_API_URL')

private_key_size = int(os.getenv('PRIVATE_KEY_SIZE'))
country_code = os.getenv('COUNTRY_CODE')
state = os.getenv('STATE')
city = os.getenv('CITY')
org = os.getenv('ORG')
domain_name = os.getenv('DOMAIN_NAME')
cert_lifespan_days = int(os.getenv('CERT_LIFESPAN_DAYS'))

# Re-order data
cert_and_pkey = CertUtils.create_certificate_and_private_key(private_key_size, country_code, state, city, org, domain_name, cert_lifespan_days)

router_info = (wifi_name, wifi_password, router_local_gateway, subnet)

telegram_info = (telegram_bot_token, telegram_chat_id, CertUtils.get_certificate(telegram_api_url))

ip_site_cert = CertUtils.get_certificate(ip_site_url)

# Create the envVariables.h file 

CreateEnvFile.create_envVariables_file(router_info, user_sessions, esp8266_static_ip, telegram_info, cert_and_pkey, ip_site_cert)