#!/usr/bin/env python3
"""Generate a reviewable nftables policy; NEVER auto-apply or flush existing firewall.
IPv4 only: IPv6 ingress is denied; use a dedicated, filtered camera VLAN.
"""
import argparse
import ipaddress
import sys

def private_net(value):
    net=ipaddress.ip_network(value, strict=False)
    if net.version!=4 or net.prefixlen<16 or not any(net.subnet_of(ipaddress.ip_network(p)) for p in ('10.0.0.0/8','172.16.0.0/12','192.168.0.0/16')):
        raise ValueError('Allowed client network must be narrow private IPv4 (/16 or smaller)')
    return str(net)

def generate(clients, cameras, port=8443):
    if not 1024<=port<=65535: raise ValueError('Port must be unprivileged')
    clients=[private_net(v) for v in clients]
    cameras=[ipaddress.ip_address(v) for v in cameras]
    if any(v.version!=4 or not v.is_private or v.is_loopback or v.is_link_local for v in cameras):
        raise ValueError('Camera IP must be a private unicast IPv4 address')
    if not clients:raise ValueError('At least one authorized client network required')
    # This policy intentionally does not replace a host's outbound routing policy.
    items=', '.join(clients); cams=', '.join(str(v) for v in cameras) if cameras else ''
    camrules=f'ip daddr {{ {cams} }} tcp dport {{ 554, 80, 8000, 8899 }} accept' if cameras else ''
    return f"""# REVIEW FIRST: installing this file affects remote administration.
# Apply using nft -c -f FILE, then nft -f FILE only from the local console.
table inet dynamicos_nvr {{
 chain input {{
  type filter hook input priority 10; policy drop;
  iifname "lo" accept
  ct state established,related accept
  ct state invalid drop
  ip protocol icmp accept
  ip saddr {{ {items} }} tcp dport {port} accept
 }}
 chain output {{
  type filter hook output priority 10; policy accept;
  {camrules}
 }}
}}
"""

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--allow-client-net',action='append',required=True)
    parser.add_argument('--camera-ip',action='append',default=[])
    parser.add_argument('--port',type=int,default=8443)
    ns=parser.parse_args()
    try:print(generate(ns.allow_client_net,ns.camera_ip,ns.port))
    except ValueError as exc:parser.error(str(exc))

if __name__=='__main__':main()
