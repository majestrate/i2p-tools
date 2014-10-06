#!/usr/bin/env python3.4
from i2p.i2cp import client as i2cp
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
        self._iface.up()

    def session_made(self, con):
        print ('we are {}'.format(con.dest.base32()))
        self.con = con
        threading.Thread(target=self.mainloop, args=(con,)).start()

    def mainloop(self, con):
        while True:
            print ('read')
            buff = self._iface.read(self._iface.mtu)
            print ('send')
            self.con.send_dgram(self._them, buff)

    def got_dgram(self, dest, data, srcport, dstport):
        if dest.base32() == self._them:
            self._iface.write(data)
        

def main():
    import argparse
    import logging
    logging.basicConfig(level=logging.DEBUG)
    ap = argparse.ArgumentParser()
    ap.add_argument('--remote', required=True, type=str)
    ap.add_argument('--our-addr', required=True, type=str)
    ap.add_argument('--their-addr', required=True, type=str)
    ap.add_argument('--mtu', default=3600 ,type=int)
    ap.add_argument('--i2cp-host', default='127.0.0.1', type=str)
    ap.add_argument('--i2cp-port', default=7654, type=int)
    
    args = ap.parse_args()

    handler = IPV4Handler(args.remote, args.our_addr, args.their_addr, args.mtu)
    con = i2cp.Connection(handler, i2cp_host=args.i2cp_host, i2cp_port=args.i2cp_port)
    con.open()
    con.start()


if __name__ == '__main__':
    main()
    
