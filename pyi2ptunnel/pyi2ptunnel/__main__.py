#!/usr/bin/env python
#
#

from twisted.internet import reactor

from pyi2ptunnel import config
from pyi2ptunnel import tunnel

from argparse import ArgumentParser as AP

import logging
import os

def main():
    
    ap = AP()
    ap.add_argument("--config", help="config file", default="i2ptunnel.json")
    ap.add_argument("--debug", default=False, help="enable debug mode", action="store_const", const=True)
    
    args = ap.parse_args()

    lvl = logging.WARN
    if args.debug:
        lvl = logging.DEBUG
    
    logging.basicConfig(level=lvl, format='%(asctime)s %(levelname)s %(name)s >> %(message)s')
    log = logging.getLogger("pyi2ptunnel.main")
    log.debug('debug on')
    if os.path.exists(args.config):
        cfg = config.Load(args.config)
        log.info('loaded config')

        samaddr = "127.0.0.1:7657"

        # check config for override
        if "sam" in cfg and "addr" in cfg["sam"]:
            samaddr = cfg["sam"]["addr"]
        
        apptunnel = tunnel.TunnelFactory(api="SAM", apiEndpoint=samaddr)
        tunnels = list()        
        tunnels_config = cfg.get('clients', list())
        for tunnel_cfg in tunnels_config:
            type = tunnel_cfg.get("type")
            if type:
                tun = apptunnel.create(type, **tunnel_cfg.get("args", dict()))
                if tun:
                    log.info('added a {} tunnel'.format(type))
                    tunnels.append(tun)
                else:
                    log.error("could not create tunnel a '{}' tunnel".format(type))
            else:
                log.error("no tunnel type specified for {}".format(tunnel.get("name")))
        log.info("run reactor")
        reactor.run()
    else:
        log.error("no such file: {}".format(args.config))
    

if __name__ == "__main__":
    main()        
