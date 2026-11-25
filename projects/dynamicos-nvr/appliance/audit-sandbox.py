#!/usr/bin/env python3
"""Fail closed on missing baseline systemd controls and insecure installer configuration."""
from pathlib import Path

RECORDER_REQUIRED={
'NoNewPrivileges':'yes','ProtectSystem':'strict','ProtectHome':'yes','PrivateTmp':'yes',
'PrivateDevices':'yes','PrivateIPC':'yes','RestrictNamespaces':'yes','ProtectKernelModules':'yes',
'ProtectKernelTunables':'yes','ProtectKernelLogs':'yes','ProtectControlGroups':'yes',
'ProtectProc':'invisible','RestrictSUIDSGID':'yes','SystemCallArchitectures':'native',
'IPAddressDeny':'any','SystemCallFilter':'@system-service','CapabilityBoundingSet':'',
'ConditionPathIsMountPoint':'/var/lib/dynamicos-nvr',
}

def parse_unit(path):
    fields={}
    for raw in Path(path).read_text().splitlines():
        line=raw.strip()
        if line and not line.startswith(('#','[')) and '=' in line:
            key,value=line.split('=',1)
            fields.setdefault(key,[]).append(value)
    return fields

def verify(unit):
    fields=parse_unit(unit)
    problems=[]
    for key,wanted in RECORDER_REQUIRED.items():
        if wanted not in fields.get(key,[]):problems.append(f'{key} missing or incorrect')
    if '/var/lib/dynamicos-nvr' not in ' '.join(fields.get('ReadWritePaths',[])):
        problems.append('Missing constrained writable vault path')
    if fields.get('User')!=['dynamicos']:problems.append('Must not run recorder as root')
    return problems

if __name__=='__main__':
    unit=Path(__file__).with_name('dynamicos-recorder.service')
    problems=verify(unit)
    if problems:
        for problem in problems:print('FAIL:',problem)
        raise SystemExit(1)
    print('PASS: recorder sandbox baseline')
