"""Dedicated network API entrypoint; recording lifecycle belongs to the recorder service."""
import argparse
import os
import uvicorn
from secure_server import create_app, NVRService, generate_certificate, CERT_FILE, KEY_FILE, checked_allowed_networks
from security_policy import require_vault
from backend import load_config


def config_from_args(args):
    allowed = checked_allowed_networks(args.allow_network)
    if args.host not in ('127.0.0.1', '::1') and not allowed:
        raise ValueError('LAN service requires explicitly approved private CIDR')
    require_vault(load_config()['record_dir'])
    return allowed


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--host', default=os.environ.get('DYNAMICOS_API_HOST','127.0.0.1'))
    parser.add_argument('--port', type=int, default=8443)
    parser.add_argument('--allow-network', action='append', default=[x for x in os.environ.get('DYNAMICOS_ALLOWED_NETWORKS','').split(',') if x])
    args=parser.parse_args()
    allowed=config_from_args(args)
    generate_certificate(hosts=('127.0.0.1',args.host))
    service=NVRService()
    try:
        uvicorn.run(create_app(service, allowed_networks=allowed),host=args.host, port=args.port,
          ssl_certfile=str(CERT_FILE),ssl_keyfile=str(KEY_FILE),server_header=False,access_log=False)
    finally:
        service.relay.stop()
        service.recording.shutdown()

if __name__=='__main__':main()
