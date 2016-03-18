#!/usr/bin/env python3.4
#
#
import traceback
import logging
import time

__doc__ = '''
tcp over i2p main tester
'''



def main():
    """
    main driver
    """
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--listen', action='store_const', const=True, default=False)
    ap.add_argument('--debug', action='store_const', const=True, default=False)
    ap.add_argument('--backend', type=str, default='sam')
    ap.add_argument('--samhost', type=str, default='127.0.0.1')
    ap.add_argument('--samport', type=int, default=7656)
    ap.add_argument('--host', default='psi.i2p')
    ap.add_argument('--port', default=80, type=int)

    args = ap.parse_args()

    lvl = logging.INFO

    if args.debug:
        lvl = logging.DEBUG

    logging.basicConfig(level=lvl)
    log = logging.getLogger("i2p.socket.main")

    if args.backend.lower() == 'i2cp':
        from i2p.socket import i2cp as socket
    elif args.backend.lower() == 'sam':
        from i2p.socket import sam as socket
    
    if args.listen:
        serv = socket.socket(samaddr=(args.samhost, args.samport))
        serv.bind('site.key')
        log.info('bound to: {}'.format(serv.dest))
        while args.listen:
            sock, addr = serv.accept()
            log.info('connection from {}'.format(addr))
            data = sock.recv(1024)
            log.info('recv data')
            log.info(data)
            sock.send(data)
            sock.close()
    try:
        log.debug("create socket")
        # make the socket
        sock = socket.socket(samaddr=(args.samhost, args.samport))
        # run it
        log.debug("connect")
        sock.connect((args.host, args.port))
        log.debug("send")
        data = 'GET / HTTP/1.1\r\nHost: {}\r\n\r\n'.format(args.host).encode("utf-8")
        sock.send(data)
        log.debug("sent")
        data_recv = sock.recv(1024)
        log.info('recv')
        log.info(data_recv)
        sock.close()
        log.debug("closed")
    except Exception as e:
        log.error(e)
        traceback.print_exc()




if __name__ == '__main__':
    main()
