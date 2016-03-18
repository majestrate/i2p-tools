#
# i2tun -- ip compatability layer for i2p
#

# net interface
from i2tun import tundev
# link protocol
from i2tun import protocol
from i2tun import link
# packet switch
from i2tun import switch

import asyncio
import collections
import logging
import signal
import struct
import threading

from i2tun import config
from i2p.datatypes import Destination

from argparse import ArgumentParser as AP

from random import randint

def main():

    ap = AP()
    ap.add_argument("--generate", action="store_const", const=True, default=False)
    ap.add_argument("--conf", default="i2tun.json", type=str)
    args = ap.parse_args()

    if args.generate:
        # only initialize config
        conf = config.genconfig()
        d = Destination()
        dp = d.to_public().base32()
        ouraddr = "10.10.{}.{}".format(randint(1, 250), randint(1, 250))
        print("our ip is {}".format(ouraddr))
        print("our destination is {}".format(dp))
        with open(conf['keyfile'], 'wb') as f:
            f.write(d.serialize())
        conf["map"][ouraddr] = dp
        config.save(conf, args.conf)
        print("saved initial config")
        return

    cfg = config.load()
    
    log = logging.getLogger("i2p.tun")

    if 'debug' in cfg:
        lvl = logging.DEBUG
    else:
        lvl = logging.WARN

    logging.basicConfig(level=lvl)

    iface_cfg = cfg["interface"]
    tap = 'tap' in iface_cfg and iface_cfg['tap']
    tun = tundev.opentun(iface_cfg, tap)

    if 'clump' in cfg and cfg['clump']:
        proto = protocol.Clumping
    else:
        proto = protocol.Flat
    sw = switch.Switch(tun)
    print ('registering addresses')
    for addr in cfg["map"]:
        dest = cfg["map"][addr]
        print ("{} -> {}".format(addr, dest))
        sw.registerAddr(addr, dest)
    pump = 0.5
    if 'pump' in cfg:
        pump = int(cfg["pump"]) / 1000.0
    # make handler
    print ("link going up...")
    handler = link.Handler(tun, tap, sw, proto(iface_cfg['mtu'], sw.next_frame, pump), cfg["keyfile"], cfg["sam"])
    print ("network interface going up...")
    tun.up()
    def sighup(signum, frame):
        cfg = config.load(args.conf)
        if 'map' in cfg:
            sw = switch.Switch(tun)
            print ("reload address map")
            for addr in cfg['map']:
                dest = cfg['map'][addr]
                print ("{} -> {}".format(addr, dest))
                sw.registerAddr(addr, dest)
            handler.switch = sw
        print("reloaded config")
    print ("registering sighup handler...")
    signal.signal(signal.SIGHUP, sighup)
    try:
        print ("running")
        handler.loop.run_forever()
    finally:
        handler.loop.close()
                           

if __name__ == "__main__":
    main()
