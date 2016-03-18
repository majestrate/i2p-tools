
__doc__ = """
config reader for i2p.tun
"""

import json
import os


def genconfig():
    """
    generate default config
    """
    return {
        "keyfile" : "i2tun.key",
        "interface" : {
            "ifname" : "i2p0",
            "netmask" : "255.255.0.0",
            "mtu" : 4000,
            "tap" : False,
        },
        "clump" : False,
        "map" : {
        },
        "sam" : {
            "controlHost" : "127.0.0.1",
            "controlPort" : 7656,
            "dgramHost" : "127.0.0.1",
            "dgramPort" : 7655,
            "dgramBind" : "127.0.0.1",
            "opts" : {
            },
        }
    }

def load(fname="i2tun.json"):
    if not os.path.exists(fname):
        save(genconfig(), fname)
    with open(fname) as f:
        return json.loads(f.read().strip())

def save(cfg, fname="i2tun.json"):
    with open(fname, 'w') as f:
        json.dump(cfg, f)
