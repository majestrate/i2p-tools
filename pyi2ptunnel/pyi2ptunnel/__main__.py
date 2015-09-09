#!/usr/bin/env python
#
#

from twisted.internet import reactor
from twisted.internet.endpoints import serverFromString

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
            tun_name = tunnel_cfg.get("name")
            if type:
                # build tunnel factory
                tunnelFactory = apptunnel.createClient(type, **tunnel_cfg.get("args", dict()))
                if tunnelFactory:
                    ep_str = tunnel_cfg.get("listen")
                    if ep_str:
                        # start it up
                        ep = serverFromString(reactor, str(ep_str))
                        ep.listen(tunnelFactory())
                        log.info("{} will listen on {}".format(tun_name, ep_str))
                    else:
                        log.error("no listen address specified for {}".format(tun_name))
            else:
                log.error("no tunnel type specified for {}".format(tun_name))
        log.info("run reactor")
        reactor.run()
    else:
        log.error("no such file: {}".format(args.config))
    

if __name__ == "__main__":
    main()        
