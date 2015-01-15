#!/usr/bin/env python3.4
from i2p.i2cp import client as i2cp
import trollius as asyncio
import pytun
import threading
import logging
import struct
import select

class IPV4Handler(i2cp.I2CPHandler):

    def __init__(self, remote_dest, our_addr, their_addr, mtu):
        self._them = remote_dest
        self._iface = pytun.TunTapDevice()
        self._iface.addr = our_addr
        self._iface.dstaddr = their_addr
        self._iface.mtu = mtu
        self._mtu = mtu
        self._iface.up()

    @asyncio.coroutine
    def session_made(self, con):
        print ('we are {}'.format(con.dest.base32()))
        self.con = con
        asyncio.get_event_loop().add_reader(self._iface, self._read_packet)

    def _read_packet(self):
        data = self._iface.read(self._mtu)
        self.con.send_dsa_dgram(self._them, data)

    @asyncio.coroutine
    def got_dgram(self, dest, data, srcport, dstport):
        if dest.base32() == self._them:
            self._iface.write(data)

    def __del__(self):
        del self._iface

def main():
    import argparse
    import logging
    ap = argparse.ArgumentParser()
    #logging.basicConfig(level=logging.DEBUG)
    ap.add_argument('--remote', required=True, type=str)
    ap.add_argument('--our-addr', required=True, type=str)
    ap.add_argument('--their-addr', required=True, type=str)
    ap.add_argument('--mtu', default=3600 ,type=int)
    ap.add_argument('--i2cp-host', default='127.0.0.1', type=str)
    ap.add_argument('--i2cp-port', default=7654, type=int)
    
    args = ap.parse_args()

    handler = IPV4Handler(args.remote, args.our_addr, args.their_addr, args.mtu)
    con = i2cp.Connection(handler, i2cp_host=args.i2cp_host, i2cp_port=args.i2cp_port, session_options={'inbound.quantity' : '2', 'outbound.quantity' : '2'})
    con.open()
    loop = asyncio.get_event_loop()
    try:
        loop.run_forever()
    finally:
        loop.close()

if __name__ == '__main__':
    main()
    
